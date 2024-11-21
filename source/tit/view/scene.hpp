/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

// NOLINTBEGIN(misc-header-include-cycle)

#include <algorithm>

#define GLM_ENABLE_EXPERIMENTAL 1
#include <glm/glm.hpp>                  // IWYU pragma: keep
#include <glm/gtc/matrix_transform.hpp> // IWYU pragma: keep
#include <glm/gtx/quaternion.hpp>       // IWYU pragma: keep

#include "tit/view/gl.hpp"

namespace tit::view::scene {

/// Scene entity transform.
class Transform final {
private:

  glm::vec3 position_{};            // NOLINT
  glm::quat rotation_{glm::vec3{}}; // NOLINT
  glm::vec3 scale_{1.0, 1.0, 1.0};  // NOLINT

public:

  /// Construct the transform.
  constexpr Transform() = default;

  /// Transform position.
  constexpr auto position() const noexcept -> const glm::vec3& {
    return position_;
  }

  /// Transform rotation.
  /// @{
  constexpr auto rotation() const noexcept -> const glm::quat& {
    return rotation_;
  }
  auto rotation_degrees() const noexcept -> glm::vec3 {
    return glm::degrees(glm::eulerAngles(rotation_)); // NOLINT
  }
  /// @}

  /// Transform scale.
  constexpr auto scale() const noexcept -> const glm::vec3& {
    return scale_;
  }

  /// Translate the transform.
  constexpr void translate(const glm::vec3& delta) noexcept {
    position_ += delta;
  }

  /// Rotate the transform.
  /// @{
  constexpr void rotate(const glm::quat& delta) noexcept {
    rotation_ *= delta;
  }
  constexpr void rotate_degrees(const glm::vec3& delta_degrees) noexcept {
    rotation_ *= glm::quat{glm::radians(delta_degrees)}; // NOLINT
  }
  /// @}

  /// Rescale the transform.
  /// @{
  constexpr void rescale(float factor) noexcept {
    scale_ *= factor;
  }
  constexpr void rescale(const glm::vec3& factor) noexcept {
    scale_ *= factor;
  }
  /// @}

  /// Transform model matrix.
  /// @todo GLM's matmul operator is not constexpr?
  auto model_matrix() const noexcept -> glm::mat4 {                  // NOLINT
    auto model_matrix = glm::translate(glm::mat4(1.0F), position_) * // NOLINT
                        glm::toMat4(rotation_) *                     // NOLINT
                        glm::scale(glm::mat4(1.0F), scale_);         // NOLINT
    return model_matrix;
  }

}; // class Transform

/// Scene camera.
class Camera final {
private:

  float near_ = 0.01F, far_ = 0.99F;
  float orbit_ = 1.0F;
  Transform transform_;
  glm::mat4 projection_matrix_{};

public:

  /// Construct the camera.
  constexpr Camera() = default;

  /// Construct the camera with @p parent.
  constexpr explicit Camera(const Transform& parent) : transform_{parent} {}

  /// Camera transform.
  /// @{
  constexpr auto transform() noexcept -> Transform& {
    return transform_;
  }
  constexpr auto transform() const noexcept -> const Transform& {
    return transform_;
  }
  /// @}

  /// Camera orbit.
  /// @{
  constexpr auto orbit() const noexcept -> float {
    return orbit_;
  }
  constexpr void set_orbit(float orbit) noexcept {
    orbit_ = std::clamp(orbit, near_, far_);
  }
  /// @}

  /// Set perspective projection matrix.
  void set_perspective(float aspect_ratio,
                       float fov_degrees = 60.0F,
                       float the_near = 0.001F,
                       float the_far = 1000.0F) noexcept {
    // Beware: GLM's documentations says that FOV is in degrees,
    // but actually it is in radians!
    projection_matrix_ = glm::perspective(glm::radians(fov_degrees), // NOLINT
                                          aspect_ratio,
                                          the_near,
                                          the_far);
    near_ = the_near, far_ = the_far;
  }

  /// Set orthographic projection matrix.
  void set_ortographic(float aspect_ratio,
                       float height = 1.0F,
                       float the_near = 0.001F,
                       float the_far = 1000.0F) noexcept {
    const float width = aspect_ratio * height;
    projection_matrix_ = glm::ortho(-0.5F * width, // NOLINT
                                    +0.5F * width,
                                    -0.5F * height,
                                    +0.5F * height,
                                    the_near,
                                    the_far);
    near_ = the_near, far_ = the_far;
  }

  /// Camera view-projection matrix.
  /// @todo GLM's matmul operator is not constexpr?
  auto view_projection_matrix() const noexcept -> glm::mat4 {
    const glm::mat4 view_matrix = glm::inverse( // NOLINT
        transform_.model_matrix() *
        glm::translate(glm::mat4(1.0F), glm::vec3(0.0F, 0.0F, orbit_)));
    return projection_matrix_ * view_matrix;
  }

}; // class Camera

/// Scene mesh renderer.
class MeshRenderer final {
private:

  Transform transform_;
  // gl::Mesh mesh_{};
  gl::Program program_;

public:

  /// Construct the mesh renderer.
  MeshRenderer() = default;

  /// Construct the mesh renderer with @p parent.
  explicit MeshRenderer(const Transform& parent) : transform_{parent} {}

  /// Mesh renderer transform.
  /// @{
  constexpr auto transform() noexcept -> Transform& {
    return transform_;
  }
  constexpr auto transform() const noexcept -> const Transform& {
    return transform_;
  }
  /// @}

  /// Mesh renderer mesh.
  /// @{
  // constexpr gl::Mesh& mesh() noexcept {
  //   return mesh_;
  // }
  // constexpr const gl::Mesh& mesh() const noexcept {
  //   return mesh_;
  // }
  /// @}

  /// Mesh renderer program.
  /// @{
  constexpr auto program() noexcept -> gl::Program& {
    return program_;
  }
  constexpr auto program() const noexcept -> const gl::Program& {
    return program_;
  }
  /// @}

  /// Draw the mesh.
  /// @{
  // void draw(const Camera& camera, const gl::Program& program,
  //           GLenum mode = GL_TRIANGLES) const {
  //   // gl::BindProgram bind_program{program};
  //   mesh_.draw(mode);
  // }
  // void draw(const Camera& camera, GLenum mode = GL_TRIANGLES) const {
  //   draw(camera, program_, mode);
  // }
  /// @}

}; // class Camera

} // namespace tit::view::scene

// NOLINTEND(misc-header-include-cycle)
