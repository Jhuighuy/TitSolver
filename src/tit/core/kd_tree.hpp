/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*\
 * Copyright (C) 2020-2023 Oleg Butakov                                       *
 *                                                                            *
 * Permission is hereby granted, free of charge, to any person obtaining a    *
 * copy of this software and associated documentation files (the "Software"), *
 * to deal in the Software without restriction, including without limitation  *
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,   *
 * and/or sell copies of the Software, and to permit persons to whom the      *
 * Software is furnished to do so, subject to the following conditions:       *
 *                                                                            *
 * The above copyright notice and this permission notice shall be included in *
 * all copies or substantial portions of the Software.                        *
 *                                                                            *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR *
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   *
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    *
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER *
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    *
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        *
 * DEALINGS IN THE SOFTWARE.                                                  *
\*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#pragma once

#include <algorithm>
#include <ranges>
#include <tuple>
#include <utility>
#include <vector>

#include "tit/core/assert.hpp"
#include "tit/core/bbox.hpp"
#include "tit/core/math.hpp"
#include "tit/core/pool_allocator.hpp"
#include "tit/core/types.hpp"
#include "tit/core/vec.hpp"

namespace tit {

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

  union KDTreeNode {
    struct {
      size_t leaf_left, leaf_right;
    };
    struct {
      size_t cut_dim;
      Real cut_left, cut_right;
      KDTreeNode* left_subtree;
      KDTreeNode* right_subtree;
    };
  }; // union KDTreeNode

  Points _points;
  size_t _max_leaf_size;
  PoolAllocator<KDTreeNode> _allocator;
  std::vector<size_t> _point_perm;
  KDTreeNode* _root_node = nullptr;
  PointBBox _root_bbox;

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
      : _points{std::move(points)}, _max_leaf_size{max_leaf_size} {
    TIT_ASSERT(max_leaf_size > 0, "Maximal leaf size should be positive.");
    if (std::ranges::empty(_points)) return;
    // Initialize identity points permutation.
    const auto size = std::ranges::size(_points);
    _point_perm.resize(size);
    std::ranges::copy(std::views::iota(size_t{0}, size), _point_perm.begin());
    // Compute the root tree node (and bounding box).
    _root_node = _build_subtree</*IsRoot=*/true>(0, size, _root_bbox);
  }

private:

  // Compute bounding box for the K-dimensional subtree.
  template<bool Parallel>
  constexpr auto _subtree_bbox(size_t left, size_t right) const noexcept {
    TIT_ASSERT(right <= _point_perm.size(), "`right` index is out of range.");
    TIT_ASSERT(left < right, "`left` index is out of range.");
    // TODO: run in parallel!
    auto bbox = BBox(_points[_point_perm[left]]);
    while (++left < right) bbox.update(_points[_point_perm[left]]);
    return bbox;
  }

  // Build the K-dimensional subtree.
  // On the input `bbox` contains a rough estimate that was guessed by the
  // caller. On return it contains an exact bounding box of the subtree.
  template<bool IsRoot = false>
  constexpr auto _build_subtree(size_t left, size_t right, PointBBox& bbox)
      -> KDTreeNode* {
    TIT_ASSERT(right <= _point_perm.size(), "`right` index is out of range.");
    TIT_ASSERT(left < right, "`left` index is out of range.");
    // Allocate node.
    const auto node = _allocator.allocate();
    const auto actual_bbox = _subtree_bbox</*Parallel=*/IsRoot>(left, right);
    if constexpr (IsRoot) bbox = actual_bbox;
    // Is leaf node reached?
    if (right - left <= _max_leaf_size) {
      // Fill the leaf node and end partitioning.
      node->left_subtree = node->right_subtree = nullptr;
      node->leaf_left = left, node->leaf_right = right;
    } else {
      // Split the points based on the "widest" bounding box dimension.
      const auto cut_dim = argmax_value(actual_bbox.span());
      const auto cut_value = actual_bbox.clip(bbox.center())[cut_dim];
      const auto pivot = _partition_subtree(left, right, cut_dim, cut_value);
      TIT_ASSERT(left <= pivot && pivot <= right, "Invalid pivot.");
      node->cut_dim = cut_dim;
      auto build_left_subtree = [=, this] {
        // Build left subtree and update it's bounding box.
        auto left_bbox = bbox;
        left_bbox.high[cut_dim] = cut_value;
        node->left_subtree = _build_subtree(left, pivot, left_bbox);
        node->cut_left = left_bbox.high[cut_dim];
      };
      auto build_right_subtree = [=, this] {
        // Build right subtree and update it's bounding box.
        auto right_bbox = bbox;
        right_bbox.low[cut_dim] = cut_value;
        node->right_subtree = _build_subtree(pivot, right, right_bbox);
        node->cut_right = right_bbox.low[cut_dim];
      };
      // TODO: execute as tasks!
      build_left_subtree(), build_right_subtree();
    }
    bbox = actual_bbox;
    return node;
  }

