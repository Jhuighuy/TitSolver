/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// IWYU pragma: private, include "tit/view/gl.hpp"
#pragma once

#include <concepts>
#include <string>
#include <string_view>

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "tit/core/checks.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/log.hpp"

/// A macro, used to spawn shaders in '.glsl' files.
#define STORM_VULTURE_SHADER_(type, source)                                    \
  []() { return gl::Shader{gl::ShaderType::type, source}; }(),

namespace tit::view::gl {

/// OpenGL shader type.
enum class ShaderType : GLenum { // NOLINT(*-enum-size)
  vertex = GL_VERTEX_SHADER,
  geometry = GL_GEOMETRY_SHADER,
  fragment = GL_FRAGMENT_SHADER,
}; // enum class ShaderType

/// OpenGL shader.
class Shader final {
private:

  GLuint shader_id_;

public:

  /// Construct a shader.
  explicit Shader(ShaderType type)
      : shader_id_(glCreateShader(static_cast<GLenum>(type))) {}
  /// Construct a shader with @p source.
  Shader(ShaderType type, std::string_view source) : Shader{type} {
    assign(source);
  }

  /// Move-construct a shader.
  Shader(Shader&&) = default;
  /// Move-assign the shader.
  auto operator=(Shader&&) -> Shader& = default;

  Shader(const Shader&) = delete;
  auto operator=(const Shader&) -> Shader& = delete;

  /// Destruct the shader.
  ~Shader() {
    glDeleteShader(shader_id_);
  }

  /// Cast to shader ID.
  constexpr operator GLuint() const noexcept {
    return shader_id_;
  }

  /// Load the shader of type @p type from @p source.
  void assign(std::string_view source) {
    static_cast<void>(this);

    // Upload the shader source code.
    const auto* const source_data = source.data();
    const auto source_size = static_cast<GLint>(source.size());
    glShaderSource(shader_id_, 1, &source_data, &source_size);

    // Try to compile the shader and check result.
    glCompileShader(shader_id_);
    GLint status = 0;
    glGetShaderiv(shader_id_, GL_COMPILE_STATUS, &status);
    std::string info_log(1024, '\0');
    GLsizei info_log_length = 0;
    glGetShaderInfoLog(shader_id_,
                       static_cast<GLsizei>(info_log.size()),
                       &info_log_length,
                       info_log.data());
    if (status != GL_TRUE) {
      TIT_THROW("Failed to compile shader: {}", info_log);
    }
    if (info_log_length != 0) {
      TIT_WARN("Shader compilation message: {}", info_log);
    }
  }

}; // class Shader

/// OpenGL shader program.
class Program final {
private:

  GLuint program_id_ = 0;

public:

  /// Construct a program.
  Program() : program_id_(glCreateProgram()) {}
  /// Construct a program with @p shaders.
  template<std::same_as<Shader>... Shader>
  explicit Program(const Shader&... shaders) : Program() {
    assign(shaders...);
  }

  /// Move-construct a program.
  Program(Program&&) = default;
  /// Move-assign the program.
  auto operator=(Program&&) -> Program& = default;

  Program(const Program&) = delete;
  auto operator=(const Program&) -> Program& = delete;

  /// Destruct the program.
  ~Program() noexcept {
    glDeleteProgram(program_id_);
  }

  /// Cast to shader ID.
  constexpr operator GLuint() const noexcept {
    return program_id_;
  }

  /// Load the program with @p shaders.
  template<std::same_as<Shader>... Shader>
  void assign(const Shader&... shaders) {
    // Attach the shaders.
    (glAttachShader(program_id_, shaders), ...);

    // Try to link the program and check result.
    glLinkProgram(program_id_);
    GLint status = 0;
    glGetProgramiv(program_id_, GL_LINK_STATUS, &status);
    std::string info_log(1024, '\0');
    GLsizei info_log_length = 0;
    glGetProgramInfoLog(program_id_,
                        static_cast<GLsizei>(info_log.size()),
                        &info_log_length,
                        info_log.data());
    if (status != GL_TRUE) {
      TIT_THROW("Failed to link program: {}", info_log);
    }
    if (info_log_length != 0) {
      TIT_WARN("Program linking message: {}", info_log);
    }
  }

