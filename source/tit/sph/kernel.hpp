/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <limits>
#include <numbers>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/math.hpp"
#include "tit/core/meta.hpp"
#include "tit/core/vec.hpp"

#include "tit/sph/field.hpp"

#ifndef TIT_BRANCHLESS_KERNELS
#define TIT_BRANCHLESS_KERNELS 1
#endif

namespace tit::sph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Abstract smoothing kernel.
template<class Derived>
class Kernel {
protected:

  /// Derived class reference.
  constexpr auto derived() const noexcept -> Derived const& {
    static_assert(std::derived_from<Derived, Kernel<Derived>>);
    return static_cast<Derived const&>(*this);
  }

public:

  /// Set of particle fields that are required.
  static constexpr auto required_fields = meta::Set{r, h};

  /// Support radius for particle.
  template<class PV>
    requires (has<PV>(required_fields))
  constexpr auto radius(PV a) const noexcept {
    return radius(h[a]);
  }

  /// Value of the smoothing kernel for two particles.
  /// @{
  template<class PV>
    requires (has<PV>(required_fields) && has_const<PV>(h))
  constexpr auto operator()(PV a, PV b) const noexcept {
    return (*this)(r[a, b], h[a]);
  }
  template<class PV, class Real>
    requires (has<PV>(required_fields) && !has_const<PV>(h))
  constexpr auto operator()(PV a, PV b, Real h) const noexcept {
    return (*this)(r[a, b], h);
  }
  /// @}

  /// Directional derivative of the smoothing kernel for two particles.
  /// @{
  template<class PV>
    requires (has<PV>(required_fields) && has_const<PV>(h))
  constexpr auto deriv(PV a, PV b) const noexcept {
    return deriv(r[a, b], h[a]);
  }
  template<class PV, class Real>
    requires (has<PV>(required_fields) && !has_const<PV>(h))
  constexpr auto deriv(PV a, PV b, Real h) const noexcept {
    return deriv(r[a, b], h);
  }
  /// @}

  /// Value of the smoothing kernel for two particles.
  /// @{
  template<class PV>
    requires (has<PV>(required_fields) && has_const<PV>(h))
  constexpr auto grad(PV a, PV b) const noexcept {
    return grad(r[a, b], h[a]);
  }
  template<class PV, class Real>
    requires (has<PV>(required_fields) && !has_const<PV>(h))
  constexpr auto grad(PV a, PV b, Real h) const noexcept {
    return grad(r[a, b], h);
  }
  /// @}

  /// Width derivative of the smoothing kernel for two points.
  /// @{
  template<class PV>
  // requires (has<PV>(required_fields) && has_const<PV>(h))
  constexpr auto width_deriv(PV a, PV b) const noexcept {
    return width_deriv(r[a, b], h[a]);
  }
  template<class PV, class Real>
    requires (has<PV>(required_fields) && !has_const<PV>(h))
  constexpr auto width_deriv(PV a, PV b, Real h) const noexcept {
    return width_deriv(r[a, b], h);
  }
  /// @}

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Kernel weight.
  template<class Real, size_t Dim>
  constexpr auto weight() const noexcept -> Real {
    // A shortcut in order not to write `.template weight<...>` all the time.
    return derived().template weight<Real, Dim>();
  }

  /// Support radius.
  template<class Real>
  constexpr auto radius(Real h) const noexcept -> Real {
    TIT_ASSERT(h > Real{0.0}, "Kernel width must be positive!");
    auto const radius = derived().template unit_radius<Real>();
    return radius * h;
  }

  /// Value of the smoothing kernel at point.
  template<class Real, size_t Dim>
  constexpr auto operator()(Vec<Real, Dim> x, Real h) const noexcept -> Real {
    TIT_ASSERT(h > Real{0.0}, "Kernel width must be positive!");
    auto const h_inverse = inverse(h);
    auto const w = weight<Real, Dim>() * pow(h_inverse, Dim);
    auto const q = h_inverse * norm(x);
    return w * derived().unit_value(q);
  }

  /// Directional derivative of the smoothing kernel at point.
  template<class Real, size_t Dim>
  constexpr auto deriv(Vec<Real, Dim> x, Real h) const noexcept -> Real {
    TIT_ASSERT(h > Real{0.0}, "Kernel width must be positive!");
    auto const h_inverse = inverse(h);
    auto const w = weight<Real, Dim>() * pow(h_inverse, Dim);
    auto const q = h_inverse * norm(x);
    return w * derived().unit_deriv(q) * h_inverse;
  }

