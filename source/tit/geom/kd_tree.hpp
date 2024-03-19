/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <iterator> // IWYU pragma: keep
#include <numeric>
#include <ranges>
#include <tuple>
#include <utility>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/math.hpp"
#include "tit/core/vec.hpp"
#include "tit/geom/bbox.hpp"
#include "tit/par/memory_pool.hpp"
#include "tit/par/thread.hpp"

namespace tit::geom {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** K-dimensional tree.
 ** Inspired by nanoflann: https://github.com/jlblancoc/nanoflann
\******************************************************************************/
template<std::ranges::view Points>
  requires std::ranges::sized_range<Points> &&
           std::ranges::random_access_range<Points> &&
           is_vec_v<std::ranges::range_value_t<Points>>
class KDTree final {
public:

  /** Point type. */
  using Point = std::ranges::range_value_t<Points>;
  /** Bounding box type. */
  using PointBBox = bbox_t<Point>;

  /** Numeric type used by the point type. */
  using Real = vec_num_t<Point>;

private:

  union KDTreeNode_ {
    std::ranges::subrange<size_t const*> leaf{};
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
    struct {
      size_t cut_dim;
      Real cut_left, cut_right;
      KDTreeNode_* left_subtree;
      KDTreeNode_* right_subtree;
    };
#pragma GCC diagnostic pop
  }; // union KDTreeNode_

  Points points_;
  size_t max_leaf_size_;
  par::MemoryPool<KDTreeNode_> pool_;
  std::vector<size_t> point_perm_;
  KDTreeNode_ const* root_node_ = nullptr;
  PointBBox root_bbox_;

  /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

public:

  /** K-dimensional tree is non-copyable. */
  KDTree(KDTree const&) = delete;
  /** K-dimensional tree is non-copy-assignable. */
  auto operator=(KDTree const&) -> KDTree& = delete;

  /** Move-construct the K-dimensional tree. */
  KDTree(KDTree&&) = default;
  /** Move-assign the K-dimensional tree. */
  auto operator=(KDTree&&) -> KDTree& = default;

  /** Destroy the K-dimensional tree. */
  ~KDTree() = default;

  /** Initialize and build the K-dimensional tree.
   ** @param max_leaf_size Maximum amount of points in the leaf node. */
  constexpr explicit KDTree(Points points, size_t max_leaf_size = 1)
      : points_{std::move(points)}, max_leaf_size_{max_leaf_size} {
    TIT_ASSERT(max_leaf_size_ > 0, "Maximal leaf size should be positive.");
    if (std::ranges::empty(points_)) return;
    // Initialize identity points permutation.
    auto const size = std::ranges::size(points_);
    point_perm_.resize(size);
    std::ranges::copy(std::views::iota(0UZ, size), point_perm_.begin());
    // Compute the root tree node (and bounding box).
    root_node_ = build_subtree_</*IsRoot=*/true>(
        point_perm_.data(), point_perm_.data() + size, root_bbox_);
  }

private:

  // Compute bounding box for the K-dimensional subtree.
  template<bool Parallel>
  constexpr auto subtree_bbox_(size_t const* first,
                               size_t const* last) const noexcept -> PointBBox {
    TIT_ASSERT(first < last, "Invalid subtree range.");
    // TODO: run in parallel!
    auto bbox = BBox{points_[*first]};
    // TODO: refactor with `std::span`.
    // NOLINTNEXTLINE(*-bounds-pointer-arithmetic)
    while (++first != last) bbox.update(points_[*first]);
    return bbox;
  }

  // Build the K-dimensional subtree.
  // On the input `bbox` contains a rough estimate that was guessed by the
  // caller. On return it contains an exact bounding box of the subtree.
  template<bool IsRoot = false>
  constexpr auto build_subtree_(size_t* first, size_t* last, PointBBox& bbox)
      -> KDTreeNode_* {
    TIT_ASSERT(first != nullptr && first < last, "Invalid subtree range.");
    // Allocate node.
    // TODO: We are not correctly initializing `node`.
    auto const node = pool_.allocate(1);
    auto const actual_bbox = subtree_bbox_</*Parallel=*/IsRoot>(first, last);
    if constexpr (IsRoot) bbox = actual_bbox;
    // Is leaf node reached?
    if (static_cast<size_t>(last - first) <= max_leaf_size_) {
      // Fill the leaf node and end partitioning.
      node->left_subtree = node->right_subtree = nullptr;
      node->leaf = {first, last};
    } else {
      // Split the points based on the "widest" bounding box dimension.
      auto const cut_dim = argmax_value(actual_bbox.extents());
      auto const cut_value = actual_bbox.clamp(bbox.center())[cut_dim];
      auto const pivot = partition_subtree_(first, last, cut_dim, cut_value);
      TIT_ASSERT(first <= pivot && pivot <= last, "Invalid pivot.");
      node->cut_dim = cut_dim;
      auto build_left_subtree = [=, this] {
        // Build left subtree and update it's bounding box.
        auto [left_bbox, _] = bbox.split(cut_dim, cut_value);
        node->left_subtree = build_subtree_(first, pivot, left_bbox);
        node->cut_left = left_bbox.high()[cut_dim];
      };
      auto build_right_subtree = [=, this] {
        // Build right subtree and update it's bounding box.
        auto [_, right_bbox] = bbox.split(cut_dim, cut_value);
        node->right_subtree = build_subtree_(pivot, last, right_bbox);
        node->cut_right = right_bbox.low()[cut_dim];
      };
      // Execute tasks.
      par::invoke(std::move(build_left_subtree),
                  std::move(build_right_subtree));
    }
    bbox = actual_bbox;
    return node;
  }

