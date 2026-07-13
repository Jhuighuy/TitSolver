/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <array>
#include <cstddef>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "tit/core/assert.hpp"
#include "tit/core/float.hpp"
#include "tit/core/vec.hpp"
#include "tit/geom/segment.hpp"
#include "tit/geom/triangle.hpp"

namespace tit::geom {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Surface.
template<class Vec>
class Surface final {
public:

  /// Dimension of the surface.
  static constexpr auto Dim = vec_dim_v<Vec>;
  static_assert(Dim == 2 || Dim == 3);

  /// Face type.
  using Face = std::conditional_t<Dim == 2, Segment<Vec>, Triangle<Vec>>;

  /// Face vertex index array type.
  using FaceVerts = std::array<std::size_t, Dim>;

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

  /// Append a vertex.
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
      -> const FaceVerts& {
    TIT_ASSERT(face_index < num_faces(), "Face index is out of range!");
    return faces_[face_index];
  }

  /// Get all face vertices.
  constexpr auto face_verts() const noexcept -> std::span<const FaceVerts> {
    return faces_;
  }

  /// Get the face at index.
  constexpr auto face(std::size_t face_index) const noexcept -> Face {
    const auto& [... vert_indices] = face_verts(face_index);
    return Face{vert(vert_indices)...};
  }

  /// Get all faces.
  constexpr auto faces() const noexcept {
    return std::views::iota(std::size_t{0}, num_faces()) |
           std::views::transform(std::bind_front(&Surface::face, this));
  }

  /// Append a face.
  constexpr void append_face(const FaceVerts& face_verts) {
    const auto& [... vert_indices] = face_verts;
    TIT_ASSERT(((vert_indices < num_verts()) && ...),
               "Face contains invalid vertex index!");
    faces_.push_back(face_verts);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  std::vector<Vec> verts_;
  std::vector<FaceVerts> faces_;

}; // class Surface

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Cast the underlying vector type of the surface.
template<class ToVec, class Vec>
  requires (vec_dim_v<ToVec> == vec_dim_v<Vec>)
constexpr auto surface_cast(const Surface<Vec>& surf) -> Surface<ToVec> {
  Surface<ToVec> result;
  for (const auto& vert : surf.verts()) {
    result.append_vert(vec_cast<vec_num_t<ToVec>>(vert));
  }
  for (std::size_t face_index = 0; face_index < surf.num_faces();
       ++face_index) {
    result.append_face(surf.face_verts(face_index));
  }
  return result;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Surface type used for IO.
template<std::size_t Dim>
using IOSurface = Surface<Vec<float32_t, Dim>>;

/// Read a surface from a string (backed by Assimp).
/// @{
template<std::size_t Dim>
auto surface_from_string(std::string_view str) -> IOSurface<Dim>;
template<class Vec>
auto surface_from_string(std::string_view str) -> Surface<Vec> {
  return surface_cast<Vec>(surface_from_string<vec_dim_v<Vec>>(str));
}
///@}

/// Write a surface to a string (backed by Assimp).
/// @{
template<std::size_t Dim>
auto surface_dump_string(const IOSurface<Dim>& surf, const std::string& format)
    -> std::string;
template<class Vec>
auto surface_dump_string(const Surface<Vec>& surf, const std::string& format)
    -> std::string {
  return surface_dump_string(
      surface_cast<tit::Vec<float32_t, vec_dim_v<Vec>>>(surf),
      format);
}
/// @}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::geom
