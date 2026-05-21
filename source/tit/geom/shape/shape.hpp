/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <array>
#include <cstddef>
#include <ranges>
#include <span>
#include <vector>

#include "tit/core/assert.hpp"
#include "tit/core/vec.hpp"
#include "tit/geom/shape/face.hpp"

namespace tit::geom {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Shape.
template<class Vec>
class Shape final {
public:

  /// Dimension of the face.
  static constexpr auto Dim = vec_dim_v<Vec>;

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Get the number of vertices.
  constexpr auto num_verts() const noexcept -> std::size_t {
    return verts_.size();
  }

  /// Get the vertex at index.
  constexpr auto vert(std::size_t vert_index) const noexcept -> const Vec& {
    TIT_ASSERT(vert_index < num_verts(), "Vertex index is out of range!");
    return verts_[vert_index];
  }

  /// Get all vertices.
  constexpr auto verts() const noexcept -> std::span<const Vec> {
    return verts_;
  }

  /// Append a vertex to the shape.
  constexpr void append_vert(const Vec& vert) {
    verts_.push_back(vert);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Get the number of faces.
  constexpr auto num_faces() const noexcept -> std::size_t {
    return faces_.size();
  }

  /// Get the face vertices at index.
  constexpr auto face_verts(std::size_t face_index) const noexcept
      -> const std::array<std::size_t, Dim>& {
    TIT_ASSERT(face_index < num_faces(), "Face index is out of range!");
    return faces_[face_index];
  }

  /// Get the face at index.
  constexpr auto face(std::size_t face_index) const noexcept -> Face<Vec> {
    const auto& [... vert_indices] = face_verts(face_index);
    return Face{std::array{vert(vert_indices)...}};
  }

  /// Get all faces.
  constexpr auto faces() const noexcept {
    return std::views::iota(std::size_t{0}, num_faces()) |
           std::views::transform(
               [this](std::size_t face_index) { return face(face_index); });
  }

  /// Append a face to the shape.
  constexpr void append_face(const std::array<std::size_t, Dim>& face_verts) {
    const auto& [... vert_indices] = face_verts;
    TIT_ASSERT(((vert_indices < num_verts()) && ...),
               "Face contains invalid vertex index!");
    faces_.push_back(face_verts);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  template<class V>
  friend constexpr void tessellate(Shape<V>&, const vec_num_t<V>&);

private:

  std::vector<Vec> verts_;
  std::vector<std::array<std::size_t, Dim>> faces_;

}; // class Shape

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::geom
