# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_library(
  NAME
    view
  SOURCES
    "gl.hpp"
    "gl/buffer.hpp"
    "gl/debug.hpp"
    "gl/framebuffer.hpp"
    "gl/shader.hpp"
    "gl/texture.hpp"
    "gl/vertex_array.hpp"
    "gl/window.hpp"
    "gl/window.cpp"
    "Scene.hpp"
    "Visualizer.hpp"
    "test.cpp"
  DEPENDS
    tit::core
    GLEW::GLEW
    glfw
    glm::glm
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_wasm_binary(
  NAME
    view_wasm
  DESTINATION
    frontend/assets
  NATIVE_TARGET
    tit::view
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
