/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// IWYU pragma: private, include "tit/view/gl.hpp"
#pragma once

#include <array>
#include <concepts>

#include <GL/glew.h>

#include "tit/core/basic_types.hpp"
#include "tit/core/exception.hpp"

#include "tit/view/gl/texture.hpp"

namespace tit::view::gl {

/// OpenGL framebuffer.
class Framebuffer {
private:

  GLuint framebuffer_id_{};

public:

  /// Construct a framebuffer.
  Framebuffer() {
    glGenFramebuffers(1, &framebuffer_id_);
  }

  /// Construct a framebuffer with @p textures.
  template<pixel... Pixels>
  explicit Framebuffer(const Texture2D<Pixels>&... textures) : Framebuffer{} {
    assign(textures...);
  }

  /// Move-construct a framebuffer.
  Framebuffer(Framebuffer&&) = default;
  /// Move-assign the framebuffer.
  auto operator=(Framebuffer&&) -> Framebuffer& = default;

  Framebuffer(const Framebuffer&) = delete;
  auto operator=(const Framebuffer&) -> Framebuffer& = delete;

  /// Destruct the framebuffer.
  ~Framebuffer() {
    glDeleteFramebuffers(1, &framebuffer_id_);
  }

  /// Cast to framebuffer ID.
  [[nodiscard]] constexpr operator GLuint() const noexcept {
    return framebuffer_id_;
  }

  /// Build the framebuffer with @p textures.
  template<pixel... Pixels>
  void assign(const Texture2D<Pixels>&... textures) {
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_id_);
    std::array<GLenum, sizeof...(textures)> attachments{};
    const auto attach_texture =
        [&]<class Pixel>(size_t index, const Texture2D<Pixel>& texture) {
          /// @todo Depth attachments!
          attachments[index] = GL_COLOR_ATTACHMENT0 + index;
          glFramebufferTexture(GL_FRAMEBUFFER,
                               GL_COLOR_ATTACHMENT0 + index,
                               texture,
                               /*level*/ 0);
        };
    size_t index = 0;
    (attach_texture(index++, textures), ...);
    glDrawBuffers(GLsizei{sizeof...(textures)}, attachments.data());
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
      TIT_THROW("Failed to initialize framebuffer!");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }

  /// Draw to framebuffer.
  template<std::invocable DrawFunc>
  void draw_into(DrawFunc draw_func) {
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_id_);
    draw_func();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }

}; // class Framebuffer

} // namespace tit::view::gl
