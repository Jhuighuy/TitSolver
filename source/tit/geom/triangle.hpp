/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <array>

#include "tit/core/vec.hpp"
#include "tit/geom/bbox.hpp"
#include "tit/geom/bsphere.hpp"

namespace tit::geom {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Triangle.
template<class Vec>
class Triangle final {
public:

  /// Numeric type of the triangle.
  using Num = vec_num_t<Vec>;

  /// Construct a triangle with the given vertices.
  constexpr explicit Triangle(const Vec& a, const Vec& b, const Vec& c)
      : verts_{a, b, c} {}

  /// Vertices.
  constexpr auto verts() const noexcept -> const std::array<Vec, 3>& {
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

  /// Third vertex.
  constexpr auto c() const noexcept -> const Vec& {
    return verts_[2];
  }

  /// First edge.
  constexpr auto ba() const noexcept -> Vec {
    return b() - a();
  }

  /// Second edge.
  constexpr auto cb() const noexcept -> Vec {
    return c() - b();
  }

  /// Third edge.
  constexpr auto ca() const noexcept -> Vec {
    return c() - a();
  }

  /// Normal vector of the triangle.
  constexpr auto normal() const noexcept -> Vec
    requires (vec_dim_v<Vec> == 3)
  {
    return normalize(cross(ba(), ca()));
  }

  /// Area of the triangle.
  constexpr auto area() const noexcept -> Num {
    return norm(cross(ba(), ca())) / 2;
  }

  /// Bounding box of the triangle.
  constexpr auto box() const noexcept -> BBox<Vec> {
    return BBox{a()}.expand(b()).expand(c());
  }

  /// Find the point inside of the triangle that is closest to @p point.
  constexpr auto clamp(const Vec& point) const noexcept -> Vec {
    const auto pa = point - a();
    const auto pb = point - b();
    const auto pc = point - c();

    // Region 1: Closest to vertex A.
    const auto d1 = dot(ba(), pa);
    const auto d2 = dot(ca(), pa);
    if (d1 <= Num{0} && d2 <= Num{0}) return a();

    // Region 2: Closest to vertex B.
    const auto d3 = dot(ba(), pb);
    const auto d4 = dot(ca(), pb);
    if (d3 >= Num{0} && d4 <= d3) return b();

    // Region 3: Closest to vertex C.
    const auto d5 = dot(ba(), pc);
    const auto d6 = dot(ca(), pc);
    if (d6 >= Num{0} && d5 <= d6) return c();

    // Region 4: Closest to edge AB.
    const auto vc = d1 * d4 - d3 * d2;
    if (vc <= Num{0} && d1 >= Num{0} && d3 <= Num{0}) {
      const auto t = d1 / (d1 - d3);
      return a() + t * ba();
    }

    // Region 5: Closest to edge AC.
    const auto vb = d5 * d2 - d1 * d6;
    if (vb <= Num{0} && d2 >= Num{0} && d6 <= Num{0}) {
      const auto w = d2 / (d2 - d6);
      return a() + w * ca();
    }

    // Region 6: Closest to edge BC.
    const auto va = d3 * d6 - d5 * d4;
    if (va <= Num{0} && d4 >= d3 && d5 >= d6) {
      const auto w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
      return b() + w * cb();
    }

    // Region 7: Closest point is inside the triangle face.
    const auto v = vb / (va + vb + vc);
    const auto w = vc / (va + vb + vc);
    return a() + v * ba() + w * ca();
  }

  /// Determine if the triangle intersects the sphere.
  constexpr auto intersects(const BSphere<Vec>& sphere) const noexcept -> bool {
    return sphere.contains(clamp(sphere.center()));
  }

private:

  std::array<Vec, 3> verts_;

}; // class Triangle

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::geom
