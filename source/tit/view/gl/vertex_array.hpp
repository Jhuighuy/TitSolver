/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// IWYU pragma: private, include "tit/view/gl.hpp"
#pragma once

#include <concepts>

#include <GL/glew.h>
#include <glm/glm.hpp>

#include "tit/view/gl/buffer.hpp"

namespace tit::view::gl {

/// OpenGL draw mode.
enum class DrawMode : GLenum { // NOLINT(*-enum-size)
  points = GL_POINTS,
  lines = GL_LINES,
  triangles = GL_TRIANGLES,
}; // enum class DrawMode

/// OpenGL type enumeration.
template<class Type>
inline constexpr auto vertex_attrib_type_v = []() -> GLenum {
  if constexpr (std::same_as<Type, GLbyte>) return GL_BYTE;
  if constexpr (std::same_as<Type, GLubyte>) return GL_UNSIGNED_BYTE;
  if constexpr (std::same_as<Type, GLshort>) return GL_SHORT;
  if constexpr (std::same_as<Type, GLushort>) return GL_UNSIGNED_SHORT;
  if constexpr (std::same_as<Type, GLint>) return GL_INT;
  if constexpr (std::same_as<Type, GLuint>) return GL_UNSIGNED_INT;
  if constexpr (std::same_as<Type, GLfloat>) return GL_FLOAT;
  if constexpr (std::same_as<Type, GLdouble>) return GL_DOUBLE;
  return 0;
}();

/// Number of the components in type.
template<class>
inline constexpr GLint vertex_attrib_length_v = 1;

/// Vertex attribute type.
template<class VertexAttrib>
concept vertex_attrib = (vertex_attrib_type_v<VertexAttrib> != 0);

template<glm::length_t Length, vertex_attrib Type>
inline constexpr GLenum
    vertex_attrib_type_v<glm::vec<Length, Type, glm::packed>> = // NOLINT
    vertex_attrib_type_v<Type>;
template<glm::length_t Length, vertex_attrib Type>
inline constexpr GLint
    vertex_attrib_length_v<glm::vec<Length, Type, glm::packed>> =
        static_cast<GLint>(Length);

struct indexed_t {};

/// Indexed vertex array helper.
inline constexpr indexed_t indexed_v{};

/// OpenGL vertex array.
class VertexArray final {
private:

  GLuint vertex_array_id_{};

public:

  /// Construct a vertex array.
  VertexArray() {
    glGenVertexArrays(1, &vertex_array_id_);
  }

  /// Construct a vertex array with buffers.
  /// @{
  template<vertex_attrib... Types>
  explicit VertexArray(const Buffer<Types>&... vertex_buffers) : VertexArray{} {
    assign(vertex_buffers...);
  }
  template<vertex_attrib... Types>
  VertexArray(indexed_t /*tag*/,
              const Buffer<GLuint>& index_buffer,
              const Buffer<Types>&... vertex_buffers)
      : VertexArray{} {
    assign_indexed(index_buffer, vertex_buffers...);
  }
  /// @}

  /// Move-construct a vertex array.
  VertexArray(VertexArray&&) = default;
  /// Move-assign the vertex array.
  auto operator=(VertexArray&&) -> VertexArray& = default;

  VertexArray(const VertexArray&) = delete;
  auto operator=(const VertexArray&) -> VertexArray& = delete;

  /// Destruct the vertex array.
  ~VertexArray() {
    glDeleteVertexArrays(1, &vertex_array_id_);
  }

  /// Cast to vertex array ID.
  constexpr operator GLuint() const noexcept {
    return vertex_array_id_;
  }

  /// Build the vertex array buffer.
  /// @{
  template<vertex_attrib... Types>
  void assign(const Buffer<Types>&... vertex_buffers) {
    glBindVertexArray(vertex_array_id_);
    attach_vertex_attribs_(vertex_buffers...);
  }
  template<vertex_attrib... Types>
  void assign_indexed(const Buffer<GLuint>& index_buffer,
                      const Buffer<Types>&... vertex_buffers) {
    glBindVertexArray(vertex_array_id_);
    index_buffer.bind(BufferTarget::element_array_buffer);
    attach_vertex_attribs_(vertex_buffers...);
  }
  /// @}

  /// Draw the vertex array.
  /// @{
  void draw(DrawMode mode, GLsizei count) const {
    glBindVertexArray(vertex_array_id_);
    glDrawArrays(static_cast<GLenum>(mode), /*first*/ 0, count);
  }
  void draw_indexed(DrawMode mode, GLsizei count) const {
    glBindVertexArray(vertex_array_id_);
    glDrawElements(static_cast<GLenum>(mode),
                   count,
                   GL_UNSIGNED_INT,
                   /*indices*/ nullptr);
  }
  /// @}

private:

  template<vertex_attrib... Types>
  static void attach_vertex_attribs_(const Buffer<Types>&... vertex_buffers) {
    GLuint index = 0;
    (attach_single_vertex_attrib(index++, vertex_buffers), ...);
  }

  template<vertex_attrib Type>
  static void attach_single_vertex_attrib(GLuint index,
                                          const Buffer<Type>& vertex_buffer) {
    vertex_buffer.bind(BufferTarget::array_buffer);
    glEnableVertexAttribArray(index);
    glVertexAttribPointer(index, //
                          vertex_attrib_length_v<Type>,
                          vertex_attrib_type_v<Type>,
                          /*normalized*/ GL_FALSE,
                          /*stride*/ 0,
                          /*offset*/ nullptr);
  }

}; // class VertexArray

} // namespace tit::view::gl
