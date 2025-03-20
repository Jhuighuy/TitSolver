/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <ranges>
#include <vector>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-template-arg-list-after-template-kw"
#endif
#include <boost/geometry.hpp>
#include <boost/geometry/algorithms/buffer.hpp>
#include <boost/geometry/algorithms/difference.hpp>
#include <boost/geometry/algorithms/intersection.hpp>
#include <boost/geometry/algorithms/sym_difference.hpp>
#include <boost/geometry/algorithms/union.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include "tit/core/basic_types.hpp"
#include "tit/core/vec.hpp"

namespace tit::geom {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace bg = boost::geometry;

// Template alias for Boost.Geometry polygon type
template<class Num>
using BoostPolygon =
    bg::model::polygon<Vec<Num, 2>, false, false>; // Counter-clockwise, open
                                                   // polygon

// Polygon: A polygon with holes
template<class Num>
class Polygon {
public:

  explicit Polygon(const std::vector<Vec<Num, 2>>& boundary,
                   const std::vector<std::vector<Vec<Num, 2>>>& holes = {}) {
    bg::assign_points(polygon, boundary);
    for (const auto& hole : holes) {
      polygon.inners().emplace_back();
      bg::assign_points(polygon.inners().back(), hole);
    }
    bg::correct(polygon);
  }

  // CSG Operations
  static auto union_op(const Polygon& a, const Polygon& b) -> Polygon {
    std::vector<BoostPolygon<Num>> result;
    bg::union_(a.polygon, b.polygon, result);
    return !result.empty() ? Polygon(result[0]) : Polygon({});
  }

  static auto intersection_op(const Polygon& a, const Polygon& b) -> Polygon {
    std::vector<BoostPolygon<Num>> result;
    bg::intersection(a.polygon, b.polygon, result);
    return !result.empty() ? Polygon(result[0]) : Polygon({});
  }

  static auto difference_op(const Polygon& a, const Polygon& b) -> Polygon {
    std::vector<BoostPolygon<Num>> result;
    bg::difference(a.polygon, b.polygon, result);
    return !result.empty() ? Polygon(result[0]) : Polygon({});
  }

  static auto xor_op(const Polygon& a, const Polygon& b) -> Polygon {
    std::vector<BoostPolygon<Num>> result;
    bg::sym_difference(a.polygon, b.polygon, result);
    return !result.empty() ? Polygon(result[0]) : Polygon({});
  }

  // Extrusion (expanding by a given distance)
  auto extrude(Num distance) const -> Polygon {
    std::vector<BoostPolygon<Num>> result;
    bg::strategy::buffer::distance_symmetric<Num> strategy(distance);
    bg::buffer(polygon, result, strategy);
    return !result.empty() ? Polygon(result[0]) : Polygon({});
  }

  // Check if a point is inside the polygon
  auto contains(const Vec<Num, 2>& point) const -> bool {
    return bg::within(point, polygon);
  }

  // Get range of nodes (points in the boundary)
  auto nodes() const {
    return polygon.outer() | std::views::all;
  }

  // Get range of edges (pairs of consecutive points in the boundary)
  auto edges() const {
    return std::views::iota(0UZ, polygon.outer().size() - 1) |
           std::views::transform([this](size_t i) {
             return std::pair{polygon.outer()[i], polygon.outer()[i + 1]};
           });
  }

private:

  BoostPolygon<Num> polygon;

}; // class Polygon

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::geom
