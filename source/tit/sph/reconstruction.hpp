/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <array>
#include <concepts>
#include <tuple>

#include "tit/core/basic_types.hpp"
#include "tit/core/type.hpp"
#include "tit/core/vec.hpp"
#include "tit/sph/field.hpp"
#include "tit/sph/particle_array.hpp"

namespace tit::sph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// No reconstruction scheme.
class NoReconstruction final {
public:

  /// Set of particle fields that are required.
  static constexpr TypeSet required_fields{/*empty*/};

  /// Set of particle fields that are modified.
  static constexpr TypeSet modified_fields{/*empty*/};

}; // class NoReconstruction

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Abstract reconstruction scheme.
class Reconstruction {
public:

  /// Set of particle fields that are required.
  static constexpr TypeSet required_fields{/*empty*/};

  /// Set of particle fields that are modified.
  static constexpr TypeSet modified_fields{/*empty*/};

  /// Reconstruct the scalar states on the left and right side of the interface.
  template<field Field, field Gradient, particle_view<Field{}, Gradient{}> PV>
    requires is_vec_v<particle_field_t<Gradient{}, PV>>
  constexpr auto operator()(this auto& self,
                            Field q,
                            Gradient grad_q,
                            PV a,
                            PV b) noexcept {
    const auto r_ab = r[a, b];
    return self.reconstruct(std::array{
        q[a] - dot(grad_q[a], r_ab),
        q[a],
        q[b],
        q[b] + dot(grad_q[b], r_ab),
    });
  }

  /// Reconstruct the vector states on the left and right side of the interface.
  template<field Field, field Gradient, particle_view<Field{}, Gradient{}> PV>
    requires is_mat_v<particle_field_t<Gradient{}, PV>>
  constexpr auto operator()(this auto& self,
                            Field q,
                            Gradient grad_q,
                            PV a,
                            PV b) noexcept {
    const auto r_ab = r[a, b];
    const auto e_ab = normalize(r_ab);
    const auto [q_ar, q_br] = self.reconstruct(std::array{
        dot(e_ab, q[a] - grad_q[a] * r_ab),
        dot(e_ab, q[a]),
        dot(e_ab, q[b]),
        dot(e_ab, q[b] + grad_q[b] * r_ab),
    });
    return std::tuple{q_ar * e_ab, q_br * e_ab};
  }

}; // class Reconstruction

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// WENO-3 reconstruction scheme.
class WENO3Reconstruction final : public Reconstruction {
public:

  template<class Num>
  static constexpr auto reconstruct(std::array<Num, 4> q) noexcept {
    const auto q1 = reconstruct_forward_(q);
    std::ranges::reverse(q);
    const auto q2 = reconstruct_forward_(q);
    return std::tuple{q1, q2};
  }

private:

  template<class Num>
  static constexpr auto reconstruct_forward_(
      const std::array<Num, 4>& q) noexcept -> Num {
    constexpr size_t i = 1;

    // Compute the smoothness indicators and weights.
    static constexpr Num eps{1.0e-6};
    static constexpr Num d_1{2.0 / 3.0};
    static constexpr Num d_2{1.0 / 3.0};
    const auto beta_1 = pow2(q[i - 1] - q[i]);
    const auto beta_2 = pow2(q[i] - q[i + 1]);
    const auto alpha_1 = d_1 / pow2(beta_1 + eps);
    const auto alpha_2 = d_2 / pow2(beta_2 + eps);
    const auto alpha_sum = alpha_1 + alpha_2;
    const auto w_1 = alpha_1 / alpha_sum;
    const auto w_2 = alpha_2 / alpha_sum;

    // Compute the reconstructed values on the different stencils.
    const auto q_12_1 = (3 * q[i] - q[i - 1]) / 2;
    const auto q_12_2 = (q[i] + q[i + 1]) / 2;

    // Interpolate the values.
    return w_1 * q_12_1 + w_2 * q_12_2;
  }

}; // class WENO3Reconstruction

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Reconstruction scheme type.
template<class R>
concept reconstruction_scheme = std::same_as<R, NoReconstruction> || //
                                std::same_as<R, WENO3Reconstruction>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
