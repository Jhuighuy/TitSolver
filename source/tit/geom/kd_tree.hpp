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

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Initialize and build the K-dimensional tree.
  ///
  /// @param max_leaf_size Maximum amount of points in the leaf node.
  explicit KDTree(Points points, size_t max_leaf_size = 1)
      : points_{std::move(points)}, max_leaf_size_{max_leaf_size} {
    TIT_PROFILE_SECTION("KDTree::KDTree()");
    TIT_ASSERT(max_leaf_size_ > 0, "Maximal leaf size should be positive.");
    build_tree_();
  }

  /// Find the points within the radius to the given point.
  template<std::output_iterator<size_t> OutIter>
  constexpr auto search(const Vec& search_point,
                        vec_num_t<Vec> search_radius,
                        OutIter out) const -> OutIter {
    TIT_ASSERT(search_radius > 0.0, "Search radius should be positive.");
    search_tree_(search_point, search_radius, out);
    return out;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  union KDTreeNode_ {
    std::span<const size_t> perm{};
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
    struct {
      size_t cut_dim;
      vec_num_t<Vec> cut_left;
      vec_num_t<Vec> cut_right;
      KDTreeNode_* left_subtree;
      KDTreeNode_* right_subtree;
    };
#pragma GCC diagnostic pop
  }; // union KDTreeNode_

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  // Build the K-dimensional tree.
  auto build_tree_() {
    if (std::ranges::empty(points_)) return;

    // Initialize identity points permutation.
    perm_.resize(std::size(points_));
    std::ranges::copy(std::views::iota(size_t{0}, perm_.size()), perm_.begin());

    // Compute the root tree node (and bounding box).
    par::TaskGroup tasks{};
    root_node_ = build_subtree_</*IsRoot=*/true>(tasks, perm_, tree_box_);
    tasks.wait();
  }

  // Build the K-dimensional subtree.
  // On the input `box` contains a rough estimate that was guessed by the
  // caller. On return it contains an exact bounding box of the subtree.
  //
  /// @todo We must do something with the `IsRoot` template parameter
  ///       and the `box` parameter. It is confusing.
  template<bool IsRoot = false>
  auto build_subtree_(par::TaskGroup& tasks,
                      std::span<size_t> perm,
                      BBox<Vec>& box) -> KDTreeNode_* {
    // Compute bounding box.
    //
    /// @todo Introduce a helper function for this computation.
    BBox true_box{points_[perm.front()]};
    for (const auto i : perm | std::views::drop(1)) true_box.expand(points_[i]);
    if constexpr (IsRoot) box = true_box;

    // Is leaf node reached?
    const auto node = pool_.create();
    if (perm.size() <= max_leaf_size_) {
      // Fill the leaf node and end partitioning.
      node->perm = perm;
      node->left_subtree = node->right_subtree = nullptr;
    } else {
      // Split the points based on the "widest" bounding box dimension.
      const auto cut_dim = max_value_index(true_box.extents());
      const auto cut_val = true_box.clamp(box.center())[cut_dim];
      const auto [left_box, right_box] = box.split(cut_dim, cut_val);
      const auto pivot =
          partition_subtree_(perm.begin(), perm.end(), cut_dim, cut_val);
      const std::span left_perm(perm.begin(), pivot);
      const std::span right_perm(pivot, perm.end());

      // Build subtrees.
      //
      /// @todo These two `NOLINT`s are a workaround for a clang-tidy bug.
      node->cut_dim = cut_dim;
      tasks.run( //
          is_async_(left_perm),
          [left_box, left_perm, cut_dim, node, &tasks, this] {
            auto true_left_box = left_box; // NOLINT
            node->left_subtree =
                build_subtree_(tasks, left_perm, true_left_box);
            node->cut_left = true_left_box.high()[cut_dim];
          });
      tasks.run( //
          is_async_(right_perm),
          [right_box, right_perm, cut_dim, node, &tasks, this] {
            auto true_right_box = right_box; // NOLINT
            node->right_subtree =
                build_subtree_(tasks, right_perm, true_right_box);
            node->cut_right = true_right_box.low()[cut_dim];
          });
    }

    box = true_box;
    return node;
  }

  // Partition the K-dimensional subtree points (iterator version).
  template<class Iter, class Sent>
  constexpr auto partition_subtree_(Iter first,
                                    Sent last,
                                    size_t cut_dim,
                                    vec_num_t<Vec> cut_val) const -> Iter {
    // Partition the tree based on the cut plane: separate those that
    // are to the left ("<") from those that are to the right or exactly on
    // the splitting plane (">=").
    auto pivot = std::begin( //
        std::ranges::partition(
            first,
            last,
            [cut_dim, cut_val](const Vec& p) { return p[cut_dim] < cut_val; },
            [this](size_t i) -> const Vec& { return points_[i]; }));

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
    pivot = std::begin( //
        std::ranges::partition(
            pivot,
            last,
            [cut_dim, cut_val](const Vec& p) { return p[cut_dim] == cut_val; },
            [this](size_t i) -> const Vec& { return points_[i]; }));

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

  // Should the building be done in parallel?
  static constexpr auto is_async_(std::span<size_t> perm) noexcept -> bool {
    static constexpr size_t TooSmall = 50; // Empirical value.
    return perm.size() >= TooSmall;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  // Search for the point neighbors in the K-dimensional tree.
  //
  /// @todo Search is slow for some reason. Investigate.
  template<std::output_iterator<size_t> OutIter>
  constexpr void search_tree_(const Vec& search_point,
                              vec_num_t<Vec> search_radius,
                              OutIter out) const {
    // Compute distance from the query point to the root bounding box
    // per each dimension. (By "dist" square distances are meant.)
    const auto search_dist = pow2(search_radius);
    const auto init_dists = pow2(search_point - tree_box_.clamp(search_point));

    // Recursively search the tree.
    TIT_ASSERT(root_node_ != nullptr, "Tree was not built!");
    search_subtree_(root_node_, init_dists, search_point, search_dist, out);
  }

  // Search for the point neighbors in the K-dimensional subtree.
  // Parameters are passed by references in order to minimize stack usage.
  template<std::output_iterator<size_t> OutIter>
  constexpr void search_subtree_(const KDTreeNode_* node,
                                 Vec dists,
                                 const Vec& search_point,
                                 vec_num_t<Vec> search_dist,
                                 OutIter& out) const {
    if (node->left_subtree == nullptr) {
      TIT_ASSERT(node->right_subtree == nullptr, "Invalid leaf node!");
      // Collect points within the leaf node.
      std::ranges::copy_if(
          node->perm,
          out,
          [&search_point, search_dist](const Vec& point) {
            return norm2(search_point - point) < search_dist;
          },
          [this](size_t i) -> const Vec& { return points_[i]; });
      return;
    }

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
    search_subtree_(first_node, dists, search_point, search_dist, out);

    // Search in the second subtree (if it not too far).
    dists[cut_dim] = cut_dist;
    if (const auto dist = sum(dists); dist < search_dist) {
      search_subtree_(second_node, dists, search_point, search_dist, out);
    }
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  Points points_;
  size_t max_leaf_size_;
  par::MemoryPool<KDTreeNode_> pool_;
  const KDTreeNode_* root_node_ = nullptr;
  BBox<Vec> tree_box_;
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
