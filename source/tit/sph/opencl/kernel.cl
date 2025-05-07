/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include "math.cl"

#define TIT_KERNEL_CUBIC_SPLINE 101
#define TIT_KERNEL_QUARTIC_SPLINE 102
#define TIT_KERNEL_QUINTIC_SPLINE 103
#define TIT_KERNEL_QUARTIC_WENDLAND 211
#define TIT_KERNEL_SIXTH_ORDER_WENDLAND 212
#define TIT_KERNEL_EIGHTH_ORDER_WENDLAND 213

#define TIT_KERNEL TIT_KERNEL_QUARTIC_WENDLAND

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Cubic B-spline (M4) smoothing kernel.
#if TIT_KERNEL == TIT_KERNEL_CUBIC_SPLINE

Num W_unit_radius() {
  return NUM(2.0);
}

Num W_weight() {
  return (Num[]) {
      2.0 / 3.0,
      10.0 / 7.0 * M_1_PI_F,
      M_1_PI_F,
  }[Dim - 1];
}

Num W_unit(Num q) {
  const Vec2 qi = {2.0, 1.0};
  const Vec2 wi = {0.25, -1.0};
  const Vec2 qv = (Vec2) q;
  return sum(filter(wi * pow3(qi - qv), isless(qv, qi)));
}

Vec W_unit_deriv(Num q) {
  const Vec2 qi = {2.0, 1.0};
  const Vec2 wi = {-0.75, 3.0};
  const Vec2 qv = (Vec2) q;
  return sum(filter(wi * pow2(qi - qv), isless(qv, qi)));
}

#endif // TIT_KERNEL == TIT_KERNEL_CUBIC_SPLINE

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// The quartic B-spline (M5) smoothing kernel.
#if TIT_KERNEL == TIT_KERNEL_QUARTIC_SPLINE

Num W_unit_radius() {
  return NUM(2.5);
}

Num W_weight() {
  return (Num[]) {
      1.0 / 24.0,
      96.0 / 1199.0 * M_1_PI_F,
      1.0 / 20.0 * M_1_PI_F,
  }[Dim - 1];
}

Num W_unit(Num q) {
  const Vec3 qi = {2.5, 1.5, 0.5};
  const Vec3 wi = {1.0, -5.0, 10.0};
  const Vec3 qv = (Vec3) q;
  return sum(filter(wi * pow4(qi - qv), isless(qv, qi)));
}

Vec W_unit_deriv(Num q) {
  const Vec3 qi = {2.5, 1.5, 0.5};
  const Vec3 wi = {-4.0, 20.0, -40.0};
  const Vec3 qv = (Vec3) q;
  return sum(filter(wi * pow3(qi - qv), isless(qv, qi)));
}

#endif // TIT_KERNEL == TIT_KERNEL_QUARTIC_SPLINE

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Quintic B-spline (M6) smoothing kernel.
#if TIT_KERNEL == TIT_KERNEL_QUINTIC_SPLINE

Num W_unit_radius() {
  return NUM(3.0);
}

Num W_weight() {
  return (Num[]) {
      1.0 / 120.0,
      7.0 / 478.0 * M_1_PI_F,
      1.0 / 120.0 * M_1_PI_F,
  }[Dim - 1];
}

Num W_unit(Num q) {
  const Vec3 qi = {3.0, 2.0, 1.0};
  const Vec3 wi = {1.0, -6.0, 15.0};
  const Vec3 qv = (Vec3) q;
  return sum(filter(wi * pow5(qi - qv), isless(qv, qi)));
}

Vec W_unit_deriv(Num q) {
  const Vec3 qi = {3.0, 2.0, 1.0};
  const Vec3 wi = {-5.0, 30.0, -75.0};
  const Vec3 qv = (Vec3) q;
  return sum(filter(wi * pow4(qi - qv), isless(qv, qi)));
}