  /// Spatial gradient of the smoothing kernel at point.
  template<class Real, size_t Dim>
  constexpr auto grad(Vec<Real, Dim> x,
                      Real h) const noexcept -> Vec<Real, Dim> {
    TIT_ASSERT(h > Real{0.0}, "Kernel width must be positive!");
    auto const h_inverse = inverse(h);
    auto const w = weight<Real, Dim>() * pow(h_inverse, Dim);
    auto const q = h_inverse * norm(x);
    auto const grad_q = normalize(x) * h_inverse;
    return w * derived().unit_deriv(q) * grad_q;
  }

  /// Width derivative of the smoothing kernel at point.
  template<class Real, size_t Dim>
  constexpr auto width_deriv(Vec<Real, Dim> x, Real h) const noexcept -> Real {
    TIT_ASSERT(h > Real{0.0}, "Kernel width must be positive!");
    auto const h_inverse = inverse(h);
    auto const w = weight<Real, Dim>() * pow(h_inverse, Dim);
    auto const dw_dh = -int{Dim} * w * h_inverse;
    auto const q = h_inverse * norm(x);
    auto const dq_dh = -q * h_inverse;
    return dw_dh * derived().unit_value(q) +
           w * derived().unit_deriv(q) * dq_dh;
  }

}; // class Kernel

/// Smoothing kernel type.
template<class DerivedKernel>
concept kernel = std::movable<DerivedKernel> &&
                 std::derived_from<DerivedKernel, Kernel<DerivedKernel>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Bell-shaped smoothing kernel.
// TODO: Implement me! What am I?
class BellShapedKernel {};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Gaussian smoothing kernel (Monaghan, 1992).
class GaussianKernel final : public Kernel<GaussianKernel> {
public:

  /// Kernel weight.
  template<class Real, size_t Dim>
  static constexpr auto weight() noexcept -> Real {
    static_assert(1 <= Dim);
    return pow(std::numbers::inv_sqrtpi_v<Real>, Dim);
  }

  /// Unit support radius.
  template<class Real>
  static constexpr auto unit_radius() noexcept -> Real {
    // We truncate gaussian at the point, where it reaches minimal non-zero
    // value for the current floating-point type. It approximately is 9.3454
    // for float and 26.6157 for double. Gaussian value at this point is
    // approximately 10^-38 for float and 10^-308 for double.
    return sqrt(-log(std::numeric_limits<Real>::min()));
  }

  /// Value of the unit smoothing kernel at a point.
  template<class Real>
  static constexpr auto unit_value(Real q) noexcept -> Real {
    return exp(-pow2(q));
  }

  /// Derivative of the unit smoothing kernel at a point.
  template<class Real>
  static constexpr auto unit_deriv(Real q) noexcept -> Real {
    return -Real{2.0} * q * exp(-pow2(q));
  }

}; // class GaussianKernel

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Super-gaussian smoothing kernel (Monaghan, 1992).
// TODO: implement me!
class SuperGaussianKernel {};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Cubic B-spline (M4) smoothing kernel.
class CubicSplineKernel final : public Kernel<CubicSplineKernel> {
public:

  /// Kernel weight.
  template<class Real, size_t Dim>
  static constexpr auto weight() noexcept -> Real {
    static_assert(1 <= Dim && Dim <= 3);
    switch (Dim) {
      case 1: return Real{2.0 / 3.0};
      case 2: return Real{10.0 / 7.0 * std::numbers::inv_pi};
      case 3: return std::numbers::inv_pi_v<Real>;
    }
  }

  /// Unit support radius.
  template<class Real>
  static constexpr auto unit_radius() noexcept -> Real {
    return Real{2.0};
  }

  /// Value of the unit smoothing kernel at a point.
  template<class Real>
  static constexpr auto unit_value(Real q) noexcept -> Real {
    constexpr auto qi = Vec{Real{2.0}, Real{1.0}};
    constexpr auto wi = Vec{Real{0.25}, Real{-1.0}};
#if TIT_BRANCHLESS_KERNELS
    auto const qv = Vec<Real, 2>(q);
    return sum(merge(qv < qi, wi * pow3(qi - qv)));
#else
    auto W = Real{0.0};
    if (q < qi[0]) {
      W += wi[0] * pow3(qi[0] - q);
      if (q < qi[1]) {
        W += wi[1] * pow3(qi[1] - q);
      }
    }
    return W;
#endif
  }

