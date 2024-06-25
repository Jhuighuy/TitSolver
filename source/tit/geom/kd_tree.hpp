/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <iterator>
#include <ranges>
#include <span>
#include <tuple>
#include <utility>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/math.hpp"
#include "tit/core/par.hpp"
#include "tit/core/profiler.hpp"
#include "tit/core/type_traits.hpp"
#include "tit/core/vec.hpp"

#include "tit/geom/bbox.hpp"

namespace tit::geom {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// K-dimensional tree.
/// Inspired by nanoflann: https://github.com/jlblancoc/nanoflann
template<std::ranges::view Points>
  requires std::ranges::sized_range<Points> &&
           std::ranges::random_access_range<Points> &&
           is_vec_v<std::ranges::range_value_t<Points>>
class KDTree final {
public:

  /// Point type.
  using Vec = std::ranges::range_value_t<Points>;

  /// Numeric type used by the point type.
  using Num = vec_num_t<Vec>;

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Initialize and build the K-dimensional tree.
  ///
  /// @param max_leaf_size Maximum amount of points in the leaf node.
  explicit KDTree(Points points, size_t max_leaf_size = 1)
      : points_{std::move(points)}, max_leaf_size_{max_leaf_size} {
    TIT_PROFILE_SECTION("tit::KDTree::KDTree()");
    TIT_ASSERT(max_leaf_size_ > 0, "Maximal leaf size should be positive.");
    if (std::ranges::empty(points_)) return;
    // Initialize identity points permutation.
    const auto size = std::ranges::size(points_);
    perm_.resize(size);
    std::ranges::copy(std::views::iota(size_t{0}, size), perm_.begin());
    // Compute the root tree node (and bounding box).
    par::TaskGroup tasks{};
    root_ = build_subtree_(tasks, perm_, root_bbox_);
    tasks.wait();
  }