  // Partition the K-dimensional subtree points.
  constexpr auto partition_subtree_(size_t* first, size_t* last, //
                                    size_t cut_dim, Real cut_value) noexcept
      -> size_t* {
    TIT_ASSERT(first != nullptr && first < last, "Invalid subtree range.");
    // TODO: make a general `balanced_partition` algorithm from this.
    // Partition the tree based on the cut plane: separate those that
    // are to the left ("<") from those that are to the right or exactly on
    // the splitting plane (">=").
    auto* pivot = std::partition(first, last, [&](size_t index) {
      return points_[index][cut_dim] < cut_value;
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
    auto* const middle = std::midpoint(first, last);
    if (middle <= pivot) return pivot;
    // Now partition the right part (">=") on the middle part ("==") and the
    // truly right part (">").
    pivot = std::partition(pivot, last, [&](size_t index) {
      // (Here we are not using "==" because of strict floating point
      //  conversions. "<=" does exactly the same thing.)
      return points_[index][cut_dim] <= cut_value;
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

  /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

public:

  /** Find the points within the radius to the given point. */
  template<std::output_iterator<size_t> OutIter>
  constexpr auto search(Point search_point, Real search_radius,
                        OutIter indices) const noexcept -> OutIter {
    TIT_ASSERT(search_radius > 0.0, "Search radius should be positive.");
    TIT_ASSERT(root_node_ != nullptr, "Tree was not built.");
    // Compute distance from the query point to the root bounding box
    // per each dimension. (By "dist" square distances are meant.)
    auto const dists = pow2(search_point - root_bbox_.clamp(search_point));
    auto const search_dist = pow2(search_radius);
    // Do the actual search.
    search_subtree_(root_node_, dists, search_point, search_dist, indices);
    return indices;
  }

private:

  // Search for the point neighbors in the K-dimensional subtree.
  // Parameters are passed by references in order to minimize stack usage.
  template<std::output_iterator<size_t> OutIter>
  constexpr void search_subtree_(KDTreeNode_ const* node, Point dists,
                                 Point const& search_point, Real search_dist,
                                 OutIter& indices) const noexcept {
    // Is leaf node reached?
    if (node->left_subtree == nullptr) {
      TIT_ASSERT(node->right_subtree == nullptr, "Invalid leaf node.");
      // Iterate through the points.
      for (size_t i : node->leaf) {
        auto const dist = norm2(search_point - points_[i]);
        if (dist < search_dist) *indices++ = i;
      }
    } else {
      // Determine which branch should be taken first.
      auto const cut_dim = node->cut_dim;
      auto const [cut_dist, first_node, second_node] = [&] {
        auto const delta_left = search_point[cut_dim] - node->cut_left;
        auto const delta_right = node->cut_right - search_point[cut_dim];
        return delta_left < delta_right ?
                   // Point is on the left to the cut plane, so the
                   // corresponding subtree should be searched first.
                   std::tuple{pow2(delta_right), //
                              node->left_subtree, node->right_subtree} :
                   // Point is on the right to the cut plane, so the
                   // corresponding subtree should be searched first.
                   std::tuple{pow2(delta_left), //
                              node->right_subtree, node->left_subtree};
      }();
      // Search in the first subtree.
      search_subtree_(first_node, dists, search_point, search_dist, indices);
      // Search in the second subtree (if it not too far).
      dists[cut_dim] = cut_dist;
      if (auto const dist = sum(dists); dist < search_dist) {
        search_subtree_(second_node, dists, search_point, search_dist, indices);
      }
    }
  }

}; // class KDTree

// Wrap a viewable range into a view on construction.
template<class Points, class... Args>
KDTree(Points&&, Args...) -> KDTree<std::views::all_t<Points>>;

template<class... Args>
concept can_kd_tree_ = requires { KDTree{std::declval<Args>()...}; };

/******************************************************************************\
 ** K-dimensional tree factory.
\******************************************************************************/
class KDTreeFactory final {
private:

  size_t max_leaf_size_;

public:

  /** Construct a K-dimensional tree factory.
   ** @param max_leaf_size Maximum amount of points in the leaf node. */
  constexpr explicit KDTreeFactory(size_t max_leaf_size = 1)
      : max_leaf_size_{max_leaf_size} {
    TIT_ASSERT(max_leaf_size_ > 0, "Maximal leaf size should be positive.");
  }

  /** Produce a K-dimensional tree for the specified set of points. */
  template<std::ranges::viewable_range Points>
    requires can_kd_tree_<Points, size_t>
  constexpr auto operator()(Points&& points) const noexcept {
    return KDTree{std::forward<Points>(points), max_leaf_size_};
  }

}; // class KDTreeFactory

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit::geom
