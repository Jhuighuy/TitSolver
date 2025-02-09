/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <concepts>
#include <iterator>
#include <ranges>
#include <span>
#include <tuple>
#include <utility>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/math.hpp"
#include "tit/core/par/memory_pool.hpp"
#include "tit/core/par/task_group.hpp"
#include "tit/core/profiler.hpp"
#include "tit/core/type_utils.hpp"
#include "tit/core/utils.hpp"
#include "tit/core/vec.hpp"

#include "tit/geom/bbox.hpp"
#include "tit/geom/bipartition.hpp"
#include "tit/geom/point_range.hpp"

namespace tit::geom {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// K-dimensional tree spatial search index.
/// Inspired by nanoflann: https://github.com/jlblancoc/nanoflann
template<point_range Points>
  requires std::ranges::view<Points>
class KDTreeIndex final {
public:

  /// Point type.
  using Vec = std::ranges::range_value_t<Points>;

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Index the points for search using a K-dimensional tree.
  ///
  /// @param max_leaf_size Maximum amount of points in the leaf node.
  explicit KDTreeIndex(Points points, size_t max_leaf_size = 1)
      : points_{std::move(points)}, max_leaf_size_{max_leaf_size} {
    TIT_ASSERT(max_leaf_size_ > 0, "Maximal leaf size should be positive.");
    build_tree_();
  }