#endif // TIT_KERNEL == TIT_KERNEL_QUINTIC_SPLINE

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Wendland's quartic (C2) smoothing kernel (Wendland, 1995).
#if TIT_KERNEL == TIT_KERNEL_QUARTIC_WENDLAND

Num W_unit_radius() {
  return NUM(2.0);
}

Num W_weight() {
  return (Num[]) {
      3.0 / 4.0,
      7.0 / 4.0 * M_1_PI_F,
      21.0 / 16.0 * M_1_PI_F,
  }[Dim - 1];
}

Num W_unit(Num q) {
  if (q > W_unit_radius()) return NUM(0.0);
  return (NUM(1.0) + NUM(2.0) * q) * pow4(NUM(1.0) - NUM(0.5) * q);
}

Num W_unit_deriv(Num q) {
  if (q > W_unit_radius()) return NUM(0.0);
  return NUM(5.0 / 8.0) * q * pow3(q - NUM(2.0));
}

#endif // if TIT_KERNEL == TIT_KERNEL_QUARTIC_WENDLAND

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Wendland's 6-th order (C4) smoothing kernel (Wendland, 1995).
#if TIT_KERNEL == TIT_KERNEL_SIXTH_ORDER_WENDLAND

Num W_unit_radius() {
  return NUM(2.0);
}

Num W_weight() {
  return (Num[]) {
      27.0 / 32.0,
      9.0 / 4.0 * M_1_PI_F,
      495.0 / 256.0 * M_1_PI_F,
  }[Dim - 1];
}

Num W_unit(Num q) {
  if (q > W_unit_radius()) return NUM(0.0);
  return horner(q, 1.0, 3.0, 35.0 / 12.0) * pow6(NUM(1.0) - NUM(0.5) * q);
}

Num W_unit_deriv(Num q) {
  if (q > W_unit_radius()) return NUM(0.0);
  return NUM(7.0 / 96.0) * q * horner(q, 2.0, 5.0) * pow5(q - NUM(2.0));
}

#endif // if TIT_KERNEL == TIT_KERNEL_SIXTH_ORDER_WENDLAND

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Wendland's 8-th order (C6) smoothing kernel (Wendland, 1995).
#if TIT_KERNEL == TIT_KERNEL_EIGHTH_ORDER_WENDLAND

Num W_unit_radius() {
  return NUM(2.0);
}

Num W_weight() {
  return (Num[]) {
      15.0 / 16.0,
      39.0 / 14.0 * M_1_PI_F,
      1365.0 / 512.0 * M_1_PI_F,
  }[Dim - 1];
}

Num W_unit(Num q) {
  if (q > W_unit_radius()) return NUM(0.0);
  return horner(q, 1.0, 4.0, 25.0 / 4.0, 4.0) * pow8(NUM(1.0) - NUM(0.5) * q);
}

Vec W_unit_deriv(Num q) {
  if (q > W_unit_radius()) return NUM(0.0);
  return NUM(11.0 / 512.0) * q * horner(q, 2.0, 7.0, 8.0) * pow7(q - NUM(2.0));
}

#endif // if TIT_KERNEL == TIT_KERNEL_EIGHTH_ORDER_WENDLAND

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Support radius.
Num radius(Num h) {
  return h * W_unit_radius();
}

// Value of the smoothing kernel at point.
Num W(Vec x, Num h) {
  const Num h_inverse = inverse(h);
  const Num w = W_weight() * pown(h_inverse, Dim);
  const Num q = h_inverse * norm(x);
  return w * W_unit(q);
}

// Spatial gradient of the smoothing kernel at point.
Vec grad_W(Vec x, Num h) {
  const Num h_inverse = inverse(h);
  const Num w = W_weight() * pown(h_inverse, Dim);
  const Num q = h_inverse * norm(x);
  const Vec grad_q = normalize(x) * h_inverse;
  return w * W_unit_deriv(q) * grad_q;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
