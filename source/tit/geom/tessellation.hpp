/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <limits>
#include <map>
#include <ranges>
#include <utility>
#include <vector>

#include "tit/core/assert.hpp"
#include "tit/core/math.hpp"
#include "tit/core/vec.hpp"
#include "tit/geom/surface.hpp"

namespace tit::geom {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Tessellate the 2D surface so that no segment is longer than @p d_max.
template<class Vec>
  requires std::floating_point<vec_num_t<Vec>> && (vec_dim_v<Vec> == 2)
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
    const auto n = std::max(1UZ, static_cast<std::size_t>(ceil(d / d_max)));

    // Append the vertices and faces.
    auto [prev_vert_index, last_vert_index] = surf.face_verts(face_index);
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

/// Tessellate the 3D surface so that no edge is longer than @p d_max.
///
/// Uses iterative red refinement: all edges exceeding @p d_max are bisected
/// simultaneously, then each triangle is split into 2/3/4 sub-triangles
/// depending on how many of its edges were bisected.
/// The process repeats until no edges exceed @p d_max.
template<class Vec>
  requires std::floating_point<vec_num_t<Vec>> && (vec_dim_v<Vec> == 3)
constexpr auto tessellate(const Surface<Vec>& surf, vec_num_t<Vec> d_max)
    -> Surface<Vec> {
  using Num = vec_num_t<Vec>;
  TIT_ASSERT(d_max > Num{0}, "Maximum edge length must be positive.");

  // Start with the original vertices and faces.
  std::vector verts(std::from_range, surf.verts());
  std::vector<std::array<std::size_t, 3>> faces;
  faces.reserve(surf.num_faces());
  for (std::size_t i = 0; i < surf.num_faces(); ++i) {
    faces.push_back(surf.face_verts(i));
  }

  // Preallocate the next faces.
  std::vector<std::array<std::size_t, 3>> next_faces;
  next_faces.reserve(surf.num_faces());

  // Edge to midpoint mapping.
  using EdgeKey = std::pair<std::size_t, std::size_t>;
  std::map<EdgeKey, std::size_t> midpoints;

  // Iteratively refine until all edges are within the limit.
  for (;;) {
    midpoints.clear();

    // Find all edges exceeding `d_max` and compute midpoints.
    const auto check_edge = [&verts, &midpoints, d_max](std::size_t a,
                                                        std::size_t b) {
      const EdgeKey key{std::minmax(a, b)};
      if (midpoints.contains(key)) return;
      if (norm2(verts[b] - verts[a]) <= pow2(d_max)) return;
      verts.push_back(avg(verts[a], verts[b]));
      midpoints.try_emplace(key, verts.size() - 1);
    };
    for (const auto& [a, b, c] : faces) {
      check_edge(a, b);
      check_edge(b, c);
      check_edge(c, a);
    }

    // If no edges were bisected, we are done.
    if (midpoints.empty()) break;

    // Split each triangle based on how many edges were bisected.
    static constexpr auto npos = std::numeric_limits<std::size_t>::max();
    const auto get_mid = [&midpoints](std::size_t a, std::size_t b) {
      const EdgeKey key{std::minmax(a, b)};
      const auto iter = midpoints.find(key);
      return iter == midpoints.end() ? npos : iter->second;
    };
    next_faces.clear();
    for (const auto& [a, b, c] : faces) {
      const auto m_ab = get_mid(a, b);
      const auto m_bc = get_mid(b, c);
      const auto m_ca = get_mid(c, a);

      // 3 midpoints added: split into 4 triangles.
      if (m_ab != npos && m_bc != npos && m_ca != npos) {
        next_faces.push_back({a, m_ab, m_ca});
        next_faces.push_back({m_ab, b, m_bc});
        next_faces.push_back({m_ca, m_bc, c});
        next_faces.push_back({m_ab, m_bc, m_ca});
        continue;
      }

      // 2 midpoints added: split into 3 triangles.
      if (m_ab != npos && m_bc != npos) {
        next_faces.push_back({a, m_ab, c});
        next_faces.push_back({m_ab, m_bc, c});
        next_faces.push_back({m_ab, b, m_bc});
        continue;
      }
      if (m_bc != npos && m_ca != npos) {
        next_faces.push_back({b, m_bc, a});
        next_faces.push_back({m_bc, m_ca, a});
        next_faces.push_back({m_bc, c, m_ca});
        continue;
      }
      if (m_ab != npos && m_ca != npos) {
        next_faces.push_back({c, m_ca, b});
        next_faces.push_back({m_ca, m_ab, b});
        next_faces.push_back({m_ca, a, m_ab});
        continue;
      }

      // 1 midpoint added: split into 2 triangles.
      if (m_ab != npos) {
        next_faces.push_back({a, m_ab, c});
        next_faces.push_back({m_ab, b, c});
        continue;
      }
      if (m_bc != npos) {
        next_faces.push_back({b, m_bc, a});
        next_faces.push_back({m_bc, c, a});
        continue;
      }
      if (m_ca != npos) {
        next_faces.push_back({c, m_ca, b});
        next_faces.push_back({m_ca, a, b});
        continue;
      }

      // No midpoints added: keep the triangle as is.
      next_faces.push_back({a, b, c});
    }
    faces.swap(next_faces);
  }

  // Build the result surface.
  Surface<Vec> result;
  for (const auto& vert : verts) result.append_vert(vert);
  for (const auto& face : faces) result.append_face(face);
  return result;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::geom
