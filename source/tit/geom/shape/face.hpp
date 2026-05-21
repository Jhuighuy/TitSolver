/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <array>
#include <cstddef>

#include "tit/core/assert.hpp"
#include "tit/core/math.hpp"
#include "tit/core/vec.hpp"
#include "tit/geom/bbox.hpp"

namespace tit::geom {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Face of a shape: a segment in 2D and a triangle in 3D.
template<class Vec>
class Face final {
public:

  /// Numeric type of the face.
  using Num = vec_num_t<Vec>;

  /// Dimension of the face.
  static constexpr auto Dim = vec_dim_v<Vec>;

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Construct a face with the given vertices.
  constexpr explicit Face(const std::array<Vec, Dim>& verts) : verts_{verts} {}

  /// Vertex at index.
  constexpr auto vert(std::size_t i) const noexcept -> const Vec& {
    TIT_ASSERT(i < Dim, "Vertex index is out of range!");
    return verts_[i];
  }

  /// Vertices.
  constexpr auto verts() const noexcept -> const std::array<Vec, Dim>& {
    return verts_;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Bounding box of the face.
  constexpr auto box() const noexcept -> BBox<Vec> {
    const auto& [v0, ... vs] = verts();
    BBox result{v0};
    (result.expand(vs), ...);
    return result;
  }

  /// Area of the face (length in 2D, area in 3D).
  constexpr auto area() const noexcept -> Num {
    const auto& [v0, ... vs] = verts();
    return norm(cross((vs - v0)...)) / Num{Dim - 1};
  }

  /// Normal vector of the face.
  constexpr auto normal() const noexcept -> Vec {
    const auto& [v0, ... vs] = verts();
    return normalize(cross((vs - v0)...));
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Check if a sphere with @p center and @p radius intersects this face.
  constexpr auto intersects_sphere(const Vec& center,
                                   const Num& radius) const noexcept -> bool {
    const auto r2 = pow2(radius);
    if constexpr (Dim == 2) {
      return segment_intersects_sphere(center, r2, verts_[0], verts_[1]);
    } else if constexpr (Dim == 3) {
      return triangle_intersects_sphere(center, r2);
    } else {
      static_assert(false);
    }
  }

  static auto segment_intersects_sphere(const Vec& center,
                                        Num r2,
                                        const Vec& a,
                                        const Vec& b) noexcept -> bool {
    if (norm2(a - center) <= r2) return true;
    if (norm2(b - center) <= r2) return true;
    const auto ab = b - a;
    const auto ab_len2 = norm2(ab);
    if (ab_len2 == 0) return false;
    const auto ac = center - a;
    const auto t = dot(ac, ab) / ab_len2;
    if (t <= Num{0}) return norm2(ac) <= r2;
    if (t >= Num{1}) return norm2(center - b) <= r2;
    const auto closest = a + t * ab;
    return norm2(center - closest) <= r2;
  }

  auto triangle_intersects_sphere(const Vec& center, Num r2) const noexcept
      -> bool {
    const auto& v0 = verts_[0];
    const auto& v1 = verts_[1];
    const auto& v2 = verts_[2];

    // Check vertices.
    if (norm2(v0 - center) <= r2) return true;
    if (norm2(v1 - center) <= r2) return true;
    if (norm2(v2 - center) <= r2) return true;

    // Check edges.
    if (segment_intersects_sphere(center, r2, v0, v1)) return true;
    if (segment_intersects_sphere(center, r2, v1, v2)) return true;
    if (segment_intersects_sphere(center, r2, v2, v0)) return true;

    // Project sphere center onto triangle plane, check if projected point
    // is inside the triangle and distance to plane <= radius.
    const auto e1 = v1 - v0;
    const auto e2 = v2 - v0;
    const auto n = cross(e1, e2);
    const auto n_len2 = norm2(n);
    if (n_len2 <= pow2(tiny_v<Num>)) return false;

    const auto cv = center - v0;
    const auto dist2 = pow2(dot(cv, n)) / n_len2;
    if (dist2 > r2) return false;

    // Barycentric coordinates: cv = u*e1 + v*e2.
    // u = (cv × e2) · n / |n|²,  v = (e1 × cv) · n / |n|².
    const auto u_raw = dot(cross(cv, e2), n);
    const auto v_raw = dot(cross(e1, cv), n);
    const auto w_raw = n_len2 - u_raw - v_raw;

    return u_raw >= Num{0} && v_raw >= Num{0} && w_raw >= Num{0};
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  std::array<Vec, Dim> verts_;

}; // class Face

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::geom
