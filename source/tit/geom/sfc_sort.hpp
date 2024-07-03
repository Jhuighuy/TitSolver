/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <ranges>
#include <span>
#include <utility>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/par.hpp"
#include "tit/core/profiler.hpp"
#include "tit/core/vec.hpp"

#include "tit/geom/bbox.hpp"

namespace tit::geom {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Morton space filling curve (Z curve) spatial sorting.
template<std::ranges::view Points>
  requires std::ranges::sized_range<Points> &&
           std::ranges::random_access_range<Points> &&
           is_vec_v<std::ranges::range_value_t<Points>>
class MortonCurveSort final {
public:

  /// Point type.
  using Vec = std::ranges::range_value_t<Points>;

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Initialize and build Morton SFC curve ordering.
  explicit MortonCurveSort(Points points) : points_{std::move(points)} {
    TIT_PROFILE_SECTION("MortonCurveSort::MortonCurveSort()");
    build_();
  }

  /// Get permutation.
  constexpr auto perm() const noexcept -> std::span<const size_t> {
    return perm_;
  }

  /// Get inverse permutation (ordering).
  constexpr auto iperm() const -> std::vector<size_t> {
    std::vector<size_t> iperm(perm_.size());
    for (const auto [i, p] : std::views::enumerate(perm_)) iperm[p] = i;
    return iperm;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  // Build the permutation recursively.
  void build_() {
    // Initialize the identity permutation.
    perm_.resize(std::size(points_));
    std::ranges::copy(std::views::iota(size_t{0}, perm_.size()), perm_.begin());

    // Compute bounding box.
    //
    /// @todo Introduce a helper function for this computation.
    auto bbox = BBox{points_[0]};
    for (const auto& p : points_ | std::views::drop(1)) bbox.expand(p);

    // Compute the root bounding box and build permutation.
    par::TaskGroup tasks{};
    constexpr size_t InitAxis = 0;
    partition_<InitAxis>(tasks, bbox, perm_);
    tasks.wait();
  }

  // Recursively partition the points along the Morton curve.
  template<size_t Axis>
    requires (Axis < vec_dim_v<Vec>)
  void partition_(par::TaskGroup& tasks,
                  const BBox<Vec>& bbox,
                  std::span<size_t> perm) {
    if (perm.size() <= 1) return;

    // Split permutation along the current axis.
    const auto center_coord = bbox.center()[Axis];
    const auto [left_bbox, right_bbox] = bbox.split(Axis, center_coord);
    const std::span right_perm =
        std::ranges::partition(perm, [center_coord, this](size_t index) {
          return points_[index][Axis] <= center_coord;
        });
    const std::span left_perm(perm.begin(), right_perm.begin());

    // Recursively split the parts along the next axis.
    constexpr auto NextAxis = (Axis + 1) % vec_dim_v<Vec>;
    tasks.run(is_async_(left_perm), [left_bbox, left_perm, &tasks, this] {
      partition_<NextAxis>(tasks, left_bbox, left_perm);
    });
    tasks.run(is_async_(right_perm), [right_bbox, right_perm, &tasks, this] {
      partition_<NextAxis>(tasks, right_bbox, right_perm);
    });
  }

  // Should the sorting be done in parallel?
  static constexpr auto is_async_(std::span<size_t> perm) noexcept -> bool {
    static constexpr size_t TooSmall = 50; // Empirical value.
    return perm.size() >= TooSmall;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  Points points_;
  std::vector<size_t> perm_;

}; // class MortonCurveSort

// Wrap a viewable range into a view on construction.
template<class Points>
MortonCurveSort(Points&&) -> MortonCurveSort<std::views::all_t<Points>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::geom
