/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp> // IWYU pragma: keep

#include "tit/core/basic_types.hpp"

#include "tit/geom/point_range.hpp"

#include "tit/sph/field.hpp"

#include "tit/view/gl.hpp"
#include "tit/view/gl/window.hpp"
#include "tit/view/scene.hpp"

namespace tit::view {

// NOLINTBEGIN

template<class Mesh>
void visualize_mesh(const Mesh& particles) {
  using namespace sph;

  // Setup window.
  constexpr static const char* window_title = "Storm::Vulture Visualizer";
  constexpr static size_t window_width = 1920;
  constexpr static size_t window_height = 1080;
  gl::WindowSession session{};
  gl::Window window{session, window_title, window_width, window_height};
  const gl::BindWindow bind_window{window};
  const gl::DebugOutput debug_output{};
  glViewport(0, 0, window_width, window_height);

  // Setup framebuffer.
  /// @todo MSAA is broken with framebuffers!
  const gl::Texture2D<glm::vec4> color_texture{window_width, window_height};
  gl::Texture2D<glm::uvec2> entity_texture{window_width, window_height};
  gl::Buffer<glm::uvec2> entity_texture_pixel_buffer( // NOLINT
      window_width * window_height,
      gl::BufferUsage::dynamic_copy);
  gl::Framebuffer framebuffer{color_texture, entity_texture};
  gl::Buffer screen_quad_buffer{std::array{glm::vec2{+1.0F, +1.0F}, // NOLINT
                                           glm::vec2{+1.0F, -1.0F},
                                           glm::vec2{-1.0F, +1.0F},
                                           glm::vec2{+1.0F, -1.0F},
                                           glm::vec2{-1.0F, -1.0F},
                                           glm::vec2{-1.0F, +1.0F}}};
  gl::VertexArray screen_quad_vertex_array{screen_quad_buffer};
  gl::Program screen_quad_program{
#include "./ShaderScreenQuad.glsl"
  };

  static constexpr GLuint node_entity_type = 111;
  // static constexpr GLuint edge_entity_type = 222;
  // static constexpr GLuint cell_entity_type = 333;

  // Setup nodes.
  const gl::Buffer node_positions_buffer(
      r[particles] | std::views::transform([](const auto& rx) {
        return glm::vec2{rx[0], rx[1]};
      }));
  gl::VertexArray node_vertex_array{node_positions_buffer};
  gl::Program node_program{
#include "./ShaderNodes.glsl"
  };
  // Setup node data.
  gl::Buffer<GLuint> node_states_buffer(particles.size(),
                                        gl::BufferUsage::dynamic_draw);
  const gl::TextureBuffer node_states_texture_buffer{node_states_buffer};

  // Setup camera.
  scene::Camera camera{};
  camera.set_perspective(16.0 / 9.0);
  const auto reset_camera = [&]() {
    camera.transform() = scene::Transform{};
    const auto aabb = geom::compute_bbox(r[particles]);
    const auto center = aabb.center();
    camera.transform().translate(glm::vec3(center[0], center[1], 0.0F));
    const auto extents = aabb.extents();
    auto orbit = static_cast<float>(0.5 * glm::max(extents[0], extents[1]));
    camera.set_orbit(orbit);
  };
  reset_camera();
  // Reset the camera.
  window.on_key_up({gl::Key::r}, [&](gl::Key /*k*/, gl::Modifiers /*m*/) { //
    reset_camera();
  });
  // Translate the camera.
  window.on_key_down(gl::Key::w, [&](gl::Key /*k*/, gl::Modifiers /*m*/) {
    camera.transform().translate(glm::vec3{0.0, +0.05, 0.0});
  });
  window.on_key_down(gl::Key::a, [&](gl::Key /*k*/, gl::Modifiers /*m*/) {
    camera.transform().translate(glm::vec3{-0.05, 0.0, 0.0});
  });
  window.on_key_down(gl::Key::s, [&](gl::Key /*k*/, gl::Modifiers /*m*/) {
    camera.transform().translate(glm::vec3{0.0, -0.05, 0.0});
  });
  window.on_key_down(gl::Key::d, [&](gl::Key /*k*/, gl::Modifiers /*m*/) {
    camera.transform().translate(glm::vec3{+0.05, 0.0, 0.0});
  });
  // Rotate the camera.
  window.on_scroll([&](ssize_t /*delta_x*/, ssize_t delta_y) {
    camera.set_orbit(camera.orbit() - 0.25 * delta_y);
  });
  window.on_key_up(gl::Key::q, [&](gl::Key /*k*/, gl::Modifiers /*m*/) {
    camera.transform().rotate_degrees(glm::vec3{0.0F, +90.0F, 0.0F});
  });
  window.on_key_up(gl::Key::e, [&](gl::Key /*k*/, gl::Modifiers /*m*/) {
    camera.transform().rotate_degrees(glm::vec3{0.0F, -90.0F, 0.0F});
  });
  window.on_key_down(gl::Key::up, [&](gl::Key /*k*/, gl::Modifiers /*m*/) {
    camera.transform().rotate_degrees(glm::vec3{+0.5, 0.0, 0.0});
  });
  window.on_key_down(gl::Key::down, [&](gl::Key /*k*/, gl::Modifiers /*m*/) {
    camera.transform().rotate_degrees(glm::vec3{-0.5, 0.0, 0.0});
  });
  window.on_key_down(gl::Key::left, [&](gl::Key /*k*/, gl::Modifiers /*m*/) {
    camera.transform().rotate_degrees(glm::vec3{0.0, +0.5, 0.0});
  });
  window.on_key_down(gl::Key::right, [&](gl::Key /*k*/, gl::Modifiers /*m*/) {
    camera.transform().rotate_degrees(glm::vec3{0.0, -0.5, 0.0});
  });

  using NodeIndex = size_t;
  const auto select_node = [&](NodeIndex node, GLuint state) {
    node_states_buffer.set(node, state);
  };
  std::optional<NodeIndex> last_selected_node;
  const auto unselect_all = [&]() { select_node(*last_selected_node, 0); };

  window.main_loop([&] {
    // Render the screen into framebuffer.
    framebuffer.draw_into([&]() {
      glClearColor(0.2F, 0.2F, 0.3F, 1.0F);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      const auto view_projection_matrix = camera.view_projection_matrix();

      gl::BindProgram bind_program{node_program};
      bind_program.set_uniform(node_program["node_entity_type"],
                               node_entity_type);
      node_states_texture_buffer.bind(0);
      bind_program.set_uniform(node_program["node_states"], 0);
      bind_program.set_uniform(node_program["view_projection_matrix"],
                               view_projection_matrix);
      bind_program.set_uniform(node_program["point_size"],
                               glm::vec2{0.002f * 9.0F / 16.0F, 0.002f});
      node_vertex_array.draw(gl::DrawMode::points, particles.size());
    });

    // Query the pixels read.
    entity_texture.read_pixels(entity_texture_pixel_buffer);

    GLsync sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

    {
      glClearColor(0.0F, 0.0F, 0.0F, 1.0F);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      gl::BindProgram bind_program{screen_quad_program};
      color_texture.bind(0);
      entity_texture.bind(1);
      bind_program.set_uniform(screen_quad_program["color_texture"], 0);
      screen_quad_vertex_array.draw(gl::DrawMode::triangles, 6);
    }

    glClientWaitSync(sync, GL_SYNC_FLUSH_COMMANDS_BIT, UINT64_MAX);

    const auto entity = entity_texture_pixel_buffer.get(
        window.cursor_pos_y() * window_width + window.cursor_pos_x());
    const auto entity_type = entity.x;
    const auto entity_index = entity.y;
    unselect_all();
    switch (entity_type) {
      case node_entity_type: {
        last_selected_node.emplace(entity_index);
        select_node(*last_selected_node, 1);
        break;
      }
    }
  });
}

// NOLINTEND

} // namespace tit::view
