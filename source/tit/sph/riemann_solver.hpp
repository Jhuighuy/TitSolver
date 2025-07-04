/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <tuple>

#include "tit/core/checks.hpp"
#include "tit/core/type.hpp"

#include "tit/sph/field.hpp"
#include "tit/sph/particle_array.hpp"

namespace tit::sph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Low-dissipation weakly-compressible Riemann solver (Zhang, Hu, Adams, 2017).
template<class Num>
class ZhangRiemannSolver final {
public:

  /// Set of particle fields that are required.
  static constexpr TypeSet required_fields{rho, v};

  /// Set of particle fields that are modified.
  static constexpr TypeSet modified_fields{/*empty*/};

  /// Construct a Riemann solver.
  constexpr explicit ZhangRiemannSolver(Num cs_0, Num rho_0) noexcept
      : cs_0_{cs_0}, rho_0_{rho_0} {
    TIT_ASSERT(cs_0_ > 0.0, "Reference sound speed must be positive!");
    TIT_ASSERT(rho_0_ > 0.0, "Reference density speed must be positive!");
  }

  /// Solve the Riemann problem.
  constexpr auto operator()(const Num& rho_a,
                            const Num& p_a,
                            const Num& v_a,
                            const Num& rho_b,
                            const Num& p_b,
                            const Num& v_b) const noexcept {
    const auto rho_ab = avg(rho_a, rho_b);
    const auto dv_ab = v_b - v_a;
    const auto dp_ab = p_b - p_a;
    const auto beta = std::clamp(3 * dv_ab, zero(dv_ab), cs_0_);
    const auto p_ast = avg(p_a, p_b) + dv_ab * beta * rho_ab / 2;
    const auto v_ast = avg(v_a, v_b) + dp_ab / (2 * rho_ab * cs_0_);
    return std::tuple{p_ast, v_ast};
  }

  /// Solve the Riemann problem.
  template<particle_view_n<Num, required_fields> PV>
  constexpr auto operator()(PV a, PV b) const noexcept {
    TIT_ASSERT(a != b, "Particles must be different!");
    const auto e_ab = normalize(r[a, b]);
    const auto [p_ast, v_ast] = (*this)( /// @todo Let's not embed a EOS here.
        rho[a],
        pow2(cs_0_) * (a.has_type(ParticleType::fixed) ?
                           std::max(rho[a], rho_0_) :
                           rho[a] - rho_0_),
        dot(v[a], e_ab),
        rho[b],
        pow2(cs_0_) * (a.has_type(ParticleType::fixed) ?
                           std::max(rho[b], rho_0_) :
                           rho[b] - rho_0_),
        dot(v[b], e_ab));
    return std::tuple{p_ast, v_ast * e_ab};
  }

private:

  Num cs_0_;
  Num rho_0_;

}; // class ZhangRiemannSolver

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Riemann solver type.
template<class RS>
concept riemann_solver = specialization_of<RS, ZhangRiemannSolver>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
