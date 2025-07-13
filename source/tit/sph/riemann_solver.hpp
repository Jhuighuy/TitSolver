/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
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
  static constexpr TypeSet required_fields{rho, grad_rho, v, grad_v, L};

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

  /// Reconstruct the values using WENO-3 scheme.
  static constexpr auto reconstruct(const std::array<Num, 4>& q) noexcept
      -> Num {
    constexpr size_t i = 1;

    // Compute the smoothness indicators and weights.
    constexpr Num eps{1.0e-6};
    constexpr Num d_1{2.0 / 3.0};
    constexpr Num d_2{1.0 / 3.0};
    const auto beta_1 = pow2(q[i - 1] - q[i]);
    const auto beta_2 = pow2(q[i] - q[i + 1]);
    const auto alpha_1 = d_1 / pow2(beta_1 + eps);
    const auto alpha_2 = d_2 / pow2(beta_2 + eps);
    const auto alpha_sum = alpha_1 + alpha_2;
    const auto w_1 = alpha_1 / alpha_sum;
    const auto w_2 = alpha_2 / alpha_sum;

    // Compute the reconstructed values on the different stencils.
    const auto q12_1 = 3 * q[i] / 2 - q[i - 1] / 2;
    const auto q12_2 = (q[i] + q[i + 1]) / 2;

    // Interpolate the values.
    return w_1 * q12_1 + w_2 * q12_2;
  }

  /// Solve the Riemann problem.
  template<particle_view_n<Num, required_fields> PV>
  constexpr auto operator()(PV a, PV b) const noexcept {
    TIT_ASSERT(a != b, "Particles must be different!");

    const auto r_ab = r[a, b];
    const auto e_ab = normalize(r_ab);

    std::array rhos{rho[a] - dot(grad_rho[a], r_ab),
                    rho[a],
                    rho[b],
                    rho[b] + dot(grad_rho[b], r_ab)};
    std::array vs{dot(e_ab, v[a] - grad_v[a] * r_ab),
                  dot(e_ab, v[a]),
                  dot(e_ab, v[b]),
                  dot(e_ab, v[b] + grad_v[b] * r_ab)};

    const auto rho_a = reconstruct(rhos);
    const auto v_a = reconstruct(vs);

    std::ranges::reverse(rhos);
    std::ranges::reverse(vs);

    const auto rho_b = reconstruct(rhos);
    const auto v_b = reconstruct(vs);

    const auto [p_ast, v_ast] = (*this)( /// @todo Let's not embed a EOS here.
        rho_a,
        pow2(cs_0_) * (a.has_type(ParticleType::fixed) ?
                           std::max(rho_a, rho_0_) :
                           rho_a - rho_0_),
        v_a,
        rho_b,
        pow2(cs_0_) * (a.has_type(ParticleType::fixed) ?
                           std::max(rho_b, rho_0_) :
                           rho_b - rho_0_),
        v_b);
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
