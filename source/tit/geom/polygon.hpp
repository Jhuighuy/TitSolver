/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <cmath>
#include <initializer_list>
#include <limits>
#include <utility>

#include <clipper2/clipper.core.h>
#include <clipper2/clipper.h>
#include <clipper2/clipper.offset.h>

#include "tit/core/basic_types.hpp"
#include "tit/core/vec.hpp"

namespace tit::geom::poly {

namespace cl = Clipper2Lib;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

using Point = Vec<float64_t, 2>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class Polygon final {
public:

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  explicit Polygon(cl::PathsD paths) noexcept : paths_{std::move(paths)} {}

  explicit Polygon(std::initializer_list<Point> points) noexcept {
    cl::PathD path;
    for (const auto& point : points) path.emplace_back(point[0], point[1]);
    paths_.emplace_back(std::move(path));
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  auto num_paths() const noexcept -> size_t {
    return paths_.size();
  }

  auto path_num_points(size_t path_index) const noexcept -> size_t {
    return paths_[path_index].size();
  }

  auto point(size_t path_index, size_t point_index) const noexcept -> Point {
    return {paths_[path_index][point_index].x,
            paths_[path_index][point_index].y};
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  auto offset(float64_t delta,
              cl::JoinType join_type = cl::JoinType::Miter,
              float64_t miter_limit = 2.0) const -> Polygon {
    return Polygon{cl::SimplifyPaths(cl::InflatePaths(paths_,
                                                      delta,
                                                      join_type,
                                                      cl::EndType::Polygon,
                                                      miter_limit),
                                     delta / 10)};
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  auto subdivide_edges(float64_t max_len) const -> Polygon {
    const auto max_len_sqr = max_len * max_len;

    cl::PathsD new_paths;
    for (const auto& path : paths_) {
      cl::PathD new_path;
      for (size_t i = 0; i < path.size(); ++i) {
        const auto& p1 = path[i];
        const auto& p2 = path[(i + 1) % path.size()];
        new_path.push_back(p1);

        // Check distance
        const auto d2 = distSq(p1, p2);
        if (d2 > max_len_sqr) {
          const auto dist = std::sqrt(d2);
          const auto segments = static_cast<int>(std::ceil(dist / max_len));
          for (int j = 1; j < segments; ++j) {
            const auto t = static_cast<float64_t>(j) / segments;
            const auto nx = p1.x + t * (p2.x - p1.x);
            const auto ny = p1.y + t * (p2.y - p1.y);
            new_path.emplace_back(nx, ny);
          }
        }
      }
      new_paths.push_back(std::move(new_path));
    }

    return Polygon{std::move(new_paths)};
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  auto closest_point(const Point& p_) const noexcept -> Point {
    const cl::PointD query{p_[0], p_[1]};

    cl::PointD best_point;
    auto min_len_sqr = std::numeric_limits<float64_t>::max();
    for (const auto& path : paths_) {
      for (size_t i = 0; i < path.size(); ++i) {
        const auto& p1 = path[i];
        const auto& p2 = path[(i + 1) % path.size()];

        const auto candidate = closestPointOnSegment(query, p1, p2);
        const auto dist = distSq(query, candidate);
        if (dist < min_len_sqr) {
          min_len_sqr = dist;
          best_point = candidate;
        }
      }
    }

    return {best_point.x, best_point.y};
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  static auto distSq(const cl::PointD& a, const cl::PointD& b) noexcept
      -> float64_t {
    return (a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y);
  }

  static auto closestPointOnSegment(const cl::PointD& p,
                                    const cl::PointD& a,
                                    const cl::PointD& b) noexcept
      -> cl::PointD {
    const auto l2 = distSq(a, b);
    if (l2 == 0.0) return a;

    const auto t = ((p.x - a.x) * (b.x - a.x) + (p.y - a.y) * (b.y - a.y)) / l2;

    if (t < 0) return a;
    if (t > 1) return b;

    return {a.x + t * (b.x - a.x), a.y + t * (b.y - a.y)};
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  cl::PathsD paths_;

}; // class Polygon

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::geom::poly
