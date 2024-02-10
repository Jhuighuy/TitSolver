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
#include "tit/core/profiler.hpp"
#include "tit/core/vec.hpp"

#include "tit/par/task_group.hpp"

#include "tit/geom/bbox.hpp"

namespace tit::geom {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Morton space filling curve spatial ordering.
template<std::ranges::view Points>
  requires std::ranges::sized_range<Points> &&
           std::ranges::random_access_range<Points> &&
           is_vec_v<std::ranges::range_value_t<Points>>
class MortonCurveOrdering final {
public:

  /// Point type.
  using Point = std::ranges::range_value_t<Points>;

  /// Bounding box type.
  using PointBBox = bbox_t<Point>;

  /// Numeric type used by the point type.
  static constexpr auto Dim = vec_dim_v<Point>;

  /// Initialize and build Morton SFC curve ordering.
  explicit MortonCurveOrdering(Points points) : points_{std::move(points)} {
    TIT_PROFILE_SECTION("MortonCurveOrdering::ctor()");
    if (std::ranges::empty(points_)) return;
    // Initialize identity point ordering.
    indices_.resize(std::ranges::size(points_));
    std::ranges::copy(std::views::iota(size_t{0}, indices_.size()),
                      indices_.begin());
    // Compute bounding box.
    auto bbox = BBox{points_[0]};
    for (auto const& p : points_ | std::views::drop(1)) bbox.update(p);
    // Compute the root bounding box and build ordering.
    par::TaskGroup tasks{};
    build_indices_<0>(tasks, bbox, indices_);
  }

  void GetHilbertElementOrdering(std::vector<size_t>& ordering) {
    ordering = std::move(indices_);
  }

private:

  // Compute the bounding box.
  auto build_bbox_() noexcept {
    auto bbox = BBox{points_[0]};
    for (auto const& p : points_ | std::views::drop(1)) bbox.update(p);
    return bbox;
  }

  // Build permutation using a divide and conquire approach.
  template<size_t Axis>
    requires (Axis < Dim)
  void build_indices_(par::TaskGroup& tasks, //
                      PointBBox bbox, std::span<size_t> range) noexcept {
    if (range.size() <= 1) return;
    auto const center = bbox.center();
    // Split ordering along the given axis.
    auto const [left_bbox, right_bbox] = bbox.split(Axis, center);
    auto const is_left = [value = center[Axis], this](size_t index) {
      return points_[index][Axis] <= value;
    };
    std::span const right_range(std::ranges::partition(range, is_left));
    std::span const left_range(range.begin(), right_range.begin());
    // Recursively split the head and tail along the next axis.
    constexpr auto NextAxis = (Axis + 1) % Dim;
    tasks.run(is_async_(left_range), [=, &tasks, this] {
      build_indices_<NextAxis>(tasks, left_bbox, left_range);
    });
    tasks.run(is_async_(right_range), [=, &tasks, this] {
      build_indices_<NextAxis>(tasks, right_bbox, right_range);
    });
  }

  // Should the ordering be done in parallel?
  static constexpr auto is_async_(std::span<size_t> range) noexcept -> bool {
    static constexpr size_t TooSmall = 50;
    return range.size() >= TooSmall;
  }

  Points points_;
  std::vector<size_t> indices_;

}; // class MortonCurveOrdering

// Wrap a viewable range into a view on construction.
template<class Points, class... Args>
MortonCurveOrdering(Points&&, Args...)
    -> MortonCurveOrdering<std::views::all_t<Points>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/******************************************************************************\
 ** Hilbert curve spatial ordering.
\******************************************************************************/
template<std::ranges::view Points>
  requires std::ranges::sized_range<Points> &&
           std::ranges::random_access_range<Points> &&
           is_vec_v<std::ranges::range_value_t<Points>>
class HilbertCurveOrdering {
public:

  // See:
  // https://en.wikipedia.org/wiki/Hilbert_curve#/media/File:Hilbert_curve_production_rules!.svg

  explicit HilbertCurveOrdering(Points _points) : points{_points} {}

  struct HilbertCmp {
    // NOLINTBEGIN(*-non-private-member-variables-in-classes)
    int coord;
    bool dir;
    Points points;
    double mid;
    // NOLINTEND(*-non-private-member-variables-in-classes)

    HilbertCmp(int _coord, bool _dir, Points _points, double _mid)
        : coord(_coord), dir(_dir), points(_points), mid(_mid) {}

    auto operator()(int i) const -> bool {
      return (points[i][coord] < mid) != dir;
    }
  };

  template<class Iter>
  static void HilbertSort2D(int coord1, // major coordinate to sort points by
                            bool dir1,  // sort coord1 ascending/descending?
                            bool dir2,  // sort coord2 ascending/descending?
                            Points points, Iter beg, Iter end, //
                            double xmin, double ymin, double xmax,
                            double ymax) {
    if (end - beg <= 1) {
      return;
    }

    double xmid = (xmin + xmax) * 0.5;
    double ymid = (ymin + ymax) * 0.5;

    int coord2 = (coord1 + 1) % 2; // the 'other' coordinate

    // sort (partition) points into four quadrants
    Iter p0 = beg, p1, p2, p3, p4 = end;
    p2 = std::partition(p0, p4, HilbertCmp(coord1, dir1, points, xmid));
    p1 = std::partition(p0, p2, HilbertCmp(coord2, dir2, points, ymid));
    p3 = std::partition(p2, p4, HilbertCmp(coord2, !dir2, points, ymid));

    HilbertSort2D(coord2, dir2, dir1, points, p0, p1, ymin, xmin, ymid, xmid);
    HilbertSort2D(coord1, dir1, dir2, points, p1, p2, xmin, ymid, xmid, ymax);
    HilbertSort2D(coord1, dir1, dir2, points, p2, p3, xmid, ymid, xmax, ymax);
    HilbertSort2D(coord2, !dir2, !dir1, points, p3, p4, ymid, xmax, ymin, xmid);
  }

