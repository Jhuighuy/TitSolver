/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include "tit/core/mat.hpp"
#include "tit/core/math.hpp"
#include "tit/core/vec.hpp"
#include "tit/par/algorithms.hpp"
#include "tit/sph/field.hpp"
#include "tit/sph/kernel.hpp"
#include "tit/sph/particle_array.hpp"
#include "tit/sph/particle_mesh.hpp"

namespace tit::sph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// @todo This is a temporary implementation.
template<kernel Kernel,
         particle_mesh ParticleMesh,
         particle_array ParticleArray>
void apply_bcs(const Kernel& kernel,
               ParticleMesh& mesh,
               ParticleArray& particles) {
  using PV = ParticleView<ParticleArray>;
  using Num = particle_num_t<ParticleArray>;
  static constexpr auto Dim = particle_dim_v<ParticleArray>;

  // Interpolate the field values on the boundary.
  par::for_each(particles.fixed(), [&kernel, &mesh](PV b) {
    /// @todo Once we have a proper geometry library, we should use
    ///       here and clean up the code.
    const auto& search_point = r[b];
    const auto clipped_point = Domain<Num>.clamp(search_point);
    const auto r_ghost = 2 * clipped_point - search_point;
    const auto SN = normalize(search_point - clipped_point);
    const auto SD = norm(r_ghost - r[b]);

    constexpr Num rho_0 = 1000.0;
    constexpr Num cs_0 = 20 * sqrt(9.81 * 0.6);
    constexpr Vec<Num, 2> G{0.0, -9.81};

    // Compute the interpolation weights, both for the constant and
    // linear interpolations.
    Num S{};
    Mat<Num, Dim + 1> M{};
    const auto h_ghost = RADIUS_SCALE * h[b];
    for (const PV a : mesh.fixed_interp(b)) {
      const auto r_delta = r_ghost - r[a];
      const auto B_delta = vec_cat(Vec{Num{1.0}}, r_delta);
      const auto W_delta = kernel(r_delta, h_ghost);
      S += W_delta * m[a] / rho[a];
      M += outer(B_delta, B_delta * W_delta * m[a] / rho[a]);
    }

    if (const auto fact = ldl(M); fact) {
      // Linear interpolation succeeds, use it.
      clear(b, rho, v, u);
      const auto E = fact->solve(unit<0>(M[0]));
      for (const PV a : mesh.fixed_interp(b)) {
        const auto r_delta = r_ghost - r[a];
        const auto B_delta = vec_cat(Vec{Num{1.0}}, r_delta);
        const auto W_delta = dot(E, B_delta) * kernel(r_delta, h_ghost);
        rho[b] += m[a] * W_delta;
        v[b] += m[a] / rho[a] * v[a] * W_delta;
        if constexpr (has<PV>(u)) u[b] += m[a] / rho[a] * u[a] * W_delta;
      }
    } else if (!is_tiny(S)) {
      // Constant interpolation succeeds, use it.
      clear(b, rho, v, u);
      const auto E = inverse(S);
      for (const PV a : mesh.fixed_interp(b)) {
        const auto r_delta = r_ghost - r[a];
        const auto W_delta = E * kernel(r_delta, h_ghost);
        rho[b] += m[a] * W_delta;
        v[b] += m[a] / rho[a] * v[a] * W_delta;
        if constexpr (has<PV>(u)) u[b] += m[a] / rho[a] * u[a] * W_delta;
      }
    } else {
      // Both interpolations fail, leave the particle as it is.
      rho[b] = rho_0;
      v[b] = {};
      if constexpr (has<PV>(u)) u[b] = {};
      return;
    }

    // Compute the density at the boundary.
    // drho/dn = rho_0/(cs_0^2)*dot(g,n).
    rho[b] += SD * rho_0 / pow2(cs_0) * dot(G, SN);

    // Compute the velocity at the boundary (slip wall boundary condition).
    const auto Vn = dot(v[b], SN) * SN;
    const auto Vt = v[b] - Vn;
    v[b] = Vt - Vn;
  });
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
