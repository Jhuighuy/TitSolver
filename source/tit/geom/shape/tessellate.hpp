/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <unordered_map>
#include <utility>
#include <vector>

#include "tit/core/assert.hpp"
#include "tit/core/math.hpp"
#include "tit/core/vec.hpp"
#include "tit/geom/shape/shape.hpp"

namespace tit::geom {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Tessellate the shape so that no face has an edge longer than @p d_max.
template<class Vec>
constexpr void tessellate(Shape<Vec>& shape, const vec_num_t<Vec>& d_max) {
  using Num = vec_num_t<Vec>;
  constexpr auto Dim = vec_dim_v<Vec>;

  TIT_ASSERT(d_max > Num{0}, "Maximum edge length must be positive!");

  if constexpr (Dim == 2) {
    // Split each segment into ceil(L/d_max) equal-length pieces.
    std::vector<std::array<std::size_t, 2>> new_faces;
    new_faces.reserve(shape.faces_.size());

    for (const auto& face : shape.faces_) {
      const auto a_idx = face[0];
      const auto b_idx = face[1];
      const auto a = shape.verts_[a_idx]; // copy (push_back may reallocate)
      const auto b = shape.verts_[b_idx]; // copy
      const auto edge_len = norm(b - a);
      const auto n = static_cast<std::size_t>(ceil(edge_len / d_max));

      if (n <= 1) {
        new_faces.push_back(face);
        continue;
      }

      // Subdivide into n equal-length segments.
      std::size_t prev = a_idx;
      for (std::size_t k = 1; k < n; ++k) {
        const auto t = static_cast<Num>(k) / static_cast<Num>(n);
        shape.verts_.push_back(a + t * (b - a));
        new_faces.push_back({prev, shape.verts_.size() - 1});
        prev = shape.verts_.size() - 1;
      }
      new_faces.push_back({prev, b_idx});
    }

    shape.faces_ = std::move(new_faces);
    return;
  }

  if constexpr (Dim == 3) {
    // Recursive longest-edge bisection for triangles.
    const auto d_max2 = pow2(d_max);

    using EdgePair = std::pair<std::size_t, std::size_t>;
    struct EdgeHash {
      static auto operator()(const EdgePair& p) noexcept -> std::size_t {
        return p.first ^ (p.second << 16);
      }
    };

    bool did_split = true;
    while (did_split) {
      did_split = false;

      std::unordered_map<EdgePair, std::size_t, EdgeHash> edge_cache;
      std::vector<std::array<std::size_t, 3>> new_faces;
      new_faces.reserve(shape.faces_.size());

      for (const auto& face : shape.faces_) {
        // Find the longest edge of the face.
        const auto e01_2 = norm2(shape.verts_[face[1]] - shape.verts_[face[0]]);
        const auto e12_2 = norm2(shape.verts_[face[2]] - shape.verts_[face[1]]);
        const auto e20_2 = norm2(shape.verts_[face[0]] - shape.verts_[face[2]]);

        std::size_t split_a = face[0]; // NOLINT(misc-const-correctness)
        std::size_t split_b = face[0]; // NOLINT(misc-const-correctness)
        Num max_edge_len2{};

        if (e01_2 >= e12_2 && e01_2 >= e20_2) {
          max_edge_len2 = e01_2;
          split_a = face[0];
          split_b = face[1];
        } else if (e12_2 >= e20_2) {
          max_edge_len2 = e12_2;
          split_a = face[1];
          split_b = face[2];
        } else {
          max_edge_len2 = e20_2;
          split_a = face[2];
          split_b = face[0];
        }

        if (max_edge_len2 <= d_max2) {
          new_faces.push_back(face);
          continue;
        }

        did_split = true;

        // Get or create the midpoint vertex for the split edge.
        const auto [ea, eb] = std::minmax(split_a, split_b);
        const auto [iter, inserted] =
            edge_cache.try_emplace(EdgePair{ea, eb}, shape.verts_.size());
        if (inserted) {
          shape.verts_.push_back((shape.verts_[ea] + shape.verts_[eb]) /
                                 Num{2});
        }
        const auto mid = iter->second;

        // Find the vertex not on the split edge.
        std::size_t third = face[0];
        for (std::size_t k = 0; k < 3; ++k) {
          if (face[k] != split_a && face[k] != split_b) {
            third = face[k];
            break;
          }
        }

        new_faces.push_back({split_a, mid, third});
        new_faces.push_back({mid, split_b, third});
      }

      shape.faces_ = std::move(new_faces);
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::geom
