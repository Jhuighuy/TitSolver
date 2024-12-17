/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

STORM_VULTURE_SHADER_(vertex, R"(
#version 330 core

layout(location = 0) in vec4 position_screen_space;

out vec2 texture_coordinates;

void main() {
  texture_coordinates = 0.5 * (position_screen_space.xy + 1.0);
  gl_Position = position_screen_space;
}
)")

STORM_VULTURE_SHADER_(fragment, R"(
#version 330 core

in vec2 texture_coordinates;

out vec4 fragment_color;

uniform sampler2D color_texture;

void main() {
  fragment_color = texture(color_texture, texture_coordinates);
}
)")
