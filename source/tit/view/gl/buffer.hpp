/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// IWYU pragma: private, include "tit/view/gl.hpp"
#pragma once

#include <algorithm>
#include <concepts>
#include <ranges>
#include <vector>

#include <GL/glew.h>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/exception.hpp"

#include "tit/core/utils.hpp"

namespace tit::view::gl {

/// OpenGL buffer usage.
enum class BufferUsage : GLenum { // NOLINT(*-enum-size)
  stream_draw = GL_STREAM_DRAW,
  stream_read = GL_STREAM_READ,
  stream_copy = GL_STREAM_COPY,
  static_draw = GL_STATIC_DRAW,
  static_read = GL_STATIC_READ,
  static_copy = GL_STATIC_COPY,
  dynamic_draw = GL_DYNAMIC_DRAW,
  dynamic_read = GL_DYNAMIC_READ,
  dynamic_copy = GL_DYNAMIC_COPY,
}; // enum class BufferUsage

/// OpenGL buffer binding target.
enum class BufferTarget : GLenum { // NOLINT(*-enum-size)
  array_buffer = GL_ARRAY_BUFFER,
  element_array_buffer = GL_ELEMENT_ARRAY_BUFFER,
  pixel_pack_buffer = GL_PIXEL_PACK_BUFFER,
  pixel_unpack_buffer = GL_PIXEL_UNPACK_BUFFER,
  texture_buffer = GL_TEXTURE_BUFFER,
}; // enum class BufferTarget

/// OpenGL buffer.
template<class Type>
class Buffer {
private:

  GLuint buffer_id_{};
  GLsizei buffer_size_ = 0;

public:

  /// Construct a buffer.
  Buffer() {
    glGenBuffers(1, &buffer_id_);
  }

  /// Construct a buffer with a size.
  /// @param usage Intended buffer usage.
  explicit Buffer(GLsizei size, BufferUsage usage = BufferUsage::static_draw)
      : Buffer{} {
    assign(size, usage);
  }

  /// Construct a buffer with a @p size copies of @p value.
  /// @param usage Intended buffer usage.
  explicit Buffer(GLsizei size,
                  const Type& value,
                  BufferUsage usage = BufferUsage::static_draw)
      : Buffer{} {
    assign(size, value, usage);
  }

  /// Construct a buffer with a range.
  /// @param usage Intended buffer usage.
  template<std::ranges::input_range Range>
    requires std::same_as<std::ranges::range_value_t<Range>, Type>
  Buffer(Range&& values, BufferUsage usage = BufferUsage::static_draw)
      : Buffer{} {
    assign(std::forward<Range>(values), usage);
  }

  Buffer(const Buffer&) = default;
  auto operator=(const Buffer&) -> Buffer& = default;

  /// Move-construct a buffer.
  Buffer(Buffer&&) = default;
  /// Move-assign the buffer.
  auto operator=(Buffer&&) -> Buffer& = default;

  /// Destruct the buffer.
  ~Buffer() {
    glDeleteBuffers(1, &buffer_id_);
  }

  /// Cast to buffer ID.
  constexpr operator GLuint() const noexcept {
    return buffer_id_;
  }

  /// Buffer size.
  constexpr auto size() const noexcept -> GLsizei {
    return buffer_size_;
  }

  /// Bind the buffer to @p target.
  void bind(BufferTarget target) const {
    glBindBuffer(static_cast<GLenum>(target), buffer_id_);
  }

  /// Assign the buffer @p size.
  /// @param usage Intended buffer usage.
  void assign(GLsizei size, BufferUsage usage = BufferUsage::static_draw) {
    glBindBuffer(GL_ARRAY_BUFFER, buffer_id_);
    buffer_size_ = size;
    const auto num_bytes = static_cast<GLsizeiptr>(size * sizeof(Type));
    glBufferData(GL_ARRAY_BUFFER,
                 num_bytes,
                 /*data*/ nullptr,
                 static_cast<GLenum>(usage));
  }

  /// Assign the buffer @p size copies of @p value.
  void assign(GLsizei size,
              const Type& value,
              BufferUsage /*usage*/ = BufferUsage::static_draw) {
    assign(std::views::repeat(value, size));
  }

  /// Assign the buffer @p values.
  /// @param usage Intended buffer usage.
  template<std::ranges::input_range Range>
    requires std::same_as<std::ranges::range_value_t<Range>, Type>
  void assign(Range&& values, BufferUsage usage = BufferUsage::static_draw) {
    TIT_ASSUME_UNIVERSAL(Range, values);
    glBindBuffer(GL_ARRAY_BUFFER, buffer_id_);
    if constexpr (std::ranges::sized_range<Range>) {
      assign(static_cast<GLsizei>(values.size()));
      auto* const pointer = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
      if (pointer == nullptr) {
        TIT_THROW("Failed to map the buffer!");
      }
      std::ranges::copy(values, static_cast<Type*>(pointer));
      if (glUnmapBuffer(GL_ARRAY_BUFFER) != GL_TRUE) {
        TIT_THROW("Failed to unmap the buffer!");
      }
    } else {
      std::vector<Type> temp{};
      std::ranges::copy(values, std::back_inserter(temp));
      buffer_size_ = static_cast<GLsizei>(temp.size());
      const auto num_bytes =
          static_cast<GLsizeiptr>(buffer_size_ * sizeof(Type));
      glBufferData(GL_ARRAY_BUFFER,
                   num_bytes,
                   temp.data(),
                   static_cast<GLenum>(usage));
    }
  }

  /// @todo Refactor as `operator[]`.
  /// Get the buffer value at @p index.
  auto get(size_t index) const -> Type {
    TIT_ASSERT(index < static_cast<size_t>(buffer_size_),
               "Index is out of range!");
    glBindBuffer(GL_ARRAY_BUFFER, buffer_id_);
    auto* const pointer =
        glMapBufferRange(GL_ARRAY_BUFFER,
                         static_cast<GLintptr>(index * sizeof(Type)),
                         static_cast<GLsizeiptr>(sizeof(Type)),
                         GL_MAP_READ_BIT);
    if (pointer == nullptr) {
      TIT_THROW("Failed to map the buffer!");
    }
    Type value = *static_cast<const Type*>(pointer);
    if (glUnmapBuffer(GL_ARRAY_BUFFER) != GL_TRUE) {
      TIT_THROW("Failed to unmap the buffer!");
    }
    return value;
  }

  /// Set the buffer @p value at @p index.
  void set(size_t index, const Type& value) {
    TIT_ASSERT(index < static_cast<size_t>(buffer_size_),
               "Index is out of range!");
    glBindBuffer(GL_ARRAY_BUFFER, buffer_id_);
    const auto offset = static_cast<GLintptr>(index * sizeof(Type));
    glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(Type), &value);
  }

}; // class Buffer

template<std::ranges::input_range Range, class... T>
Buffer(Range&&, T...) -> Buffer<std::ranges::range_value_t<Range>>;

} // namespace tit::view::gl
