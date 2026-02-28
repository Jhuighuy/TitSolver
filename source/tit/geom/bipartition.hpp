/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <functional>
#include <ranges>
#include <span>
#include <utility>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/vec.hpp"
#include "tit/geom/point_range.hpp"

namespace tit::geom {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Coordinate bisection function.
class CoordBisection final {
public:

  /// Bisect the points along axis, spaned by the given coordinate axis.
  template<point_range Points>
  constexpr auto operator()(Points&& points,
                            std::span<size_t> perm,
                            point_range_num_t<Points> pivot,
                            size_t axis,
                            bool reverse = false) const
      -> std::pair<std::span<size_t>, std::span<size_t>> {
    TIT_ASSERT(axis < point_range_dim_v<Points>, "Axis is out of range!");
    std::span<size_t> right_perm;
    if (reverse) {
      right_perm = std::ranges::partition(
          perm,
          std::bind_back(std::greater{}, pivot),
          [&points, axis](size_t index) { return points[index][axis]; });
    } else {
      right_perm = std::ranges::partition(
          perm,
          std::bind_back(std::less{}, pivot),
          [&points, axis](size_t index) { return points[index][axis]; });
    }
    return {
        {std::ranges::begin(perm), std::ranges::begin(right_perm)},
        right_perm,
    };
  }

}; // class CoordBisection

/// Coordinate bisection.
inline constexpr CoordBisection coord_bisection{};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Direction bisection function.
class DirBisection final {
public:

  /// Bisect the points along axis, spaned by the given direction.
  template<point_range Points>
  constexpr auto operator()(Points&& points,
                            std::span<size_t> perm,
                            point_range_num_t<Points> pivot,
                            const point_range_vec_t<Points>& dir,
                            bool reverse = false) const
      -> std::pair<std::span<size_t>, std::span<size_t>> {
    std::span<size_t> right_perm;
    if (reverse) {
      right_perm = std::ranges::partition(
          perm,
          std::bind_back(std::greater{}, pivot),
          [&points, &dir](size_t index) { return dot(points[index], dir); });
    } else {
      right_perm = std::ranges::partition(
          perm,
          std::bind_back(std::less{}, pivot),
          [&points, &dir](size_t index) { return dot(points[index], dir); });
    }
    return {
        {std::ranges::begin(perm), std::ranges::begin(right_perm)},
        right_perm,
    };
  }

}; // class DirBisection

/// Direction bisection.
inline constexpr DirBisection dir_bisection{};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Coordinate median split function.
class CoordMedianSplit final {
public:

  /// Split the points into two parts by the median along the
  /// given coordinate axis.
  template<point_range Points>
  constexpr auto operator()(Points&& points,
                            std::span<size_t> perm,
                            size_t median_index,
                            size_t axis) const
      -> std::pair<std::span<size_t>, std::span<size_t>> {
    TIT_ASSERT(axis < point_range_dim_v<Points>, "Axis is out of range!");
    const auto median = perm.begin() + static_cast<ssize_t>(median_index);
    std::ranges::nth_element(
        perm,
        median,
        std::less{},
        [&points, axis](size_t index) { return points[index][axis]; });
    return {
        {std::ranges::begin(perm), median},
        {median, std::ranges::end(perm)},
    };
  }

  /// Split the points into two parts by the median along the longest axis
  /// of the points bounding box.
  template<point_range Points>
  constexpr auto operator()(Points&& points,
                            std::span<size_t> perm,
                            size_t median_index) const
      -> std::pair<std::span<size_t>, std::span<size_t>> {
    const auto box = compute_bbox(points, perm);
    const auto axis = max_value_index(box.extents());
    return (*this)(points, perm, median_index, axis);
  }

}; // class CoordMedianSplit

/// Coordinate median split.
inline constexpr CoordMedianSplit coord_median_split{};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Directional median split function.
class DirMedianSplit final {
public:

  /// Split the points into two parts by the median along the axis, spaned by
  /// the given direction.
  template<point_range Points>
  constexpr auto operator()(Points&& points,
                            std::span<size_t> perm,
                            size_t median_index,
                            const point_range_vec_t<Points>& dir) const
      -> std::pair<std::span<size_t>, std::span<size_t>> {
    const auto median = perm.begin() + static_cast<ssize_t>(median_index);
    std::ranges::nth_element( //
        perm,
        median,
        [&points, &dir](size_t i, size_t j) {
          return dot(points[i] - points[j], dir) < 0;
        });
    return {
        {std::ranges::begin(perm), median},
        {median, std::ranges::end(perm)},
    };
  }

}; // class DirMedianSplit

/// Directional median split.
inline constexpr DirMedianSplit dir_median_split{};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Inertial median split function.
class InertialMedianSplit final {
public:

  /// Split the points into two parts by the median along the axis, spaned by
  /// "largest" inertial axis of the point cloud. If the inertia analysis fails,
  /// the given fallback vector is used instead.
  template<point_range Points>
  constexpr auto operator()(Points&& points,
                            std::span<size_t> perm,
                            size_t median_index,
                            const point_range_vec_t<Points>& fallback_dir =
                                unit(point_range_vec_t<Points>{})) const
      -> std::pair<std::span<size_t>, std::span<size_t>> {
    const auto dir =
        compute_largest_inertia_axis(points, perm).value_or(fallback_dir);
    return dir_median_split(points, perm, median_index, dir);
  }

}; // class InertialMedianSplit

/// Inertial median split.
inline constexpr InertialMedianSplit inertial_median_split{};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::geom