  template<class Iter>
  static void HilbertSort3D(int coord1, bool dir1, bool dir2, bool dir3,
                            Points& points, Iter beg, Iter end, double xmin,
                            double ymin, double zmin, //
                            double xmax, double ymax, double zmax) {
    if (end - beg <= 1) {
      return;
    }

    double xmid = (xmin + xmax) * 0.5;
    double ymid = (ymin + ymax) * 0.5;
    double zmid = (zmin + zmax) * 0.5;

    int coord2 = (coord1 + 1) % 3;
    int coord3 = (coord1 + 2) % 3;

    // sort (partition) points into eight octants
    auto p0 = beg, p8 = end;
    auto p4 = std::partition(p0, p8, //
                             HilbertCmp(coord1, dir1, points, xmid));
    auto p2 = std::partition(p0, p4, //
                             HilbertCmp(coord2, dir2, points, ymid));
    auto p6 = std::partition(p4, p8, //
                             HilbertCmp(coord2, !dir2, points, ymid));
    auto p1 = std::partition(p0, p2, //
                             HilbertCmp(coord3, dir3, points, zmid));
    auto p3 = std::partition(p2, p4, //
                             HilbertCmp(coord3, !dir3, points, zmid));
    auto p5 = std::partition(p4, p6, //
                             HilbertCmp(coord3, dir3, points, zmid));
    auto p7 = std::partition(p6, p8, //
                             HilbertCmp(coord3, !dir3, points, zmid));

    if (p1 != p8) {
      HilbertSort3D(coord3, dir3, dir1, dir2, points, p0, p1, zmin, xmin, ymin,
                    zmid, xmid, ymid);
    }
    if (p1 != p0 || p2 != p8) {
      HilbertSort3D(coord2, dir2, dir3, dir1, points, p1, p2, ymin, zmid, xmin,
                    ymid, zmax, xmid);
    }
    if (p2 != p0 || p3 != p8) {
      HilbertSort3D(coord2, dir2, dir3, dir1, points, p2, p3, ymid, zmid, xmin,
                    ymax, zmax, xmid);
    }
    if (p3 != p0 || p4 != p8) {
      HilbertSort3D(coord1, dir1, !dir2, !dir3, points, p3, p4, xmin, ymax,
                    zmid, xmid, ymid, zmin);
    }
    if (p4 != p0 || p5 != p8) {
      HilbertSort3D(coord1, dir1, !dir2, !dir3, points, p4, p5, xmid, ymax,
                    zmid, xmax, ymid, zmin);
    }
    if (p5 != p0 || p6 != p8) {
      HilbertSort3D(coord2, !dir2, dir3, !dir1, points, p5, p6, ymax, zmid,
                    xmax, ymid, zmax, xmid);
    }
    if (p6 != p0 || p7 != p8) {
      HilbertSort3D(coord2, !dir2, dir3, !dir1, points, p6, p7, ymid, zmid,
                    xmax, ymin, zmax, xmid);
    }
    if (p7 != p0) {
      HilbertSort3D(coord3, !dir3, !dir1, dir2, points, p7, p8, zmid, xmax,
                    ymin, zmin, xmid, ymid);
    }
  }

  void GetHilbertElementOrdering(std::vector<size_t>& ordering) {
    TIT_PROFILE_SECTION("HilbertSort3D::ctor()");
    /** Point type. */
    using Point = std::ranges::range_value_t<Points>;
    using PointBBox = decltype(BBox(std::declval<Point>()));

    Point min, max;
    { // GetBoundingBox(min, max);
      PointBBox bbox{points[0]};
      for (size_t i = 1; i < points.size(); ++i) bbox.update(points[i]);
      min = bbox.low(), max = bbox.high();
    }

    std::vector<size_t> indices(points.size());

    // calculate element centers
    for (size_t i = 0; i < points.size(); i++) {
      indices[i] = i;
    }

    auto spaceDim = dim(points[0]);
    if (spaceDim == 1) {
      // indices.Sort([&](int a, int b) { return points[3 * a] < points[3 * b];
      // });
    } else if (spaceDim == 2) {
      // recursively partition the points in 2D
      HilbertSort2D(0, false, false, points,        //
                    indices.begin(), indices.end(), //
                    min[0], min[1], max[0], max[1]);
    } else {
      // recursively partition the points in 3D
      HilbertSort3D(0, false, false, false, points, //
                    indices.begin(), indices.end(), //
                    min[0], min[1], min[2], max[0], max[1], max[2]);
    }

    // return ordering in the format required by ReorderElements
    ordering = std::move(indices);
  }

private:

  Points points;
};

// Wrap a viewable range into a view on construction.
template<class Points, class... Args>
HilbertCurveOrdering(Points&&, Args...)
    -> HilbertCurveOrdering<std::views::all_t<Points>>;

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit::geom