  /// Derivative of the unit smoothing kernel at a point.
  template<class Real>
  static constexpr auto unit_deriv(Real q) noexcept -> Real {
    constexpr auto qi = Vec{Real{2.0}, Real{1.0}};
    constexpr auto wi = Vec{Real{0.25}, Real{-1.0}};
#if TIT_BRANCHLESS_KERNELS
    auto const qv = Vec<Real, 2>(q);
    return sum(merge(qv < qi, wi * Real{-3.0} * pow2(qi - qv)));
#else
    auto dW_dq = Real{0.0};
    if (q < qi[0]) {
      dW_dq += wi[0] * Real{-3.0} * pow2(qi[0] - q);
      if (q < qi[1]) {
        dW_dq += wi[1] * Real{-3.0} * pow2(qi[1] - q);
      }
    }
    return dW_dq;
#endif
  }

}; // class CubicSplineKernel

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Cubic B-spline (M4) smoothing kernel
/// with modified derivative (Thomas, Couchman, 1992).
class ThomasCouchmanKernel final : public Kernel<ThomasCouchmanKernel> {
public:

  /// Kernel weight.
  template<class Real, size_t Dim>
  static consteval auto weight() noexcept -> Real {
    return CubicSplineKernel::weight<Real, Dim>();
  }

  /// Unit support radius.
  template<class Real>
  static consteval auto unit_radius() noexcept -> Real {
    return CubicSplineKernel::unit_radius<Real>();
  }

  /// Value of the unit smoothing kernel at a point.
  template<class Real>
  static constexpr auto unit_value(Real q) noexcept -> Real {
    return CubicSplineKernel::unit_value(q);
  }

  /// Derivative of the unit smoothing kernel at a point.
  template<class Real>
  static constexpr auto unit_deriv(Real q) noexcept -> Real {
    // TODO: provide a branchless implementation.
    if (q < Real{2.0 / 3.0}) return Real{-1.0};
    if (q < Real{1.0}) return (Real{2.25} * q - Real{3.0}) * q;
    if (q < Real{2.0}) return Real{0.75} * pow2(Real{2.0} - q);
    return Real{0.0};
  }

}; // class ThomasCouchmanKernel

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// The quartic B-spline (M5) smoothing kernel.
class QuarticSplineKernel final : public Kernel<QuarticSplineKernel> {
public:

  /// Kernel weight.
  template<class Real, size_t Dim>
  static constexpr auto weight() noexcept -> Real {
    static_assert(1 <= Dim && Dim <= 3);
    switch (Dim) {
      case 1: return Real{1.0 / 24.0};
      case 2: return Real{96.0 / 1199.0 * std::numbers::inv_pi};
      case 3: return Real{1.0 / 20.0 * std::numbers::inv_pi};
    }
  }

  /// Unit support radius.
  template<class Real>
  static constexpr auto unit_radius() noexcept -> Real {
    return Real{2.5};
  }

  /// Value of the unit smoothing kernel at a point.
  template<class Real>
  static constexpr auto unit_value(Real q) noexcept -> Real {
    constexpr auto qi = Vec{Real{2.5}, Real{1.5}, Real{0.5}};
    constexpr auto wi = Vec{Real{1.0}, Real{-5.0}, Real{10.0}};
#if TIT_BRANCHLESS_KERNELS
    auto const qv = Vec<Real, 3>(q);
    return sum(merge(qv < qi, wi * pow4(qi - qv)));
#else
    auto W = Real{0.0};
    if (q < qi[0]) {
      W += wi[0] * pow4(qi[0] - q);
      if (q < qi[1]) {
        W += wi[1] * pow4(qi[1] - q);
        if (q < qi[2]) {
          W += wi[2] * pow4(qi[2] - q);
        }
      }
    }
    return W;
#endif
  }

