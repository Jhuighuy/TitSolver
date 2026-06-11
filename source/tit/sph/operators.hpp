/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include "tit/geom/shape/face.hpp"
#include "tit/sph/field.hpp"
#include "tit/sph/particle_array.hpp"
#include "tit/sph/particle_mesh.hpp"

namespace tit::sph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Gradient operators.
//

/// First-order gradient operator.
template<field auto grad_f, field auto f>
struct Grad1 {
  constexpr void on_init(auto a) const {
    grad_f[a] = {};
  }
  template<class PV>
  constexpr void on_face(PV a, auto s, auto grad_gamma_as) const {
    if constexpr (is_vec_v<particle_field_t<grad_f, PV>>) {
      grad_f[a] -= f[s, a] * grad_gamma_as;
    } else {
      grad_f[a] -= outer(f[s, a], grad_gamma_as);
    }
  }
  template<class PV>
  constexpr void on_pair(PV a, PV b, auto grad_w_ab) const {
    if constexpr (is_vec_v<particle_field_t<grad_f, PV>>) {
      grad_f[a] += m[b] / rho[b] * f[b, a] * grad_w_ab;
      grad_f[b] -= m[a] / rho[a] * f[a, b] * grad_w_ab;
    } else {
      grad_f[a] += m[b] / rho[b] * outer(f[b, a], grad_w_ab);
      grad_f[b] -= m[a] / rho[a] * outer(f[a, b], grad_w_ab);
    }
  }
  constexpr void on_finalize(auto a) const {
    grad_f[a] /= gamma[a];
  }
};

/// Second-order gradient operator.
/// @note Correction matrix L must be precomputed.
template<field auto grad_f, field auto f>
struct Grad2 : Grad1<grad_f, f> {
  template<class PV>
  constexpr void on_finalize(PV a) const {
    Grad1<grad_f, f>::on_finalize(a);
    if constexpr (is_vec_v<particle_field_t<grad_f, PV>>) {
      grad_f[a] = L[a] * grad_f[a];
    } else {
      grad_f[a] = grad_f[a] * transpose(L[a]);
    }
  }
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Conservative operators.
//

/// Conservative gradient operator, divided by density: ∇f/ρ.
template<field auto grad_f, field auto f>
struct GradOverRho final {
  constexpr void on_init(auto a) const {
    grad_f[a] = {};
  }
  template<class PV>
  constexpr void on_face(PV a, auto s, auto grad_gamma_as) const {
    const auto F_as = f[a] / pow2(rho[a]) + f[s] / pow2(rho[s]);
    grad_f[a] -= rho[s] * F_as * grad_gamma_as;
  }
  template<class PV>
  constexpr void on_pair(PV a, PV b, auto grad_w_ab) const {
    const auto F_ab = f[a] / pow2(rho[a]) + f[b] / pow2(rho[b]);
    grad_f[a] += m[b] * F_ab * grad_w_ab;
    grad_f[b] -= m[a] * F_ab * grad_w_ab;
  }
  constexpr void on_finalize(auto a) const {
    grad_f[a] /= gamma[a];
  }
};

/// Conservative divergence operator, multiplied by density, ρ∇·u.
template<field auto div_u, field auto u>
struct RhoDiv final {
  constexpr void on_init(auto a) const {
    div_u[a] = {};
  }
  template<class PV>
  constexpr void on_face(PV a, auto s, auto grad_gamma_as) const {
    div_u[a] -= rho[s] * dot(u[s, a], grad_gamma_as);
  }
  template<class PV>
  constexpr void on_pair(PV a, PV b, auto grad_w_ab) const {
    div_u[a] += m[b] * dot(u[b, a], grad_w_ab);
    div_u[b] -= m[a] * dot(u[a, b], grad_w_ab);
  }
  constexpr void on_finalize(auto a) const {
    div_u[a] /= gamma[a];
  }
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Specialized operators.
//

/// Compute ∇γ.
struct ComputeGradGamma final {
  constexpr void on_init(auto a) const {
    grad_gamma[a] = {};
  }
  constexpr void on_face(auto a, auto /*s*/, auto grad_gamma_as) const {
    grad_gamma[a] += grad_gamma_as;
  }
  constexpr void on_pair(auto /*a*/, auto /*b*/, auto /*grad_w_ab*/) const {
    // No-op.
  }
  constexpr void on_finalize(auto /*a*/) const {
    // No-op.
  }
};

/// Compute L = (∇r)⁻¹.
struct ComputeL final : Grad1<L, r> {
  constexpr void on_finalize(auto a) const {
    Grad1<L, r>::on_finalize(a);
    if (const auto fact = lu(transpose(L[a]))) {
      L[a] = fact->inverse();
    } else {
      L[a] = eye(L[a]);
    }
  }
};

/// Compute dr = ∇1.
struct ComputeDr final {
  constexpr void on_init(auto a) const {
    dr[a] = {};
  }
  constexpr void on_face(auto a, auto /*s*/, auto grad_gamma_as) const {
    dr[a] -= grad_gamma_as;
  }
  constexpr void on_pair(auto a, auto b, auto grad_w_ab) const {
    dr[a] += m[b] / rho[b] * grad_w_ab;
    dr[b] -= m[a] / rho[a] * grad_w_ab;
  }
  constexpr void on_finalize(auto a) const {
    dr[a] /= gamma[a];
  }
};

/// Compute N = [(∇r)⁻¹∇1]/|[(∇r)⁻¹∇1]|.
struct ComputeN final {
  constexpr void on_init(auto a) const {
    N[a] = {};
  }
  constexpr void on_face(auto a, auto /*s*/, auto grad_gamma_as) const {
    N[a] -= grad_gamma_as;
  }
  constexpr void on_pair(auto a, auto b, auto grad_w_ab) const {
    N[a] += m[b] / rho[b] * grad_w_ab;
    N[b] -= m[a] / rho[a] * grad_w_ab;
  }
  constexpr void on_finalize(auto a) const {
    N[a] = normalize(L[a] * N[a]);
  }
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Operator fusion.
//

template<particle_mesh ParticleMesh, particle_array ParticleArray>
void fused_pass(const auto& kernel,
                const auto& domain,
                ParticleMesh& mesh,
                ParticleArray& particles,
                auto... ops) {
  par::for_each(particles.all(), [&domain, &mesh, &kernel, ops...](auto a) {
    (ops.on_init(a), ...);
    for (const auto& [s_index, s] : mesh[domain, a]) {
      const geom::Face face{domain.face(s_index)};
      const auto grad_gamma_as = kernel.flux(face, a);
      (ops.on_face(a, s, grad_gamma_as), ...);
    }
  });
  par::block_for_each(mesh.block_pairs(particles),
                      [&domain, &mesh, &kernel, ops...](auto ab) {
                        const auto [a, b] = ab;
                        const auto grad_w_ab = kernel.grad(a, b);
                        (ops.on_pair(a, b, grad_w_ab), ...);
                      });
  par::for_each(particles.all(),
                [ops...](auto a) { (ops.on_finalize(a), ...); });
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
