/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <iterator>
#include <ranges>
#include <span>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/func.hpp"
#include "tit/core/profiler.hpp"
#include "tit/core/type.hpp"
#include "tit/core/vec.hpp"
#include "tit/geom/bipartition.hpp"
#include "tit/geom/point_range.hpp"
#include "tit/par/atomic.hpp"
#include "tit/par/task_group.hpp"

namespace tit::geom {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// K-dimensional tree spatial search index.
template<point_range Points>
  requires std::ranges::view<Points>
class KDTreeIndex final {
public:

  /// Point type.
  using Vec = point_range_vec_t<Points>;

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  TIT_MOVE_ONLY(KDTreeIndex);

  /// Move-construct from another index.
  constexpr KDTreeIndex(KDTreeIndex&&) noexcept = default;

  /// Move-assign from another index.
  constexpr auto operator=(KDTreeIndex&&) noexcept -> KDTreeIndex& = default;

  /// Destroy the index.
  constexpr ~KDTreeIndex() noexcept = default;

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Index the points for search using a K-dimensional tree.
  explicit KDTreeIndex(Points points) : points_{std::move(points)} {
    if (std::ranges::empty(points_)) return;

    // Initialize the permutation.
    auto perm = iota_perm(points_) | std::ranges::to<std::vector>();

    // Initialize the node storage.
    nodes_.resize(std::ranges::size(points_));
    size_t node_counter = 0;

    // Recursively construct the tree.
    par::TaskGroup tasks{};
    [&my_points = points_, &nodes = nodes_, &node_counter, &tasks](
        this const auto& self,
        std::span<size_t> my_perm) {
      // Allocate the node.
      auto* const node = &nodes[par::fetch_and_add(node_counter, 1)];

      // Quick exit if a single point is left.
      if (my_perm.size() == 1) {
        node->index = my_perm.front();
        return node;
      }

      // Split the points into the roughly equal halves.
      const auto box = compute_bbox(my_points, my_perm);
      node->axis = max_value_index(box.extents());
      const auto median_iter =
          my_perm.begin() + static_cast<ssize_t>(my_perm.size() / 2);
      const auto [left_perm, right_perm] =
          coord_median_split(my_points, my_perm, median_iter, node->axis);
      node->index = *median_iter;

      // Recursively partition the halves.
      tasks.run(is_async_(left_perm), [node, left_perm, self] {
        if (left_perm.empty()) return;
        node->left = self(left_perm);
      });
      tasks.run(is_async_(right_perm), [node, right_perm, self] {
        // Median point is already accounted for, so we need to skip it.
        if (right_perm.size() == 1) return;
        node->right = self(std::span{right_perm}.subspan(1));
      });
      return node;
    }(perm);
    tasks.wait();
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Find the points within the radius to the given point.
  template<std::output_iterator<size_t> OutIter,
           std::predicate<size_t> Pred = AlwaysTrue>
  auto search(const Vec& search_point,
              vec_num_t<Vec> search_radius,
              OutIter out,
              Pred pred = {}) const -> OutIter {
    TIT_ASSERT(search_radius > 0.0, "Search radius should be positive!");
    const auto search_radius_sq = pow2(search_radius);
    const auto* const root_node = &nodes_.front();
    [&search_point, &search_radius_sq, &pred, &points = points_, &out](
        this const auto& self,
        const KDTreeNode_* node) {
      if (node == nullptr) return;
      const auto& [index, axis, left, right] = *node;
      TIT_ASSERT(axis < point_range_dim_v<Points>, "Axis is out of range!");

      // Check if the point is within the search radius.
      const auto& point = points[index];
      if (pred(index) && norm2(point - search_point) < search_radius_sq) {
        *out++ = index;
      }

      // Recursively search the subtrees.
      const auto delta = search_point[axis] - point[axis];
      const auto delta_sq = pow2(delta);
      if (delta < vec_num_t<Vec>{}) {
        self(left);
        if (delta_sq < search_radius_sq) self(right);
      } else {
        self(right);
        if (delta_sq < search_radius_sq) self(left);
      }
    }(root_node);
    return out;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  // Should the building be done in parallel?
  static auto is_async_(std::span<size_t> perm) noexcept -> bool {
    constexpr size_t parallel_threshold = 50; // Empirical value.
    return std::size(perm) >= parallel_threshold;
  }

  struct KDTreeNode_ final {
    size_t index = 0;
    size_t axis = 0;
    const KDTreeNode_* left = nullptr;
    const KDTreeNode_* right = nullptr;
  };

  Points points_;
  std::vector<KDTreeNode_> nodes_;

}; // class KDTreeIndex

// Wrap a viewable range into a view on construction.
template<std::ranges::viewable_range Points, class... Args>
KDTreeIndex(Points&&, Args...) -> KDTreeIndex<std::views::all_t<Points>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// K-dimensional tree based spatial search indexing function.
class KDTreeSearch final {
public:

  /// Index the points for search using a K-dimensional tree.
  template<std::ranges::viewable_range Points>
    requires deduce_constructible_from<KDTreeIndex, Points&&>
  [[nodiscard]] static auto operator()(Points&& points) {
    TIT_PROFILE_SECTION("KDTreeIndex::operator()");
    return KDTreeIndex{std::forward<Points>(points)};
  }

}; // class KDTreeSearch

/// K-dimensional tree based spatial search indexing.
inline constexpr KDTreeSearch kd_tree_indexing{};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::geom