  /// Derivative value of the unit smoothing kernel at a point.
  template<class Real>
  static constexpr auto unit_deriv(Real q) noexcept -> Real {
    constexpr auto qi = Vec{Real{2.5}, Real{1.5}, Real{0.5}};
    constexpr auto wi = Vec{Real{1.0}, Real{-5.0}, Real{10.0}};
#if TIT_BRANCHLESS_KERNELS
    auto const qv = Vec<Real, 3>(q);
    return sum(merge(qv < qi, wi * Real{-4.0} * pow3(qi - qv)));
#else
    auto dW_dq = Real{0.0};
    if (q < qi[0]) {
      dW_dq += wi[0] * Real{-4.0} * pow3(qi[0] - q);
      if (q < qi[1]) {
        dW_dq += wi[1] * Real{-4.0} * pow3(qi[1] - q);
        if (q < qi[2]) {
          dW_dq += wi[2] * Real{-4.0} * pow3(qi[2] - q);
        }
      }
    }
    return dW_dq;
#endif
  }

}; // class QuarticSplineKernel

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Quintic B-spline (M6) smoothing kernel.
class QuinticSplineKernel final : public Kernel<QuinticSplineKernel> {
public:

  /// Kernel weight.
  template<class Real, size_t Dim>
  static constexpr auto weight() noexcept {
    static_assert(1 <= Dim && Dim <= 3);
    switch (Dim) {
      case 1: return Real{1.0 / 120.0};
      case 2: return Real{7.0 / 478.0 * std::numbers::inv_pi};
      case 3: return Real{1.0 / 120.0 * std::numbers::inv_pi}; // or 3/359?
    }
  }

  /// Unit support radius.
  template<class Real>
  static constexpr auto unit_radius() noexcept -> Real {
    return Real{3.0};
  }

  /// Value of the unit smoothing kernel at a point.
  template<class Real>
  static constexpr auto unit_value(Real q) noexcept -> Real {
    constexpr auto qi = Vec{Real{3.0}, Real{2.0}, Real{1.0}};
    constexpr auto wi = Vec{Real{1.0}, Real{-6.0}, Real{15.0}};
#if TIT_BRANCHLESS_KERNELS
    auto const qv = Vec<Real, 3>(q);
    return sum(merge(qv < qi, wi * pow5(qi - qv)));
#else
    auto W = Real{0.0};
    if (q < qi[0]) {
      W += wi[0] * pow5(qi[0] - q);
      if (q < qi[1]) {
        W += wi[1] * pow5(qi[1] - q);
        if (q < qi[2]) {
          W += wi[2] * pow5(qi[2] - q);
        }
      }
    }
    return W;
#endif
  }

  /// Derivative of the unit smoothing kernel at a point.
  template<class Real>
  static constexpr auto unit_deriv(Real q) noexcept -> Real {
    constexpr auto qi = Vec{Real{3.0}, Real{2.0}, Real{1.0}};
    constexpr auto wi = Vec{Real{1.0}, Real{-6.0}, Real{15.0}};
#if TIT_BRANCHLESS_KERNELS
    auto const qv = Vec<Real, 3>(q);
    return sum(merge(qv < qi, wi * Real{-5.0} * pow4(qi - qv)));
#else
    auto dW_dq = Real{0.0};
    if (q < qi[0]) {
      dW_dq += wi[0] * Real{-5.0} * pow4(qi[0] - q);
      if (q < qi[1]) {
        dW_dq += wi[1] * Real{-5.0} * pow4(qi[1] - q);
        if (q < qi[2]) {
          dW_dq += wi[2] * Real{-5.0} * pow4(qi[2] - q);
        }
      }
    }
    return dW_dq;
#endif
  }

}; // class QuinticSplineKernel

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Abstract Wendland's smoothing kernel.
template<class Derived>
class WendlandKernel : public Kernel<Derived> {
protected:

  using Kernel<Derived>::derived;

public:

  /// Unit support radius.
  template<class Real>
  static constexpr auto unit_radius() noexcept -> Real {
    // Wendland's kernels always have a support radius of 2.
    return Real{2.0};
  }

  /// Value of the unit smoothing kernel at a point.
  template<class Real>
  constexpr auto unit_value(Real q) const noexcept -> Real {
#if TIT_BRANCHLESS_KERNELS
    return merge(q < Real{2.0}, derived().unit_value_notrunc(q));
#else
    return q < Real{2.0} ? derived().unit_value_notrunc(q) : Real{0.0};
#endif
  }

