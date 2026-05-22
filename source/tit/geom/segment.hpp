/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <array>

#include "tit/core/math.hpp"
#include "tit/core/vec.hpp"
#include "tit/geom/bbox.hpp"
#include "tit/geom/bsphere.hpp"

namespace tit::geom {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Segment.
template<class Vec>
class Segment final {
public:

  /// Numeric type of the triangle.
  using Num = vec_num_t<Vec>;

  /// Construct a segment with the given vertices.
  constexpr explicit Segment(const Vec& a, const Vec& b) : verts_{a, b} {}

  /// Vertices.
  constexpr auto verts() const noexcept -> const std::array<Vec, 2>& {
    return verts_;
  }

  /// First vertex.
  constexpr auto a() const noexcept -> const Vec& {
    return verts_[0];
  }

  /// Second vertex.
  constexpr auto b() const noexcept -> const Vec& {
    return verts_[1];
  }

  /// Direction of the segment (unnormalized).
  constexpr auto ba() const noexcept -> Vec {
    return b() - a();
  }

  /// Bounding box of the segment.
  constexpr auto box() const noexcept -> BBox<Vec> {
    return BBox{a()}.expand(b());
  }

  /// Center of the segment.
  constexpr auto center() const noexcept -> Vec {
    return avg(a(), b());
  }

  /// Normal vector of the segment times length.
  constexpr auto wnormal() const noexcept -> Vec {
    return cross(ba());
  }

  /// Normal vector of the segment.
  constexpr auto normal() const noexcept -> Vec {
    return normalize(wnormal());
  }

  /// Length of the segment.
  constexpr auto length() const noexcept -> Num {
    return norm(wnormal());
  }

  /// Generalized winding number contribution around the given point.
  constexpr auto winding_number(const Vec& point) const noexcept -> Num {
    const auto ap = a() - point;
    const auto bp = b() - point;
    return atan2(det(ap, bp), dot(ap, bp)) / unit_sphere_area_v<2, Num>;
  }

  /// Project a segment into the coordinate system induced by the segment
  /// plane and with the given point as the origin.
  constexpr auto project(const Vec& origin) const noexcept
      -> std::array<Num, 2> {
    const auto& [... vs] = verts();
    const auto e = normalize(ba());
    return {dot(vs - origin, e)...};
  }

  /// Find the point inside of the segment that is closest to @p point.
  constexpr auto clamp(const Vec& point) const noexcept -> Vec {
    const auto len_sqr = norm2(ba());
    if (is_tiny(len_sqr)) return a();

    const auto t = dot(point - a(), ba()) / len_sqr;

    // Region 1: Closest to vertex A.
    if (t < Num{0}) return a();

    // Region 2: Closest to vertex B.
    if (t > Num{1}) return b();

    // Region 3: Closest point is inside the segment.
    return a() + t * ba();
  }

  /// Determine if the segment intersects the sphere.
  constexpr auto intersects(const BSphere<Vec>& sphere) const noexcept -> bool {
    return sphere.box().intersects(box()) &&
           sphere.contains(clamp(sphere.center()));
  }

private:

  std::array<Vec, 2> verts_;

}; // class Segment

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::geom
