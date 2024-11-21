/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

STORM_VULTURE_SHADER_(vertex, R"(
#version 330 core

layout(location = 0) in vec4 position_world_space;

uniform mat4 view_projection_matrix;

void main() {
  gl_Position = view_projection_matrix * position_world_space;
}
)")

STORM_VULTURE_SHADER_(geometry, R"(
#version 330 core

layout(points) in;

layout(triangle_strip, max_vertices = 4) out;
out vec2 position_model_space;
out vec4 color;

uniform vec2 point_size;

void main() {
  vec4 position_screen_space = gl_in[0].gl_Position;

  gl_PrimitiveID = gl_PrimitiveIDIn;

  position_model_space = vec2(-1.0, -1.0);
  gl_Position = position_screen_space +
                vec4(point_size * position_model_space, 0.0, 0.0);
  EmitVertex();

  position_model_space = vec2(+1.0, -1.0);
  gl_Position = position_screen_space +
                vec4(point_size * position_model_space, 0.0, 0.0);
  EmitVertex();

  position_model_space = vec2(-1.0, +1.0);
  gl_Position = position_screen_space +
                vec4(point_size * position_model_space, 0.0, 0.0);
  EmitVertex();

  position_model_space = vec2(+1.0, +1.0);
  gl_Position = position_screen_space +
                vec4(point_size * position_model_space, 0.0, 0.0);
  EmitVertex();

  EndPrimitive();
}
)")

STORM_VULTURE_SHADER_(fragment, R"(
#version 330 core

in vec2 position_model_space;
in vec4 color;

layout(location = 0) out vec4 fragment_color;
layout(location = 1) out uvec2 fragment_entity;

const vec4 regular_color = vec4(0.9, 0.9, 0.9, 1.0);
const vec4 selected_color = vec4(0.9, 0.1, 0.9, 1.0);

uniform uint node_entity_type;
uniform usamplerBuffer node_states;

void main() {
  if (length(position_model_space) > 1.0) { discard; }
  uint state = texelFetch(node_states, gl_PrimitiveID).r;
  fragment_color = state == uint(0) ? regular_color : selected_color;
  fragment_entity = uvec2(node_entity_type, gl_PrimitiveID);
}
)")