  // Partition the K-dimensional subtree points.
  constexpr auto _partition_subtree(size_t left, size_t right, //
                                    size_t cut_dim, Real cut_value) noexcept
      -> size_t {
    TIT_ASSERT(right <= _point_perm.size(), "`right` index is out of range.");
    TIT_ASSERT(left < right, "`left` index is out of range.");
    const size_t middle = avg(left, right);
    const auto left_iter = _point_perm.begin() + left,
               right_iter = _point_perm.begin() + right;
    // Shift the points that are to the left of the splititng plane to the
    // front of the list.
    const auto on_the_left = [&](size_t index) {
      return _points[index][cut_dim] < cut_value;
    };
    auto pivot_iter = std::partition(left_iter, right_iter, on_the_left);
    auto pivot = pivot_iter - _point_perm.begin();
    if (pivot > middle) return pivot;
    // Now at the pivot are the points which are on the splitting plane
    // or to the right of it. Now shift to the pivot points that are on the
    // plane and find the optimal pivot value to maintain the tree balanced.
    const auto on_the_plane = [&](size_t index) {
      return _points[index][cut_dim] == cut_value;
    };
    pivot_iter = std::partition(pivot_iter, right_iter, on_the_plane);
    pivot = pivot_iter - _point_perm.begin();
    if (pivot < middle) return pivot;
    return middle;
  }

  /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

public:

  /** Find the points within the radius to the given point. */
  template<class OutputIterator>
  constexpr auto search(Point search_point, Real search_radius,
                        OutputIterator output) const noexcept
      -> OutputIterator {
    TIT_ASSERT(search_radius > 0.0, "Search radius should be positive.");
    TIT_ASSERT(_root_node, "Tree was not built.");
    // Compute distance from the query point to the root bounding box
    // per each dimension. (By "dist" square distances are ment.)
    const auto dists = pow2(search_point - _root_bbox.clip(search_point));
    const auto search_dist = pow2(search_radius);
    // Do the actual search.
    _search_subtree(_root_node, dists, search_point, search_dist, output);
    return output;
  }

private:

  // Search for the point neighbours in the K-dimensional subtree.
  // Parameters are passed by references in order to minimize stack usage.
  template<class OutputIterator>
  constexpr void _search_subtree(const KDTreeNode* node, Point dists,
                                 const Point& search_point, Real search_dist,
                                 OutputIterator& output) const noexcept {
    // Is leaf node reached?
    if (node->left_subtree == nullptr) {
      TIT_ASSERT(node->right_subtree == nullptr, "Invalid leaf node.");
      // Iterate through the points.
      for (size_t i : std::views::iota(node->leaf_left, node->leaf_right)) {
        const auto point_index = _point_perm[i];
        const auto dist = norm2(search_point - _points[point_index]);
        if (dist < search_dist) *output++ = point_index;
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
      _search_subtree(first_node, dists, search_point, search_dist, output);
      // Search in the second subtree (if it not too far).
      dists[cut_dim] = cut_dist;
      const auto dist = sum(dists);
      if (dist < search_dist) {
        _search_subtree(second_node, dists, search_point, search_dist, output);
      }
    }
  }

}; // class KDTree

// Wrap a viewable range into a view on construction.
template<class Points, class... Args>
KDTree(Points&&, Args...) -> KDTree<std::views::all_t<Points>>;

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit
