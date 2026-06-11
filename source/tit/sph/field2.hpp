/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <cstdint>
#include <string_view>

namespace tit::sph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Field IDs.
enum FieldID : std::uint8_t {
  h,              ///< Radius.
  m,              ///< Mass.
  gamma,          ///< Volume fraction.
  grad_gamma,     ///< Volume fraction gradient.
  phi,            ///< Free surface indicator.
  rho,            ///< Density.
  grad_rho,       ///< Density gradient.
  drho_dt,        ///< Density time derivative.
  p,              ///< Pressure.
  cs,             ///< Sound speed.
  v,              ///< Velocity.
  grad_v,         ///< Velocity gradient.
  dv_dt,          ///< Velocity time derivative.
  r,              ///< Position.
  N,              ///< Normal vector, ~ ∇1/|∇1|.
  L,              ///< Renormalization matrix, ~ (∇r)⁻¹.
  dr,             ///< Shift vector, ~ C ∇1.
  scratch_scalar, ///< Scratch scalar field.
  scratch_vector, ///< Scratch vector field.
  scratch_matrix, ///< Scratch matrix field.
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Field rank.
enum FieldRank : std::uint8_t {
  scalar, ///< Scalar field.
  vector, ///< Vector field.
  matrix, ///< Matrix field.
};

/// Rank of a field.
auto field_rank(FieldID id) noexcept -> FieldRank;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Name of a field.
auto field_name(FieldID id) noexcept -> std::string_view;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