  /// Get program uniform location.
  /// @{
  auto operator[](const std::string& name) const -> GLint {
    return (*this)[name.c_str()];
  }
  auto operator[](const char* name) const -> GLint {
    TIT_ASSERT(name != nullptr, "Invalid uniform name!");
    return glGetUniformLocation(program_id_, name);
  }
  /// @}

}; // class Program

/// Binder for an OpenGL program.
class BindProgram final {
public:

  /// Bind the @p program.
  explicit BindProgram(const Program& program) {
    TIT_ASSERT(program != 0, "Invalid program!");
    glUseProgram(program);
  }

  /// Set the scalar uniform value.
  /// @{
  void set_uniform(GLint location, GLfloat value) {
    static_cast<void>(this);
    TIT_ASSERT(location >= 0, "Invalid location!");
    glUniform1f(location, value);
  }
  void set_uniform(GLint location, GLint value) {
    static_cast<void>(this);
    TIT_ASSERT(location >= 0, "Invalid location!");
    glUniform1i(location, value);
  }
  void set_uniform(GLint location, GLuint value) {
    static_cast<void>(this);
    TIT_ASSERT(location >= 0, "Invalid location!");
    glUniform1ui(location, value);
  }
  /// @}

  /// Set the vector uniform value.
  /// @{
  template<glm::length_t Length, glm::qualifier Qualifier> // NOLINT
  void set_uniform(
      GLint location,
      const glm::vec<Length, GLfloat, Qualifier>& value) { // NOLINT
    TIT_ASSERT(location >= 0, "Invalid location!");
    if constexpr (Length == 2) {
      glUniform2f(location, value.x, value.y);
    } else if constexpr (Length == 3) {
      glUniform3f(location, value.x, value.y, value.z);
    } else if constexpr (Length == 4) {
      glUniform4f(location, value.x, value.y, value.z, value.w);
    } else static_assert(false);
  }
  template<glm::length_t Length, glm::qualifier Qualifier>
  void set_uniform(GLint location,
                   const glm::vec<Length, GLint, Qualifier>& value) {
    TIT_ASSERT(location >= 0, "Invalid location!");
    if constexpr (Length == 2) {
      glUniform2i(location, value.x, value.y);
    } else if constexpr (Length == 3) {
      glUniform3i(location, value.x, value.y, value.z);
    } else if constexpr (Length == 4) {
      glUniform4i(location, value.x, value.y, value.z, value.w);
    } else static_assert(false);
  }
  template<glm::length_t Length, glm::qualifier Qualifier>
  void set_uniform(GLint location,
                   const glm::vec<Length, GLuint, Qualifier>& value) {
    TIT_ASSERT(location >= 0, "Invalid location!");
    if constexpr (Length == 2) {
      glUniform2ui(location, value.x, value.y);
    } else if constexpr (Length == 3) {
      glUniform3ui(location, value.x, value.y, value.z);
    } else if constexpr (Length == 4) {
      glUniform4ui(location, value.x, value.y, value.z, value.w);
    } else static_assert(false);
  }
  /// @}

  /// Set the matrix uniform value.
  template<glm::length_t Rows, glm::length_t Cols, glm::qualifier Qualifier>
  void set_uniform(
      GLint location,
      const glm::mat<Rows, Cols, GLfloat, Qualifier>& value) { // NOLINT
    TIT_ASSERT(location >= 0, "Invalid location!");
    const auto packed_value =
        static_cast<glm::mat<Rows,
                             Cols,
                             GLfloat,              //
                             glm::packed>>(value); // NOLINT
    if constexpr (Rows == Cols) {
      if constexpr (Rows == 2) {
        glUniformMatrix2fv(location, 1, GL_FALSE, glm::value_ptr(packed_value));
      } else if constexpr (Rows == 3) {
        glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(packed_value));
      } else if constexpr (Rows == 4) {
        glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(packed_value));
      } else {
        static_assert(false);
      }
    } else if constexpr (Rows == 2 && Cols == 3) {
      glUniformMatrix2x3fv(location, 1, GL_FALSE, glm::value_ptr(packed_value));
    } else if constexpr (Rows == 3 && Cols == 2) {
      glUniformMatrix3x2fv(location, 1, GL_FALSE, glm::value_ptr(packed_value));
    } else if constexpr (Rows == 3 && Cols == 4) {
      glUniformMatrix3x4fv(location, 1, GL_FALSE, glm::value_ptr(packed_value));
    } else if constexpr (Rows == 4 && Cols == 3) {
      glUniformMatrix4x3fv(location, 1, GL_FALSE, glm::value_ptr(packed_value));
    } else {
      static_assert(false);
    }
  }