  /// Find the points within the radius to the given point.
  template<std::output_iterator<size_t> OutIter>
  constexpr auto search(const Vec& search_point,
                        const Num& search_radius,
                        OutIter indices) const -> OutIter {
    TIT_ASSERT(search_radius > 0.0, "Search radius should be positive.");
    TIT_ASSERT(root_ != nullptr, "Tree was not built.");
    // Compute distance from the query point to the root bounding box
    // per each dimension. (By "dist" square distances are meant.)
    const auto dists = pow2(search_point - root_bbox_.clamp(search_point));
    const auto search_dist = pow2(search_radius);
    // Do the actual search.
    search_subtree_(root_, dists, search_point, search_dist, indices);
    return indices;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  union KDTreeNode_ {
    std::span<const size_t> leaf{};
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
    struct {
      size_t cut_dim;
      Num cut_left;
      Num cut_right;
      KDTreeNode_* left_subtree;
      KDTreeNode_* right_subtree;
    };
#pragma GCC diagnostic pop
  }; // union KDTreeNode_

  // Build the K-dimensional subtree.
  // On the input `bbox` contains a rough estimate that was guessed by the
  // caller. On return it contains an exact bounding box of the subtree.
  auto build_subtree_(par::TaskGroup& tasks,
                      std::span<size_t> perm,
                      BBox<Vec>& bbox) -> KDTreeNode_* {
    // Allocate node.
    const auto node = pool_.create();
    // Compute the bounding box.
    bbox = BBox{points_[perm.front()]};
    for (const auto i : perm | std::views::drop(1)) bbox.expand(points_[i]);
    // Is leaf node reached?
    if (perm.size() <= max_leaf_size_) {
      // Fill the leaf node and end partitioning.
      node->leaf = perm;
      node->left_subtree = node->right_subtree = nullptr;
    } else {
      // Split the points based on the "widest" bounding box dimension.
      const auto cut_dim = node->cut_dim = max_value_index(bbox.extents());
      const auto cut_val = bbox.center()[cut_dim];
      const auto pivot =
          partition_subtree_(perm.begin(), perm.end(), cut_dim, cut_val);
      const auto left_perm = std::span(perm.begin(), pivot);
      const auto right_perm = std::span(pivot, perm.end());
      // Recursively build subtrees.
      tasks.run(run_async_(left_perm), [=, &tasks, this] {
        BBox<Vec> left_bbox{};
        node->left_subtree = build_subtree_(tasks, left_perm, left_bbox);
        node->cut_left = left_bbox.high()[cut_dim];
      });
      tasks.run(run_async_(right_perm), [=, &tasks, this] {
        BBox<Vec> right_bbox{};
        node->right_subtree = build_subtree_(tasks, right_perm, right_bbox);
        node->cut_right = right_bbox.low()[cut_dim];
      });
    }
    return node;
  }

  // Partition the K-dimensional subtree points (iterator version).
  template<class Iter>
  constexpr auto partition_subtree_(Iter first,
                                    Iter last,
                                    size_t cut_dim,
                                    const Num& cut_val) const -> Iter {
    // Partition the tree based on the cut plane: separate those that
    // are to the left ("<") from those that are to the right or exactly on
    // the splitting plane (">=").
    auto pivot = std::partition(first, last, [&](size_t index) {
      return points_[index][cut_dim] < cut_val;
    });
    // Try to balance the partition by redistributing the points that are
    // exactly ("==") on the splitting plane.
    //
    // Partition may be already balanced if the left part ("<") is too large,
    // so moving points into it from the part after the pivot makes no sense:
    //
    //   first                      middle                     last
    //   |--------------------------|--------------------------|
    //   |------------- "<" --------------|------- ">=" -------|
    //   first                            pivot                last
    const auto middle = first + (last - first) / 2;
    if (middle <= pivot) return pivot;
    // Now partition the right part (">=") on the middle part ("==") and the
    // truly right part (">").
    pivot = std::partition(pivot, last, [&](size_t index) {
      return points_[index][cut_dim] == cut_val;
    });
    // Two outcomes may be possible:
    //
    // - Either midpoint of our range is the best possible option:
    //
    //   first                      middle                     last
    //   |--------------------------|--------------------------|
    //   |--------- "<" ----------|- "==" -|------- ">" -------|
    //   first                             pivot               last
    //
    // - Either it is optimal to attach the middle part to the left:
    //
    //   first                      middle                     last
    //   |--------------------------|--------------------------|
    //   |----- "<" ----|- "==" -|------------ ">" ------------|
    //   first                   pivot                         last
    return std::min(pivot, middle);
  }

  // Should the ordering be done in parallel?
  constexpr auto run_async_(std::span<size_t> perm) noexcept -> bool {
    static constexpr size_t TooSmall = 50;
    return perm.size() >= TooSmall * max_leaf_size_;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  // Search for the point neighbors in the K-dimensional subtree.
  // Parameters are passed by references in order to minimize stack usage.
  template<std::output_iterator<size_t> OutIter>
  constexpr void search_subtree_(const KDTreeNode_* node,
                                 Vec dists,
                                 const Vec& search_point,
                                 const Num& search_dist,
                                 OutIter& indices) const {
    // Is leaf node reached?
    if (node->left_subtree == nullptr) {
      TIT_ASSERT(node->right_subtree == nullptr, "Invalid leaf node.");
      // Iterate through the points.
      for (size_t i : node->leaf) {
        const auto dist = norm2(search_point - points_[i]);
        if (dist < search_dist) *indices++ = i;
      }
    } else {
      // Determine which branch should be taken first.
      const auto cut_dim = node->cut_dim;
      const auto [cut_dist, first_node, second_node] = [&] {
        const auto delta_left = search_point[cut_dim] - node->cut_left;
        const auto delta_right = node->cut_right - search_point[cut_dim];
        return delta_left < delta_right ?
                   // Point is on the left to the cut plane, so the
                   // corresponding subtree should be searched first.
                   std::tuple{pow2(delta_right),
                              node->left_subtree,
                              node->right_subtree} :
                   // Point is on the right to the cut plane, so the
                   // corresponding subtree should be searched first.
                   std::tuple{pow2(delta_left),
                              node->right_subtree,
                              node->left_subtree};
      }();
      // Search in the first subtree.
      search_subtree_(first_node, dists, search_point, search_dist, indices);
      // Search in the second subtree (if it not too far).
      dists[cut_dim] = cut_dist;
      if (const auto dist = sum(dists); dist < search_dist) {
        search_subtree_(second_node, dists, search_point, search_dist, indices);
      }
    }
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  Points points_;
  size_t max_leaf_size_;
  par::MemoryPool<KDTreeNode_> pool_;
  const KDTreeNode_* root_ = nullptr;
  BBox<Vec> root_bbox_;
  std::vector<size_t> perm_;

}; // class KDTree

// Wrap a viewable range into a view on construction.
template<class Points, class... Args>
KDTree(Points&&, Args...) -> KDTree<std::views::all_t<Points>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// K-dimensional tree factory.
class KDTreeFactory final {
public:

  /// Construct a K-dimensional tree factory.
  ///
  /// @param max_leaf_size Maximum amount of points in the leaf node.
  constexpr explicit KDTreeFactory(size_t max_leaf_size = 1)
      : max_leaf_size_{max_leaf_size} {
    TIT_ASSERT(max_leaf_size_ > 0, "Maximal leaf size should be positive.");
  }

  /// Produce a K-dimensional tree for the specified set of points.
  template<std::ranges::viewable_range Points>
    requires deduce_constructible_from<KDTree, Points&&, size_t>
  constexpr auto operator()(Points&& points) const noexcept {
    return KDTree{std::forward<Points>(points), max_leaf_size_};
  }

private:

  size_t max_leaf_size_;

}; // class KDTreeFactory

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::geom
