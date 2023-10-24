/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License
 * See /LICENSE.md for license information.
 * SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <iterator>
#include <ranges>
#include <tuple>
#include <utility>
#include <vector>

#include "tit/core/assert.hpp"
#include "tit/core/math.hpp"
#include "tit/core/pool_allocator.hpp"
#include "tit/core/types.hpp"
#include "tit/core/vec.hpp"
#include "tit/geom/bbox.hpp"
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
  using PointBBox = decltype(BBox(std::declval<Point>()));
  /** Numeric type used by the point type. */
  using Real = vec_num_t<Point>;

private:

  union KDTreeNode_ {
    std::ranges::subrange<const size_t*> leaf;
    struct {
      size_t cut_dim;
      Real cut_left, cut_right;
      KDTreeNode_* left_subtree;
      KDTreeNode_* right_subtree;
    };
  }; // union KDTreeNode_

  Points points_;
  size_t max_leaf_size_;
  PoolAllocator<KDTreeNode_> alloc_;
  std::vector<size_t> point_perm_;
  const KDTreeNode_* root_node_ = nullptr;
  PointBBox root_bbox_;

  /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

public:

  /** K-dimensional tree is non-copyable. */
  KDTree(const KDTree&) = delete;
  /** K-dimensional tree is non-copy-assignable. */
  KDTree& operator=(const KDTree&) = delete;

  /** Move-construct the K-dimensional tree. */
  KDTree(KDTree&&) = default;
  /** Move-assign the K-dimensional tree. */
  KDTree& operator=(KDTree&&) = default;

  /** Initialze and build the K-dimensional tree.
   ** @param max_leaf_size Maximum amount of points in the leaf node. */
  constexpr explicit KDTree(Points points, size_t max_leaf_size = 1)
      : points_{std::move(points)}, max_leaf_size_{max_leaf_size} {
    TIT_ASSERT(max_leaf_size_ > 0, "Maximal leaf size should be positive.");
    if (std::ranges::empty(points_)) return;
    // Initialize identity points permutation.
    const auto size = std::ranges::size(points_);
    point_perm_.resize(size);
    std::ranges::copy(std::views::iota(size_t{0}, size), point_perm_.begin());
    // Compute the root tree node (and bounding box).
    root_node_ = build_subtree_</*IsRoot=*/true>(
        point_perm_.data(), point_perm_.data() + size, root_bbox_);
  }