#ifdef NOT_WORKING
  /// Set the scalar uniform array value.
  /// @{
  // clang-format off
  template<std::ranges::contiguous_range Range>
    requires std::same_as<std::ranges::range_value_t<Range>, GLfloat>
  void set_uniform_array(GLint location, Range&& values) {
    // clang-format on
    TIT_ASSERT(location >= 0, "Invalid location!");
    glUniform1fv(location, static_cast<GLsizei>(values.size()), values.data());
  }
  // clang-format off
  template<std::ranges::contiguous_range Range>
    requires std::same_as<std::ranges::range_value_t<Range>, GLint>
  void set_uniform_array(GLint location, Range&& values) {
    // clang-format on
    TIT_ASSERT(location >= 0, "Invalid location!");
    glUniform1iv(location, static_cast<GLsizei>(values.size()), values.data());
  }
  // clang-format off
  template<std::ranges::contiguous_range Range>
    requires std::same_as<std::ranges::range_value_t<Range>, GLuint>
  void set_uniform_array(GLint location, Range&& values) {
    // clang-format on
    TIT_ASSERT(location >= 0, "Invalid location!");
    glUniform1uiv(location, static_cast<GLsizei>(values.size()), values.data());
  }
  /// @}

  /// Set the matrix uniform array value.
  // clang-format off
  template<std::ranges::sized_range Range>
    requires requires {
      []<glm::length_t Rows, glm::length_t Cols, glm::qualifier Qualifier>(
          meta::type<glm::mat<Rows, Cols, GLfloat, Qualifier>>) {
      }(meta::type_v<std::ranges::range_value_t<Range>>);
    }
  void set_uniform_array(GLint location, Range&& values) {
    // clang-format on
    TIT_ASSERT(location >= 0, "Invalid location!");
    const auto size = static_cast<GLsizei>(values.size());
    [&]<glm::length_t Rows, glm::length_t Cols, glm::qualifier Qualifier>(
        meta::type<glm::mat<Rows, Cols, GLfloat, Qualifier>>) {
      decltype(auto) packed_values = [&]() -> decltype(auto) {
        if constexpr (std::ranges::contiguous_range<Range> &&
                      detail_::one_of_(Qualifier,
                                       glm::packed_lowp,
                                       glm::packed_mediump,
                                       glm::packed_highp)) {
          return std::forward<Range>(values);
        } else {
          return values | std::views::transform([](const auto& value) {
                   return static_cast< //
                       glm::mat<Rows, Cols, GLfloat, glm::packed_highp>>(value);
                 });
        }
      }();
      if constexpr (Rows == Cols) {
        if constexpr (Rows == 2) {
          glUniformMatrix2fv(location,
                             size,
                             GL_FALSE,
                             glm::value_ptr(*packed_values.data()));
        } else if constexpr (Rows == 3) {
          glUniformMatrix3fv(location,
                             size,
                             GL_FALSE,
                             glm::value_ptr(*packed_values.data()));
        } else if constexpr (Rows == 4) {
          glUniformMatrix4fv(location,
                             size,
                             GL_FALSE,
                             glm::value_ptr(*packed_values.data()));
        } else {
          static_assert(
              meta::always_false<std::integral_constant<glm::length_t, Rows>>);
        }
      } else if constexpr (Rows == 2 && Cols == 3) {
        glUniformMatrix2x3fv(location,
                             size,
                             GL_FALSE,
                             glm::value_ptr(*packed_values.data()));
      } else if constexpr (Rows == 3 && Cols == 2) {
        glUniformMatrix3x2fv(location,
                             size,
                             GL_FALSE,
                             glm::value_ptr(*packed_values.data()));
      } else if constexpr (Rows == 3 && Cols == 4) {
        glUniformMatrix3x4fv(location,
                             size,
                             GL_FALSE,
                             glm::value_ptr(*packed_values.data()));
      } else if constexpr (Rows == 4 && Cols == 3) {
        glUniformMatrix4x3fv(location,
                             size,
                             GL_FALSE,
                             glm::value_ptr(*packed_values.data()));
      } else {
        static_assert(meta::always_false<
                      std::integer_sequence<glm::length_t, Rows, Cols>>);
      }
    }(meta::type_v<std::ranges::range_value_t<Range>>);
  }
#endif

}; // class BindProgram

} // namespace tit::view::gl
