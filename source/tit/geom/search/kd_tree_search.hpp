/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <iterator>
#include <memory>
#include <ranges>
#include <span>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/func.hpp"
#include "tit/core/par/task_group.hpp"
#include "tit/core/profiler.hpp"
#include "tit/core/type.hpp"
#include "tit/core/vec.hpp"

#include "tit/geom/bipartition.hpp"
#include "tit/geom/point_range.hpp"

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

  /// Index the points for search using a K-dimensional tree.
  explicit KDTreeIndex(Points points) : points_{std::move(points)} {
    if (std::ranges::empty(points_)) return;

    // Initialize identity points permutation.
    perm_ = iota_perm(points_) | std::ranges::to<std::vector>();

    // Compute the root tree node (and bounding box).
    par::TaskGroup tasks{};
    root_node_ = [&points = points_, &tasks](
                     this const auto& self,
                     std::span<size_t> perm) -> std::unique_ptr<KDTreeNode_> {
      // Fill the leaf node and end partitioning.
      if (perm.empty()) return nullptr;
      if (perm.size() == 1) return std::make_unique<KDTreeNode_>(perm.front());

      // Split the points into the roughly equal halves.
      const auto box = compute_bbox(points, perm);
      const auto axis = max_value_index(box.extents());
      const auto median = perm.begin() + static_cast<ssize_t>(perm.size() / 2);
      const auto [left_perm, right_perm] =
          coord_median_split(points, perm, median, axis);

      // Recursively partition the halves.
      auto node = std::make_unique<KDTreeNode_>(*median, axis);
      tasks.run(is_async_(left_perm), [left_perm, node = node.get(), &self] {
        node->left = self(left_perm);
      });
      tasks.run(is_async_(right_perm), [right_perm, node = node.get(), &self] {
        node->right = self(right_perm);
      });

      return node;
    }(perm_);
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
    [&search_point, &search_radius_sq, &pred, &points = points_, &out](
        this const auto& self,
        const KDTreeNode_* node) -> void {
      if (node == nullptr) return;
      const auto& [index, axis, left, right] = *node;

      // Check if the point is within the search radius.
      const auto& point = points[index];
      if (pred(index) && norm2(point - search_point) < search_radius_sq) {
        *out++ = index;
      }

      // Recursively search the subtrees.
      if (axis != npos) {
        const auto delta = search_point[axis] - point[axis];
        const auto delta_sq = pow2(delta);
        if (delta <= vec_num_t<Vec>{}) {
          self(left.get());
          if (delta_sq <= search_radius_sq) self(right.get());
        } else {
          self(right.get());
          if (delta_sq <= search_radius_sq) self(left.get());
        }
      }
    }(root_node_.get());
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
    size_t index = npos;
    size_t axis = npos;
    std::unique_ptr<const KDTreeNode_> left;
    std::unique_ptr<const KDTreeNode_> right;
  };

  Points points_;
  std::vector<size_t> perm_;
  std::unique_ptr<KDTreeNode_> root_node_;

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