private:

  // Compute bounding box for the K-dimensional subtree.
  template<bool Parallel>
  constexpr auto subtree_bbox_(const size_t* first,
                               const size_t* last) const noexcept -> PointBBox {
    TIT_ASSERT(first < last, "Invalid subtree range.");
    // TODO: run in parallel!
    auto bbox = BBox{points_[*first]};
    while (++first != last) bbox.update(points_[*first]);
    return bbox;
  }

  // Build the K-dimensional subtree.
  // On the input `bbox` contains a rough estimate that was guessed by the
  // caller. On return it contains an exact bounding box of the subtree.
  template<bool IsRoot = false>
  constexpr auto build_subtree_(size_t* first, size_t* last, PointBBox& bbox)
      -> KDTreeNode_* {
    TIT_ASSERT(first < last, "Invalid subtree range.");
    // Allocate node.
    // TODO: We are not correctly initializing `node`.
    const auto node = alloc_.allocate(1);
    const auto actual_bbox = subtree_bbox_</*Parallel=*/IsRoot>(first, last);
    if constexpr (IsRoot) bbox = actual_bbox;
    // Is leaf node reached?
    if (last - first <= max_leaf_size_) {
      // Fill the leaf node and end partitioning.
      node->left_subtree = node->right_subtree = nullptr;
      node->leaf = {first, last};
    } else {
      // Split the points based on the "widest" bounding box dimension.
      const auto cut_dim = argmax_value(actual_bbox.extents());
      const auto cut_value = actual_bbox.clamp(bbox.center())[cut_dim];
      const auto pivot = partition_subtree_(first, last, cut_dim, cut_value);
      TIT_ASSERT(first <= pivot && pivot <= last, "Invalid pivot.");
      node->cut_dim = cut_dim;
      auto build_left_subtree = [=, this] {
        // Build left subtree and update it's bounding box.
        auto left_bbox = bbox;
        left_bbox.high[cut_dim] = cut_value;
        node->left_subtree = build_subtree_(first, pivot, left_bbox);
        node->cut_left = left_bbox.high[cut_dim];
      };
      auto build_right_subtree = [=, this] {
        // Build right subtree and update it's bounding box.
        auto right_bbox = bbox;
        right_bbox.low[cut_dim] = cut_value;
        node->right_subtree = build_subtree_(pivot, last, right_bbox);
        node->cut_right = right_bbox.low[cut_dim];
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
    TIT_ASSERT(first < last, "Invalid subtree range.");
    // Shift the points that are to the left of the splititng plane to the
    // front of the list.
    const auto to_the_left = [&](size_t index) {
      return points_[index][cut_dim] < cut_value;
    };
    auto pivot = std::partition(first, last, to_the_left);
    const auto middle = first + (last - first) / 2;
    if (pivot > middle) return pivot;
    // Now at the pivot are the points which are on the splitting plane
    // or to the right of it. Now shift to the pivot points that are on the
    // plane and find the optimal pivot value to maintain the tree balanced.
    const auto on_the_plane = [&](size_t index) {
      return points_[index][cut_dim] == cut_value;
    };
    pivot = std::partition(pivot, last, on_the_plane);
    if (pivot < middle) return pivot;
    return middle;
  }

  /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

public:

  /** Find the points within the radius to the given point. */
  template<std::output_iterator<size_t> OutIter>
  constexpr auto search(Point search_point, Real search_radius,
                        OutIter out) const noexcept -> OutIter {
    TIT_ASSERT(search_radius > 0.0, "Search radius should be positive.");
    TIT_ASSERT(root_node_ != nullptr, "Tree was not built.");
    // Compute distance from the query point to the root bounding box
    // per each dimension. (By "dist" square distances are ment.)
    const auto dists = pow2(search_point - root_bbox_.clamp(search_point));
    const auto search_dist = pow2(search_radius);
    // Do the actual search.
    search_subtree_(root_node_, dists, search_point, search_dist, out);
    return out;
  }

private:

  // Search for the point neighbours in the K-dimensional subtree.
  // Parameters are passed by references in order to minimize stack usage.
  template<std::output_iterator<size_t> OutIter>
  constexpr void search_subtree_(const KDTreeNode_* node, Point dists,
                                 const Point& search_point, Real search_dist,
                                 OutIter& out) const noexcept {
    // Is leaf node reached?
    if (node->left_subtree == nullptr) {
      TIT_ASSERT(node->right_subtree == nullptr, "Invalid leaf node.");
      // Iterate through the points.
      for (size_t i : node->leaf) {
        const auto dist = norm2(search_point - points_[i]);
        if (dist < search_dist) *out++ = i;
      }
    } else {
      // Determine which branch should be taken first.
      const auto cut_dim = node->cut_dim;
      const auto [cut_dist, first_node, second_node] = [&] {
        const auto delta_left = search_point[cut_dim] - node->cut_left;
        const auto delta_right = node->cut_right - search_point[cut_dim];
        if (delta_left < delta_right) {
          // Point is on the left to the cut plane, so the corresponding
          // subtree should be searched first.
          return std::tuple{pow2(delta_right), //
                            node->left_subtree, node->right_subtree};
        } else {
          // Point is on the right to the cut plane, so the corresponding
          // subtree should be searched first.
          return std::tuple{pow2(delta_left), //
                            node->right_subtree, node->left_subtree};
        }
      }();
      // Search in the first subtree.
      search_subtree_(first_node, dists, search_point, search_dist, out);
      // Search in the second subtree (if it not too far).
      dists[cut_dim] = cut_dist;
      if (const auto dist = sum(dists); dist < search_dist) {
        search_subtree_(second_node, dists, search_point, search_dist, out);
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
