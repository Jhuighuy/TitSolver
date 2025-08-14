/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

uniform sampler2D colorMap;
uniform float minValue;
uniform float maxValue;

uniform vec3 ambientLightColor;
uniform vec3 pointLightColor;
uniform vec3 pointLightPosition;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

varying float vScalar;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void main() {
  // Sample the value.
  float t = clamp((vScalar - minValue) / (maxValue - minValue), 0.0, 1.0);
  vec4 color = texture2D(colorMap, vec2(t, 0.5));

  // Circular point mask.
  float r = length(gl_PointCoord - vec2(0.5));
  if (r > 0.5) discard;

  vec2 pos = gl_PointCoord.xy - vec2(0.5);
  vec3 normal = normalize(vec3(pos, sqrt(1.0 - dot(pos, pos))));
  vec3 lightDirection = normalize(pointLightPosition - vec3(gl_FragCoord));
  float lightIntensity = max(dot(lightDirection, normal), 0.0);
  vec3 light = ambientLightColor + pointLightColor * lightIntensity;

  // Assign the color.
  gl_FragColor = vec4(color.rgb * light, color.a);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
