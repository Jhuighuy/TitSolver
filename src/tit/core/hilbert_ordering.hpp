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
#include <utility>
#include <vector>

#include "tit/core/assert.hpp"
#include "tit/core/bbox.hpp"
#include "tit/core/par.hpp"
#include "tit/core/types.hpp"
#include "tit/core/vec.hpp"

#include <oneapi/tbb/task_group.h>

namespace tit {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** Z-curve spatial ordering.
\******************************************************************************/
template<std::ranges::view Points>
  requires std::ranges::sized_range<Points> &&
           std::ranges::random_access_range<Points> &&
           is_vec_v<std::ranges::range_value_t<Points>>
class ZCurveOrdering {
public:

  /** Point type. */
  using Point = std::ranges::range_value_t<Points>;
  /** Bounding box type. */
  using PointBBox = decltype(BBox(std::declval<Point>()));
  /** Numeric type used by the point type. */
  static constexpr auto Dim = vec_dim_v<Point>;

private:

  Points _points;
  std::vector<size_t> _point_perm;

public:

  /** Initialze and build the K-dimensional tree.
   ** @param max_leaf_size Maximum amount of points in the leaf node. */
  constexpr explicit ZCurveOrdering(Points points)
      : _points{std::move(points)} {
    if (std::ranges::empty(_points)) return;
    // Initialize identity points permutation.
    const auto size = std::ranges::size(_points);
    _point_perm.resize(size);
    std::ranges::copy(std::views::iota(size_t{0}, size), _point_perm.begin());
    // Compute bounding box.
    auto bbox = BBox{_points[0]};
    for (const auto& p : _points | std::views::drop(1)) bbox.update(p);
    // Compute the root bounding box and build ordering.
    _partition(_point_perm.data(), //
               _point_perm.data() + _point_perm.size(), bbox);
  }

  void GetHilbertElementOrdering(std::vector<size_t>& ordering) {
    ordering = std::move(_point_perm);
  }

private:

  // Build the K-dimensional subtree.
  // On the input `bbox` contains a rough estimate that was guessed by the
  // caller. On return it contains an exact bounding box of the subtree.
  constexpr void _partition(size_t* first, size_t* last,
                            PointBBox bbox) noexcept {
    TIT_ASSERT(first < last, "Invalid point iterators.");
    if (last - first <= 1) return;
    const auto center = bbox.center();
    if constexpr (Dim == 2) {
      const auto in_upper_part = [&](size_t index) {
        return _points[index][1] > center[1];
      };
      const auto to_the_left = [&](size_t index) {
        return _points[index][0] < center[0];
      };
      // Split subtree vertically.
      const auto [lower_bbox, upper_bbox] = bbox.split(center, 1);
      const auto upper = first;
      const auto lower = std::partition(first, last, in_upper_part);
      // Split upper part horizontally.
      const auto [upper_left_bbox, //
                  upper_right_bbox] = upper_bbox.split(center, 0);
      const auto upper_left = upper;
      const auto upper_right = std::partition(upper, lower, to_the_left);
      // Split lower part horizontally.
      const auto [lower_left_bbox, //
                  lower_right_bbox] = lower_bbox.split(center, 0);
      const auto lower_left = lower;
      const auto lower_right = std::partition(lower, last, to_the_left);
      // Recursively build quadrants.
      par::invoke(
          [=, this] { _partition(upper_left, upper_right, upper_left_bbox); },
          [=, this] { _partition(upper_right, lower_left, upper_right_bbox); },
          [=, this] { _partition(lower_left, lower_right, lower_left_bbox); },
          [=, this] { _partition(lower_right, last, lower_right_bbox); });
    }
  }

}; // class ZCurveOrdering

// Wrap a viewable range into a view on construction.
template<class Points, class... Args>
ZCurveOrdering(Points&&, Args...) -> ZCurveOrdering<std::views::all_t<Points>>;

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

template<std::ranges::view Points>
class HilbertOrdering {
public:

  struct HilbertCmp {
    int coord;
    bool dir;
    Points points;
    double mid;

    HilbertCmp(int coord, bool dir, Points points, double mid)
        : coord(coord), dir(dir), points(points), mid(mid) {}

    bool operator()(int i) const {
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
    {
      p2 = std::partition(p0, p4, //
                          HilbertCmp(coord1, dir1, points, xmid));
      // tbb::task_group g{};
      // g.run([=, &p1] {
      p1 = std::partition(p0, p2, //
                          HilbertCmp(coord2, dir2, points, ymid));
      //});
      // g.run([=, &p3] {
      p3 = std::partition(p2, p4, //
                          HilbertCmp(coord2, !dir2, points, ymid));
      //});
      // g.wait();
    }

    {
      tbb::task_group g{};
      // if (p1 != p4) {
      g.run([=] {
        HilbertSort2D(coord2, dir2, dir1, points, p0, p1, ymin, xmin, ymid,
                      xmid);
      });
      //}
      // if (p1 != p0 || p2 != p4) {
      g.run([=] {
        HilbertSort2D(coord1, dir1, dir2, points, p1, p2, xmin, ymid, xmid,
                      ymax);
      });
      //}
      // if (p2 != p0 || p3 != p4) {
      g.run([=] {
        HilbertSort2D(coord1, dir1, dir2, points, p2, p3, xmid, ymid, xmax,
                      ymax);
      });
      //}
      // if (p3 != p0) {
      g.run([=] {
        HilbertSort2D(coord2, !dir2, !dir1, points, p3, p4, ymid, xmax, ymin,
                      xmid);
      });
      //}
      g.wait();
    }
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

  Points points;

  HilbertOrdering(Points points) : points{points} {}

  void GetHilbertElementOrdering(std::vector<size_t>& ordering) {
    /** Point type. */
    using Point = std::ranges::range_value_t<Points>;
    using PointBBox = decltype(BBox(std::declval<Point>()));

    Point min, max;
    { // GetBoundingBox(min, max);
      PointBBox bbox{points[0]};
      for (size_t i = 1; i < points.size(); ++i) bbox.update(points[i]);
      min = bbox.low, max = bbox.high;
    }

    std::vector<size_t> indices(points.size());

    // calculate element centers
    for (int i = 0; i < points.size(); i++) {
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
};

} // namespace tit
