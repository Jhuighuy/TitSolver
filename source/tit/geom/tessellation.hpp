/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <cstddef>

#include "tit/core/assert.hpp"
#include "tit/core/math.hpp"
#include "tit/core/vec.hpp"
#include "tit/geom/surface.hpp"

namespace tit::geom {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Tessellate the 2D surface so that no segment is longer than @p d_max.
/// @todo Implement the 3D version.
template<class Vec>
  requires (vec_dim_v<Vec> == 2)
constexpr auto tessellate(const Surface<Vec>& surf, vec_num_t<Vec> d_max)
    -> Surface<Vec> {
  using Num = vec_num_t<Vec>;
  TIT_ASSERT(d_max > Num{0}, "Maximum segment length must be positive.");

  Surface<Vec> result;

  // Copy the existing vertices.
  for (const auto& vert : surf.verts()) result.append_vert(vert);

  // Copy and split the segments.
  for (std::size_t face_index = 0; face_index < surf.num_faces();
       ++face_index) {
    // Compute the number of segments and the segment length.
    const auto& face = surf.face(face_index);
    const auto d = face.length();
    const auto n = static_cast<std::size_t>(ceil(d / d_max));
    TIT_ASSERT(n >= 1, "Number of segments must be at least 1.");

    // Append the vertices and faces.
    const auto& [first_vert_index, last_vert_index] =
        surf.face_verts(face_index);
    auto prev_vert_index = first_vert_index;
    for (std::size_t i = 1; i < n; ++i) {
      const auto t = static_cast<Num>(i) / static_cast<Num>(n);
      const auto vert = face.a() + t * face.ba();
      result.append_vert(vert);
      const auto vert_index = result.num_verts() - 1;
      result.append_face({prev_vert_index, vert_index});
      prev_vert_index = vert_index;
    }
    result.append_face({prev_vert_index, last_vert_index});
  }

  return result;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::geom
