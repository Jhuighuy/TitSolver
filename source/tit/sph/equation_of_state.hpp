/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>

#include "tit/core/assert.hpp"
#include "tit/core/math.hpp"

namespace tit::sph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Tait equation equation of state.
template<class Num>
class TaitEquationOfState final {
public:

  /// Construct an equation of state.
  ///
  /// @param cs_0  Reference sound speed,.
  /// @param rho_0 Reference density.
  /// @param xi    Equation of state index.
  constexpr explicit TaitEquationOfState(Num cs_0,
                                         Num rho_0,
                                         Num xi = Num{7}) noexcept
      : cs_0_{cs_0}, rho_0_{rho_0}, xi_{xi} {
    TIT_ASSERT(cs_0_ > Num{0}, "Reference sound speed must be positive!");
    TIT_ASSERT(rho_0_ > Num{0}, "Reference density must be positive!");
    TIT_ASSERT(xi_ > Num{1}, "Polytropic index must be greater than 1!");
  }

  /// Compute pressure from density.
  constexpr auto pressure_from_density(const Num& rho) const noexcept -> Num {
    const auto B = rho_0_ * pow2(cs_0_) / xi_;
    return B * (pow(rho / rho_0_, xi_) - 1);
  }

  /// Compute density from pressure.
  constexpr auto density_from_pressure(const Num& p) const noexcept -> Num {
    const auto B = rho_0_ * pow2(cs_0_) / xi_;
    return rho_0_ * pow(1 + p / B, inverse(xi_));
  }

  /// Compute sound speed from density.
  constexpr auto sound_speed_from_density(const Num& rho) const noexcept
      -> Num {
    return cs_0_ * pow(rho / rho_0_, (xi_ - 1) / 2);
  }

  /// Compute pressure-density potential H(ρ) = ∫ cs^2(ζ)/ζ dζ from ρ_0 to ρ.
  constexpr auto potential_from_density(const Num& rho) const noexcept -> Num {
    const auto xi_1 = xi_ - 1;
    return pow2(cs_0_) * pow(rho / rho_0_, xi_1) / xi_1;
  }

  /// Compute density from pressure-density potential H(ρ).
  constexpr auto density_from_potential(const Num& H) const noexcept -> Num {
    const auto xi_1 = xi_ - 1;
    return rho_0_ * pow(xi_1 * H / pow2(cs_0_), inverse(xi_1));
  }

private:

  Num cs_0_;
  Num rho_0_;
  Num xi_;

}; // class TaitEquationOfState

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Linear Tait equation equation of state.
template<class Num>
class LinearTaitEquationOfState final {
public:

  /// Construct an equation of state.
  ///
  /// @param cs_0  Reference sound speed.
  /// @param rho_0 Reference density.
  constexpr LinearTaitEquationOfState(Num cs_0, Num rho_0) noexcept
      : cs_0_{cs_0}, rho_0_{rho_0} {
    TIT_ASSERT(cs_0_ > Num{0}, "Reference sound speed must be positive!");
    TIT_ASSERT(rho_0_ > Num{0}, "Reference density must be positive!");
  }

  /// Compute pressure from density.
  constexpr auto pressure_from_density(const Num& rho) const noexcept -> Num {
    return pow2(cs_0_) * (rho - rho_0_);
  }

  /// Compute density from pressure.
  constexpr auto density_from_pressure(const Num& p) const noexcept -> Num {
    return rho_0_ + p / pow2(cs_0_);
  }

  /// Compute sound speed from density.
  constexpr auto sound_speed_from_density(const Num& /*rho*/) const noexcept
      -> const Num& {
    return cs_0_;
  }

  /// Compute pressure-density potential H(ρ) = ∫ cs^2(ζ)/ζ dζ from ρ_0 to ρ.
  constexpr auto potential_from_density(const Num& rho) const noexcept -> Num {
    return pow2(cs_0_) * log(rho / rho_0_);
  }

  /// Compute density from pressure-density potential H(ρ).
  constexpr auto density_from_potential(const Num& H) const noexcept -> Num {
    return rho_0_ * exp(H / pow2(cs_0_));
  }

private:

  Num cs_0_;
  Num rho_0_;

}; // class LinearTaitEquationOfState

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Equation of state type.
template<class EOS, class Num>
concept equation_of_state = std::same_as<EOS, TaitEquationOfState<Num>> ||
                            std::same_as<EOS, LinearTaitEquationOfState<Num>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
