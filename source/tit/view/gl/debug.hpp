/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// IWYU pragma: private, include "tit/view/gl.hpp"
#pragma once

#include <GL/glew.h>

#include "tit/core/log.hpp"

namespace tit::view::gl {

/// RAII OpenGL debug output.
class DebugOutput {
public:

  /// Enable OpenGL debug output.
  /// Sticks to GL_ARB_debug_output extension since debug output
  /// is inside OpenGL since 4.3, and we are using 3.3.
  DebugOutput() {
    if (GLEW_ARB_debug_output != GL_TRUE) return;
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
    glDebugMessageCallbackARB(&on_message_, nullptr);
    TIT_INFO("OpenGL debug output enabled.");
  }

  /// Disable OpenGL debug output.
  ~DebugOutput() { // NOLINT(*-exception-escape)
    if (GLEW_ARB_debug_output != GL_TRUE) return;
    glDisable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
    TIT_INFO("OpenGL debug output disabled.");
  }

  DebugOutput(const DebugOutput&) = delete;
  auto operator=(const DebugOutput&) -> DebugOutput& = delete;

  DebugOutput(DebugOutput&&) = delete;
  auto operator=(DebugOutput&&) -> DebugOutput& = delete;

private:

  static void on_message_(GLenum source,
                          GLenum type,
                          GLuint id,
                          GLenum severity,
                          [[maybe_unused]] GLsizei length,
                          const GLchar* message,
                          [[maybe_unused]] const void* user_param) {
    const char* debug_error_source = nullptr;
    switch (source) {
      case GL_DEBUG_SOURCE_API_ARB: //
        debug_error_source = "API call";
        break;
      case GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB:
        debug_error_source = "window system API all";
        break;
      case GL_DEBUG_SOURCE_SHADER_COMPILER_ARB:
        debug_error_source = "shader compiler";
        break;
      case GL_DEBUG_SOURCE_THIRD_PARTY_ARB:
        debug_error_source = "third party API";
        break;
      case GL_DEBUG_SOURCE_APPLICATION_ARB:
        debug_error_source = "application";
        break;
      case GL_DEBUG_SOURCE_OTHER_ARB: //
        debug_error_source = "other";
        break;
      default: //
        debug_error_source = "unknown source";
        break;
    }

    const char* debug_type = nullptr;
    switch (type) {
      default:
      case GL_DEBUG_TYPE_OTHER_ARB: //
        debug_type = "other issue";
        break;
      case GL_DEBUG_TYPE_ERROR_ARB: //
        debug_type = "error";
        break;
      case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB:
        debug_type = "deprecated behavior";
        break;
      case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB:
        debug_type = "undefined behavior";
        break;
      case GL_DEBUG_TYPE_PORTABILITY_ARB: //
        debug_type = "portability issue";
        break;
      case GL_DEBUG_TYPE_PERFORMANCE_ARB: //
        debug_type = "performance issue";
        break;
    }

    switch (severity) {
      case GL_DEBUG_SEVERITY_HIGH_ARB:
        TIT_ERROR("OpenGL: {} {} {:#x}: {}",
                  debug_error_source,
                  debug_type,
                  id,
                  message);
        break;
      case GL_DEBUG_SEVERITY_MEDIUM_ARB:
        TIT_WARN("OpenGL: {} {} {:#x}: {}",
                 debug_error_source,
                 debug_type,
                 id,
                 message);
        break;
      case GL_DEBUG_SEVERITY_LOW_ARB:
      default:
        TIT_INFO("OpenGL: {} {} {:#x}: {}",
                 debug_error_source,
                 debug_type,
                 id,
                 message);
        break;
    }
  }

}; // class DebugOutput

} // namespace tit::view::gl