  /// Find the points within the radius to the given point.
  template<std::output_iterator<size_t> OutIter,
           std::predicate<size_t> Pred = AlwaysTrue>
  auto search(const Vec& search_point,
              vec_num_t<Vec> search_radius,
              OutIter out,
              Pred pred = {}) const -> OutIter {
    TIT_ASSERT(search_radius > 0.0, "Search radius should be positive.");
    return search_tree_(search_point, search_radius, out, pred);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  // K-dimensional tree node structure.
  union KDTreeNode_ {
    std::span<const size_t> perm{};
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
    struct {
      size_t cut_axis;
      vec_num_t<Vec> cut_left;
      vec_num_t<Vec> cut_right;
      KDTreeNode_* left_subtree;
      KDTreeNode_* right_subtree;
    };
#pragma GCC diagnostic pop
  }; // union KDTreeNode_

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  // Build the K-dimensional tree.
  void build_tree_() {
    if (std::ranges::empty(points_)) return;

    // Initialize identity points permutation.
    perm_ = iota_perm(points_) | std::ranges::to<std::vector>();

    // Compute the root tree node (and bounding box).
    par::TaskGroup tasks{};
    std::tie(root_node_, tree_box_) = build_subtree_(tasks, perm_);
    tasks.wait();
  }

  // Build the K-dimensional subtree.
  auto build_subtree_(par::TaskGroup& tasks, std::span<size_t> perm)
      -> std::pair<KDTreeNode_*, BBox<Vec>> {
    // Compute bounding box.
    const auto box = compute_bbox(points_, perm);

    // Is leaf node reached?
    const auto node = pool_.create();
    if (perm.size() <= max_leaf_size_) {
      // Fill the leaf node and end partitioning.
      node->perm = perm;
      node->left_subtree = node->right_subtree = nullptr;
      return {node, box};
    }

    // Split the points based on the "widest" bounding box dimension.
    const auto cut_axis = max_value_index(box.extents());
    const auto center_coord = box.clamp(box.center())[cut_axis];
    const auto [left_perm, right_perm] =
        coord_bisection(points_, perm, center_coord, cut_axis);

    // Build subtrees.
    node->cut_axis = cut_axis;
    tasks.run(is_async_(left_perm), [left_perm, node, &tasks, this] {
      const auto [left_tree, left_box] = build_subtree_(tasks, left_perm);
      node->left_subtree = left_tree;
      node->cut_left = left_box.high()[node->cut_axis];
    });
    tasks.run(is_async_(right_perm), [right_perm, node, &tasks, this] {
      const auto [right_tree, right_box] = build_subtree_(tasks, right_perm);
      node->right_subtree = right_tree;
      node->cut_right = right_box.low()[node->cut_axis];
    });

    return {node, box};
  }

  // Should the building be done in parallel?
  static auto is_async_(std::span<size_t> perm) noexcept -> bool {
    constexpr size_t parallel_threshold = 50; // Empirical value.
    return std::size(perm) >= parallel_threshold;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  // Search for the point neighbors in the K-dimensional tree.
  //
  /// @todo Search is slow for some reason. Investigate.
  template<std::output_iterator<size_t> OutIter,
           std::predicate<size_t> Pred = AlwaysTrue>
  auto search_tree_(const Vec& search_point,
                    vec_num_t<Vec> search_radius,
                    OutIter out,
                    Pred pred = {}) const -> OutIter {
    // Compute distance from the query point to the root bounding box
    // per each dimension. (By "dist" square distances are meant.)
    const auto search_dist = pow2(search_radius);
    auto dists = pow2(search_point - tree_box_.clamp(search_point));

    // Recursively search the tree.
    TIT_ASSERT(root_node_ != nullptr, "Tree was not built!");
    return search_subtree_(root_node_,
                           dists,
                           search_point,
                           search_dist,
                           out,
                           pred);
  }

  // Search for the point neighbors in the K-dimensional subtree.
  // Parameters are passed by references in order to minimize stack usage.
  template<std::output_iterator<size_t> OutIter,
           std::predicate<size_t> Pred = AlwaysTrue>
  auto search_subtree_(const KDTreeNode_* node,
                       Vec& dists,
                       const Vec& search_point,
                       vec_num_t<Vec> search_dist,
                       OutIter out,
                       Pred pred) const -> OutIter {
    if (node->left_subtree == nullptr) {
      TIT_ASSERT(node->right_subtree == nullptr, "Invalid leaf node!");
      // Collect points within the leaf node.
      out = copy_points_near(points_,
                             node->perm,
                             out,
                             search_point,
                             search_dist,
                             pred);
      return out;
    }

    // Determine which branch should be taken first.
    const auto cut_axis = node->cut_axis;
    const auto [cut_dist, first_node, second_node] = [&] {
      const auto delta_left = search_point[cut_axis] - node->cut_left;
      const auto delta_right = node->cut_right - search_point[cut_axis];
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
    out = search_subtree_(first_node,
                          dists,
                          search_point,
                          search_dist,
                          out,
                          pred);

    // Search in the second subtree (if it not too far).
    if (const auto dist = sum(dists); dist < search_dist) {
      const auto old_cut_dist = std::exchange(dists[cut_axis], cut_dist);
      out = search_subtree_(second_node,
                            dists,
                            search_point,
                            search_dist,
                            out,
                            pred);
      dists[cut_axis] = old_cut_dist;
    }

    return out;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  Points points_;
  size_t max_leaf_size_;
  par::MemoryPool<KDTreeNode_> pool_;
  const KDTreeNode_* root_node_ = nullptr;
  BBox<Vec> tree_box_;
  std::vector<size_t> perm_;

}; // class KDTreeIndex

// Wrap a viewable range into a view on construction.
template<std::ranges::viewable_range Points, class... Args>
KDTreeIndex(Points&&, Args...) -> KDTreeIndex<std::views::all_t<Points>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// K-dimensional tree based spatial search indexing function.
class KDTreeSearch final {
public:

  /// Construct a K-dimensional tree search indexing function.
  ///
  /// @param max_leaf_size Maximum amount of points in the leaf node.
  constexpr explicit KDTreeSearch(size_t max_leaf_size = 1)
      : max_leaf_size_{max_leaf_size} {
    TIT_ASSERT(max_leaf_size_ > 0, "Maximal leaf size should be positive!");
  }

  /// Index the points for search using a K-dimensional tree.
  template<std::ranges::viewable_range Points>
    requires deduce_constructible_from<KDTreeIndex, Points&&, size_t>
  [[nodiscard]] auto operator()(Points&& points) const {
    TIT_PROFILE_SECTION("KDTreeIndex::KDTreeSearch()");
    return KDTreeIndex{std::forward<Points>(points), max_leaf_size_};
  }

private:

  size_t max_leaf_size_;

}; // class KDTreeSearch

/// K-dimensional tree based spatial search indexing.
inline constexpr KDTreeSearch kd_tree_indexing{};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::geom
