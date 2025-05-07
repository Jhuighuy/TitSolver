/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define Num float
#define Dim 2

#include "artificial_viscosity.cl"
#include "common.cl"
#include "kernel.cl"
#include "math.cl"

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

__kernel void setup_boundary(const __global float2* r,
                             __global float2* v,
                             __global float* rho,
                             const float h,
                             const float m,
                             const __global unsigned long* iadj_ranges,
                             const __global unsigned long* iadj,
                             const unsigned long fixed_start,
                             const unsigned long fixed_end) {
  const size_t num_fixed = fixed_end - fixed_start;
  const size_t index = get_global_id(0);
  if (index >= num_fixed) return;
  const size_t b = fixed_start + index;

  const Vec search_point = r[b];
  const Vec clipped_point = domain_clamp(search_point);
  const Vec r_ghost = 2 * clipped_point - search_point;
  const Vec SN = normalize(search_point - clipped_point);
  const Num SD = norm(r_ghost - r[b]);

  Num S = 0;
  Mat3 M = {};
  const float h_ghost = RADIUS_SCALE * h;
  for (size_t k = iadj_ranges[index]; k < iadj_ranges[index + 1]; ++k) {
    const size_t a = iadj[k];

    const Num V_a = m / rho[a];
    const Vec r_delta = r_ghost - r[a];
    const Num W_delta = W(r_delta, h_ghost);
    const VecP B_delta = (VecP) {1, r_delta};

    S += W_delta * V_a;
    M = mat_add(M, vecP_outer(B_delta, B_delta * W_delta * V_a));
  }

  Mat3 F;
  if (mat_ldl(M, F)) {
    // Linear interpolation succeeds, use it.
    rho[b] = 0;
    v[b] = 0;
    const VecP E = mat_ldl_solve(F, ((VecP) {1, (Vec) 0}));
    for (size_t k = iadj_ranges[index]; k < iadj_ranges[index + 1]; ++k) {
      const size_t a = iadj[k];

      const Num V_a = m / rho[a];
      const Vec r_delta = r_ghost - r[a];
      const VecP B_delta = (VecP) {1, r_delta};
      const Num W_delta = dot(E, B_delta) * W(r_delta, h_ghost);

      rho[b] += m * W_delta;
      v[b] += V_a * v[a] * W_delta;
    }
  } else if (!is_tiny(S)) {
    // Constant interpolation succeeds, use it.
    rho[b] = 0;
    v[b] = 0;
    const Num E = inverse(S);
    for (size_t k = iadj_ranges[index]; k < iadj_ranges[index + 1]; ++k) {
      const size_t a = iadj[k];

      const Num V_a = m / rho[a];
      const Vec r_delta = r_ghost - r[a];
      const Num W_delta = E * W(r_delta, h_ghost);

      rho[b] += m * W_delta;
      v[b] += V_a * v[a] * W_delta;
    }
  } else {
    // Interpolation fails, leave the particle as it is.
    return;
  }

  // Compute the density at the boundary.
  // drho/dn = rho_0/(cs_0^2)*dot(g,n).
  const Num rho_0 = 1000.0;
  const Num cs_0 = 20.0 * sqrt(9.81 * 0.6);
  const Vec G = {0.0, -9.81};
  rho[b] += SD * rho_0 / pown(cs_0, 2) * dot(G, SN);

  // Compute the velocity at the boundary (slip wall boundary condition).
  const Vec Vn = dot(v[b], SN) * SN;
  const Vec Vt = v[b] - Vn;
  v[b] = Vt - Vn;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

__kernel void compute_density_density_gradient(
    const __global float2* r,
    const __global float* rho,
    __global float2* grad_rho,
    const float h,
    const float m,
    const __global unsigned long* adj_ranges,
    const __global unsigned long* adj,
    const unsigned long fluid_end) {
  const size_t num_fluid = fluid_end - fluid_start;
  const size_t index = get_global_id(0);
  if (index >= num_fluid) return;
  const size_t a = fluid_start + index;

  grad_rho[a] = 0.0F;
  for (size_t k = adj_ranges[index]; k < adj_ranges[index + 1]; ++k) {
    const size_t b = adj[k];
    if (a == b) continue;

    const float2 grad_W_ab = grad_W(r[b] - r[a], h);
    const float V_b = m / rho[b];
    grad_rho[a] += V_b * grad_W_ab;
  }
}

__kernel void compute_density_density_time_derivative(
    const __global float2* r,
    const __global float2* v,
    const __global float* rho,
    const __global float2* grad_rho,
    __global float* drho_dt,
    __global float* p,
    __global float2* dv_dt,
    const float h,
    const float m,
    const float cs_0,
    const float rho_0,
    const __global unsigned long* adj_ranges,
    const __global unsigned long* adj,
    const unsigned long fluid_end,
    const unsigned long fixed_end) {
  const size_t index = get_global_id(0);
  if (index >= fixed_end) return;
  const size_t a = fluid_start + index;

  if (index < fluid_end) {
    drho_dt[a] = 0.0F;
    for (size_t k = adj_ranges[index]; k < adj_ranges[index + 1]; ++k) {
      const size_t b = adj[k];
      if (a == b) continue;

      const float2 grad_W_ab = grad_W(r[a] - r[b], h);
      const float2 Psi_ab = artificial_viscosity__density_term(r,
                                                               rho,
                                                               a,
                                                               b,
                                                               h,
                                                               m,
                                                               cs_0,
                                                               /*xi=*/0.1);
      drho_dt[a] -= m * dot(v[b] - v[a] - Psi_ab / rho[b], grad_W_ab);
    }
  }

  // p[a] = pown(cs_0, 2) * (rho[a] - rho_0);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

__kernel void compute_forces(const __global float2* r,
                             const __global float* rho,
                             __global float2* grad_rho,
                             __global float* p,
                             const __global float2* v,
                             __global float2* dv_dt,
                             const float h,
                             const float m,
                             const float cs_0,
                             const float rho_0,
                             const __global unsigned long* adj_ranges,
                             const __global unsigned long* adj,
                             const unsigned long fluid_end) {
  const size_t num_fluid = fluid_end - fluid_start;
  const size_t index = get_global_id(0);
  if (index >= num_fluid) return;
  const size_t a = fluid_start + index;

  dv_dt[a].x = 0.0F;
  dv_dt[a].y = -9.81F;
  for (size_t k = adj_ranges[index]; k < adj_ranges[index + 1]; ++k) {
    const size_t b = adj[k];
    if (a == b) continue;

    const float2 grad_W_ab = grad_W(r[a] - r[b], h);

    const float p_a = pown(cs_0, 2) * (rho[a] - rho_0);
    const float p_b = pown(cs_0, 2) * (rho[b] - rho_0);

    const float P_a = p_a / pown(rho[a], 2);
    const float P_b = p_b / pown(rho[b], 2);
    const float Pi_ab = artificial_viscosity__velocity_term(r,
                                                            v,
                                                            rho,
                                                            a,
                                                            b,
                                                            h,
                                                            rho_0,
                                                            cs_0,
                                                            /*alpha=*/0.02);

    const float2 v_flux = (-P_a - P_b + Pi_ab) * grad_W_ab;
    dv_dt[a] += m * v_flux;
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

__kernel void update_density(__global float* rho,
                             const __global float* drho_dt,
                             const float dt,
                             const unsigned long fluid_end) {
  const size_t num_fluid = fluid_end - fluid_start;
  const size_t index = get_global_id(0);
  if (index >= num_fluid) return;
  const size_t a = fluid_start + index;

  rho[a] += dt * drho_dt[a];
}

__kernel void update_velocity(__global float2* r,
                              __global float2* v,
                              const __global float2* dv_dt,
                              const float dt,
                              const unsigned long fluid_end) {
  const size_t num_fluid = fluid_end - fluid_start;
  const size_t index = get_global_id(0);
  if (index >= num_fluid) return;
  const size_t a = fluid_start + index;

  v[a] += dt * dv_dt[a];
  r[a] += dt * v[a];
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
