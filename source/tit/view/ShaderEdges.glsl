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

STORM_VULTURE_SHADER_(fragment, R"(
#version 330 core

in vec2 position_model_space;

layout(location = 0) out vec4 fragment_color;
layout(location = 1) out uvec2 fragment_entity;

uniform uint edge_entity_type;
uniform usamplerBuffer edge_states;

const vec4 regular_color = vec4(0.8, 0.8, 0.8, 1.0);
const vec4 selected_color = vec4(0.2, 0.8, 0.8, 1.0);

void main() {
  uint state = texelFetch(edge_states, gl_PrimitiveID).r;
  fragment_color = state == uint(0) ? regular_color : selected_color;
  fragment_entity = uvec2(edge_entity_type, gl_PrimitiveID);
}
)")
