/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <tuple>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/mat.hpp"
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
  static constexpr TypeSet fields{/*empty*/};

  /// Reconstruct states on the left and right side of the interface.
  template<field Field, field Gradient, particle_view<Field{}, Gradient{}> PV>
  static constexpr auto operator()(Field q,
                                   Gradient /*grad_q*/,
                                   PV a,
                                   PV b) noexcept {
    return std::tuple{q[a], q[b]};
  }

}; // class NoReconstruction

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Abstract reconstruction scheme.
class Reconstruction {
public:

  /// Set of particle fields that are required.
  static constexpr TypeSet fields{r};

  /// Reconstruct the scalar states on the left and right side of the interface.
  template<field Field, field Gradient, particle_view<Field{}, Gradient{}> PV>
  constexpr auto operator()(this const auto& self,
                            Field q,
                            Gradient grad_q,
                            PV a,
                            PV b) noexcept {
    return self.reconstruct_from_vals_and_grads(r[a, b],
                                                q[a],
                                                grad_q[a],
                                                q[b],
                                                grad_q[b]);
  }

  /// Reconstruct the scalar states on the left and right side of the interface
  /// from the values and gradients at the particle positions.
  template<class Num, size_t Dim>
  constexpr auto reconstruct_from_vals_and_grads(
      this const auto& self,
      const Vec<Num, Dim>& r_ab,
      const Num& q_a,
      const Vec<Num, Dim>& grad_q_a,
      const Num& q_b,
      const Vec<Num, Dim>& grad_q_b) noexcept -> std::tuple<Num, Num> {
    return self.reconstruct_from_vals_on_grid(q_a - dot(grad_q_a, r_ab),
                                              q_a,
                                              q_b,
                                              q_b + dot(grad_q_b, r_ab));
  }

  /// Reconstruct the vector states on the left and right side of the interface
  /// from the values and gradients at the particle positions.
  template<class Num, size_t Dim>
  constexpr auto reconstruct_from_vals_and_grads(
      this const auto& self,
      const Vec<Num, Dim>& r_ab,
      const Vec<Num, Dim>& q_a,
      const Mat<Num, Dim>& grad_q_a,
      const Vec<Num, Dim>& q_b,
      const Mat<Num, Dim>& grad_q_b) noexcept
      -> std::tuple<Vec<Num, Dim>, Vec<Num, Dim>> {
    const auto e_ab = normalize(r_ab);
    const auto [q_l, q_r] =
        self.reconstruct_from_vals_on_grid(dot(e_ab, q_a - grad_q_a * r_ab),
                                           dot(e_ab, q_a),
                                           dot(e_ab, q_b),
                                           dot(e_ab, q_b + grad_q_b * r_ab));
    return std::tuple{q_l * e_ab, q_r * e_ab};
  }

  /// Reconstruct states on the left and right side of the interface from four
  /// consecutive values of the scalar state on the uniform grid.
  template<class Num>
  constexpr auto reconstruct_from_vals_on_grid(this const auto& self,
                                               const Num& q_0,
                                               const Num& q_1,
                                               const Num& q_2,
                                               const Num& q_3) noexcept {
    return std::tuple{
        self.reconstruct_from_vals_on_grid_forwards(q_0, q_1, q_2),
        self.reconstruct_from_vals_on_grid_forwards(q_3, q_2, q_1),
    };
  }

}; // class Reconstruction

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// WENO-3 reconstruction scheme.
template<class Num>
class WENO3Reconstruction final : public Reconstruction {
public:

  /// Construct WENO-3 reconstruction scheme.
  constexpr explicit WENO3Reconstruction(Num eps = Num{1.0e-6}) noexcept
      : eps_{eps} {
    TIT_ASSERT(eps_ > 0.0, "Smoothness parameter must be positive!");
  }

  /// Reconstruct state on the left side of the interface from three consecutive
  /// values of the scalar state on the uniform grid.
  constexpr auto reconstruct_from_vals_on_grid_forwards(
      const Num& q_0,
      const Num& q_1,
      const Num& q_2) const noexcept -> Num {
    // Linear reconstruction.
    const auto v_0 = (3 * q_1 - q_0) / 2;
    const auto v_1 = (q_1 + q_2) / 2;

    // Smoothness indicators.
    const auto beta_0 = pow2(q_0 - q_1);
    const auto beta_1 = pow2(q_1 - q_2);

    // Weights.
    static constexpr Num d_0{2.0 / 3.0};
    static constexpr Num d_1{1.0 / 3.0};
    const auto alpha_0 = d_0 / pow2(beta_0 + eps_);
    const auto alpha_1 = d_1 / pow2(beta_1 + eps_);
    const auto alpha_sum = alpha_0 + alpha_1;
    const auto w_0 = alpha_0 / alpha_sum;
    const auto w_1 = alpha_1 / alpha_sum;

    // Interpolate the values.
    return w_0 * v_0 + w_1 * v_1;
  }

private:

  Num eps_;

}; // class WENO3Reconstruction

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Reconstruction scheme type.
template<class R, class Num>
concept reconstruction = std::same_as<R, NoReconstruction> ||
                         std::same_as<R, WENO3Reconstruction<Num>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
