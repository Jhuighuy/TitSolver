/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

STORM_VULTURE_SHADER_(vertex, R"(
#version 330 core

layout(location = 0) in vec4 position_world_space;
layout(location = 1) in float in_node_value;

out float node_value;

uniform mat4 view_projection_matrix;

void main() {
  node_value = in_node_value;
  gl_Position = view_projection_matrix * position_world_space;
}
)")

STORM_VULTURE_SHADER_(fragment, R"(
#version 330 core

in float node_value;

layout(location = 0) out vec4 fragment_color;
layout(location = 1) out uvec2 fragment_entity;

uniform uint cell_entity_type;
uniform usamplerBuffer cell_states;
uniform bool use_node_value;
uniform samplerBuffer cell_values;

const vec4 regular_color = vec4(1.0, 1.0, 1.0, 1.0);
const vec4 selected_color = vec4(0.8, 0.8, 0.2, 1.0);

float get_value() {
  if (use_node_value) {
    return node_value;
  } else {
    float cell_value = texelFetch(cell_values, gl_PrimitiveID).r;
    return cell_value;
  }
}

void main() {
  float value = get_value();
  value = clamp(value, 0.0, 1.0);
  fragment_color.rgb = mix(vec3(0.0, 0.0, 1.0), vec3(1.0, 0.0, 0.0), value);
  fragment_color.a = 1.0;
  uint state = texelFetch(cell_states, gl_PrimitiveID).r;
  if (state != uint(0)) {
    fragment_color = mix(fragment_color, selected_color, 0.5);
  }
  fragment_entity = uvec2(cell_entity_type, gl_PrimitiveID);
}
)")
