/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <tuple>

#include "tit/core/checks.hpp"
#include "tit/core/type.hpp"
#include "tit/sph/field.hpp"
#include "tit/sph/particle_array.hpp"

namespace tit::sph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Zhang, Hu, Adams low-dissipation weakly-compressible Riemann solver.
template<class Num>
class ZhangRiemannSolver final {
public:

  /// Set of particle fields that are required.
  static constexpr TypeSet required_fields{rho, v, p};

  /// Set of particle fields that are modified.
  static constexpr TypeSet modified_fields{/*empty*/};

  /// Construct Riemann solver.
  constexpr explicit ZhangRiemannSolver(Num cs_0) noexcept : cs_0_{cs_0} {
    TIT_ASSERT(cs_0_ > 0.0, "Reference sound speed must be positive!");
  }

  /// Solve Riemann problem.
  template<particle_view_n<Num, required_fields> PV>
  constexpr auto operator()(PV a, PV b) const noexcept {
    TIT_ASSERT(a != b, "Particles must be different!");
    const auto e_ab = normalize(r[a, b]);
    const auto rho_ab = rho.avg(a, b);
    const auto dp_ab = p[b, a];
    const auto dv_ab = dot(v[b, a], e_ab);
    const auto beta = std::clamp(3 * dv_ab, zero(dv_ab), cs_0_);
    const auto p_ast = p.avg(a, b) + beta * rho_ab * dv_ab / 2;
    const auto v_ast = v.avg(a, b) + dp_ab / (2 * rho_ab * cs_0_) * e_ab;
    return std::tuple{p_ast, v_ast};
  }

private:

  Num cs_0_;

}; // class ZhangRiemannSolver

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Riemann solver type.
template<class RS>
concept riemann_solver = specialization_of<RS, ZhangRiemannSolver>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
