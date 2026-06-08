/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>

#include "tit/core/assert.hpp"

namespace tit::sph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Buoyancy model that contributes no extra acceleration.
template<class Num>
class NoBuoyancy final {
public:

  /// Buoyancy acceleration coefficient.
  constexpr auto coeff(Num /*T*/) const noexcept -> Num {
    return {};
  }

}; // class NoBuoyancy

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Boussinesq buoyancy model with a constant thermal expansion coefficient.
template<class Num>
class BoussinesqBuoyancy final {
public:

  /// Construct a Boussinesq buoyancy model.
  constexpr explicit BoussinesqBuoyancy(Num alpha_T, Num T_ref) noexcept
      : alpha_T_{alpha_T}, T_ref_{T_ref} {
    TIT_ASSERT(alpha_T_ >= Num{0},
               "Thermal expansion coefficient must be non-negative!");
  }

  /// Buoyancy acceleration coefficient.
  constexpr auto coeff(Num T) const noexcept -> Num {
    return Num{1} + alpha_T_ * (T - T_ref_);
  }

private:

  Num alpha_T_;
  Num T_ref_;

}; // class BoussinesqBuoyancy

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Buoyancy model type.
template<class BM, class Num>
concept buoyancy_model = std::same_as<BM, NoBuoyancy<Num>> ||
                         std::same_as<BM, BoussinesqBuoyancy<Num>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
