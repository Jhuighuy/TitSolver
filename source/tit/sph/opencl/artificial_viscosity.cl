/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include "math.cl"

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

float2 artificial_viscosity__density_term(const __global float2* r,
                                          const __global float* rho,
                                          const size_t a,
                                          const size_t b,
                                          const float h,
                                          const float m,
                                          const float cs_0,
                                          const float xi) {
  const float D_ab = D(rho, a, b);
  const float Xi_ab = xi * cs_0 * h;
  return 2 * Xi_ab * D_ab * (r[a] - r[b]) / (norm2(r[a] - r[b]) + 1.0e-10F);
}

float artificial_viscosity__velocity_term(const __global float2* r,
                                          const __global float2* v,
                                          const __global float* rho,
                                          const size_t a,
                                          const size_t b,
                                          const float h,
                                          const float rho_0,
                                          const float cs_0,
                                          const float alpha) {
  const float Alpha_ab = alpha * cs_0 * rho_0 * h;
  return Alpha_ab * dot(D(r, a, b), D(v, a, b)) /
         (rho[a] * rho[b] * (norm2(D(r, a, b)) + 1.0e-10F));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