  /// Derivative of the unit smoothing kernel at a point.
  template<class Real>
  constexpr auto unit_deriv(Real q) const noexcept -> Real {
#if TIT_BRANCHLESS_KERNELS
    return merge(q < Real{2.0}, derived().unit_deriv_notrunc(q));
#else
    return q < Real{2.0} ? derived().unit_deriv_notrunc(q) : Real{0.0};
#endif
  }

}; // class WendlandKernel

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Wendland's quartic (C2) smoothing kernel (Wendland, 1995).
class QuarticWendlandKernel final :
    public WendlandKernel<QuarticWendlandKernel> {
public:

  /// Kernel weight.
  template<class Real, size_t Dim>
  static constexpr auto weight() noexcept -> Real {
    static_assert(1 <= Dim && Dim <= 3);
    switch (Dim) {
      case 1: return Real{3.0 / 4.0};
      case 2: return Real{7.0 / 4.0 * std::numbers::inv_pi};
      case 3: return Real{21.0 / 16.0 * std::numbers::inv_pi};
    }
  }

  /// Value of the unit smoothing kernel at a point (not truncated).
  template<class Real>
  static constexpr auto unit_value_notrunc(Real q) noexcept -> Real {
    return (Real{1.0} + Real{2.0} * q) * pow4(Real{1.0} - Real{0.5} * q);
  }

  /// Derivative of the unit smoothing kernel at a point (not truncated).
  template<class Real>
  static constexpr auto unit_deriv_notrunc(Real q) noexcept -> Real {
    // Well known formula is dW/dq = -5 * q * (1 - q/2)^3, but it requires 5
    // multiplications. Formula that is used requires 4.
    return Real{5.0 / 8.0} * q * pow3(q - Real{2.0});
  }

}; // class QuarticWendlandKernel

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Wendland's 6-th order (C4) smoothing kernel (Wendland, 1995).
class SixthOrderWendlandKernel final :
    public WendlandKernel<SixthOrderWendlandKernel> {
public:

  /// Kernel weight.
  template<class Real, size_t Dim>
  static constexpr auto weight() noexcept -> Real {
    static_assert(1 <= Dim && Dim <= 3);
    switch (Dim) {
      case 1: return Real{27.0 / 16.0};
      case 2: return Real{9.0 / 4.0 * std::numbers::inv_pi};
      case 3: return Real{495.0 / 256.0 * std::numbers::inv_pi};
    }
  }

  /// Value of the unit smoothing kernel at a point (not truncated).
  template<class Real>
  static constexpr auto unit_value_notrunc(Real q) noexcept -> Real {
    return poly(q, Vec{Real{1.0}, Real{3.0}, Real{35.0 / 12.0}}) *
           pow6(Real{1.0} - Real{0.5} * q);
  }

  /// Derivative of the unit smoothing kernel at a point (not truncated).
  template<class Real>
  static constexpr auto unit_deriv_notrunc(Real q) noexcept -> Real {
    return poly(q, Real{7.0 / 96.0} * Vec{Real{2.0}, Real{5.0}}) * q *
           pow5(q - Real{2.0});
  }

}; // class SixthOrderWendlandKernel

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Wendland's 8-th order (C6) smoothing kernel (Wendland, 1995).
class EighthOrderWendlandKernel final :
    public WendlandKernel<EighthOrderWendlandKernel> {
public:

  /// Kernel weight.
  template<class Real, size_t Dim>
  static constexpr auto weight() noexcept -> Real {
    static_assert(1 <= Dim && Dim <= 3);
    switch (Dim) {
      case 1: return Real{15.0 / 8.0};
      case 2: return Real{39.0 / 14.0 * std::numbers::inv_pi};
      case 3: return Real{339.0 / 128.0 * std::numbers::inv_pi};
    }
  }

  /// Value of the unit smoothing kernel at a point (not truncated).
  template<class Real>
  static constexpr auto unit_value_notrunc(Real q) noexcept -> Real {
    return poly(q, Vec{Real{1.0}, Real{4.0}, Real{25.0 / 4.0}, Real{4.0}}) *
           pow8(Real{1.0} - Real{0.5} * q);
  }

  /// Derivative of the unit smoothing kernel at a point (not truncated).
  template<class Real>
  static constexpr auto unit_deriv_notrunc(Real q) noexcept -> Real {
    return poly(q, Real{11.0 / 512.0} * Vec{Real{2.0}, Real{7.0}, Real{8.0}}) *
           q * pow7(q - Real{2.0});
  }

}; // class SixthOrderWendlandKernel

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
