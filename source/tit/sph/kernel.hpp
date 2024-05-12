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
class BaseKernel {
public:

  /// Set of particle fields that are required.
  static constexpr auto required_fields = meta::Set{r, h};

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Support radius for particle.
  template<class PV>
    requires (has<PV>(required_fields))
  constexpr auto radius(this auto& self, PV a) noexcept {
    return self.radius(h[a]);
  }

  /// Value of the smoothing kernel for two particles.
  /// @{
  template<class PV>
    requires (has<PV>(required_fields) && has_const<PV>(h))
  constexpr auto operator()(this auto& self, PV a, PV b) noexcept {
    return self(r[a, b], h[a]);
  }
  template<class PV, class Num>
    requires (has<PV>(required_fields) && !has_const<PV>(h))
  constexpr auto operator()(this auto& self, PV a, PV b, Num h) noexcept {
    return self(r[a, b], h);
  }
  /// @}

  /// Directional derivative of the smoothing kernel for two particles.
  /// @{
  template<class PV>
    requires (has<PV>(required_fields) && has_const<PV>(h))
  constexpr auto deriv(this auto& self, PV a, PV b) noexcept {
    return self.deriv(r[a, b], h[a]);
  }
  template<class PV, class Num>
    requires (has<PV>(required_fields) && !has_const<PV>(h))
  constexpr auto deriv(this auto& self, PV a, PV b, Num h) noexcept {
    return self.deriv(r[a, b], h);
  }
  /// @}

  /// Value of the smoothing kernel for two particles.
  /// @{
  template<class PV>
    requires (has<PV>(required_fields) && has_const<PV>(h))
  constexpr auto grad(this auto& self, PV a, PV b) noexcept {
    return self.grad(r[a, b], h[a]);
  }
  template<class PV, class Num>
    requires (has<PV>(required_fields) && !has_const<PV>(h))
  constexpr auto grad(this auto& self, PV a, PV b, Num h) noexcept {
    return self.grad(r[a, b], h);
  }
  /// @}

  /// Width derivative of the smoothing kernel for two points.
  /// @{
  template<class PV>
    requires (has<PV>(required_fields) && has_const<PV>(h))
  constexpr auto width_deriv(this auto& self, //
                             PV a,
                             PV b) noexcept {
    return self.width_deriv(r[a, b], h[a]);
  }
  template<class PV, class Num>
    requires (has<PV>(required_fields) && !has_const<PV>(h))
  constexpr auto width_deriv(this auto& self, PV a, PV b, Num h) noexcept {
    return self.width_deriv(r[a, b], h);
  }
  /// @}

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Support radius.
  template<class Num>
  constexpr auto radius(this auto& self, Num h) noexcept -> Num {
    TIT_ASSERT(h > Num{0.0}, "Kernel width must be positive!");
    auto const radius = self.template unit_radius<Num>();
    return radius * h;
  }

  /// Value of the smoothing kernel at point.
  template<class Num, size_t Dim>
  constexpr auto operator()(this auto& self,
                            Vec<Num, Dim> x,
                            Num h) noexcept -> Num {
    TIT_ASSERT(h > Num{0.0}, "Kernel width must be positive!");
    auto const h_inverse = inverse(h);
    auto const w = self.template weight<Num, Dim>() * pow(h_inverse, Dim);
    auto const q = h_inverse * norm(x);
    return w * self.unit_value(q);
  }

  /// Directional derivative of the smoothing kernel at point.
  template<class Num, size_t Dim>
  constexpr auto deriv(this auto& self,
                       Vec<Num, Dim> x,
                       Num h) noexcept -> Num {
    TIT_ASSERT(h > Num{0.0}, "Kernel width must be positive!");
    auto const h_inverse = inverse(h);
    auto const w = self.template weight<Num, Dim>() * pow(h_inverse, Dim);
    auto const q = h_inverse * norm(x);
    return w * self.unit_deriv(q) * h_inverse;
  }

  /// Spatial gradient of the smoothing kernel at point.
  template<class Num, size_t Dim>
  constexpr auto grad(this auto& self,
                      Vec<Num, Dim> x,
                      Num h) noexcept -> Vec<Num, Dim> {
    TIT_ASSERT(h > Num{0.0}, "Kernel width must be positive!");
    auto const h_inverse = inverse(h);
    auto const w = self.template weight<Num, Dim>() * pow(h_inverse, Dim);
    auto const q = h_inverse * norm(x);
    auto const grad_q = normalize(x) * h_inverse;
    return w * self.unit_deriv(q) * grad_q;
  }

  /// Width derivative of the smoothing kernel at point.
  template<class Num, size_t Dim>
  constexpr auto width_deriv(this auto& self,
                             Vec<Num, Dim> x,
                             Num h) noexcept -> Num {
    TIT_ASSERT(h > Num{0.0}, "Kernel width must be positive!");
    auto const h_inverse = inverse(h);
    auto const w = self.template weight<Num, Dim>() * pow(h_inverse, Dim);
    auto const dw_dh = -int{Dim} * w * h_inverse;
    auto const q = h_inverse * norm(x);
    auto const dq_dh = -q * h_inverse;
    return dw_dh * self.unit_value(q) + w * self.unit_deriv(q) * dq_dh;
  }

}; // class BaseKernel

/// Smoothing kernel type.
template<class Kernel>
concept kernel = std::movable<Kernel> && std::derived_from<Kernel, BaseKernel>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Bell-shaped smoothing kernel.
// TODO: Implement me! What am I?
class BellShapedKernel : public BaseKernel {};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Gaussian smoothing kernel (Monaghan, 1992).
class GaussianKernel final : public BaseKernel {
public:

  /// Kernel weight.
  template<class Num, size_t Dim>
  static constexpr auto weight() noexcept -> Num {
    static_assert(1 <= Dim);
    return pow(std::numbers::inv_sqrtpi_v<Num>, Dim);
  }

  /// Unit support radius.
  template<class Num>
  static constexpr auto unit_radius() noexcept -> Num {
    // We truncate gaussian at the point, where it reaches minimal non-zero
    // value for the current floating-point type. It approximately is 9.3454
    // for float and 26.6157 for double. Gaussian value at this point is
    // approximately 10^-38 for float and 10^-308 for double.
    return sqrt(-log(std::numeric_limits<Num>::min()));
  }

  /// Value of the unit smoothing kernel at a point.
  template<class Num>
  static constexpr auto unit_value(Num q) noexcept -> Num {
    return exp(-pow2(q));
  }

  /// Derivative of the unit smoothing kernel at a point.
  template<class Num>
  static constexpr auto unit_deriv(Num q) noexcept -> Num {
    return -Num{2.0} * q * exp(-pow2(q));
  }

}; // class GaussianKernel

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Super-gaussian smoothing kernel (Monaghan, 1992).
// TODO: implement me!
class SuperGaussianKernel {};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Cubic B-spline (M4) smoothing kernel.
class CubicSplineKernel final : public BaseKernel {
public:

  /// Kernel weight.
  template<class Num, size_t Dim>
  static constexpr auto weight() noexcept -> Num {
    if constexpr (Dim == 1) return Num{2.0 / 3.0};
    else if constexpr (Dim == 2) return Num{10.0 / 7.0 * std::numbers::inv_pi};
    else if constexpr (Dim == 3) return std::numbers::inv_pi_v<Num>;
    else static_assert(false);
  }

  /// Unit support radius.
  template<class Num>
  static constexpr auto unit_radius() noexcept -> Num {
    return Num{2.0};
  }

  /// Value of the unit smoothing kernel at a point.
  template<class Num>
  static constexpr auto unit_value(Num q) noexcept -> Num {
    constexpr auto qi = Vec{Num{2.0}, Num{1.0}};
    constexpr auto wi = Vec{Num{0.25}, Num{-1.0}};
#if TIT_BRANCHLESS_KERNELS
    auto const qv = Vec<Num, 2>(q);
    return sum(filter(qv < qi, wi * pow3(qi - qv)));
#else
    auto W = Num{0.0};
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
  template<class Num>
  static constexpr auto unit_deriv(Num q) noexcept -> Num {
    constexpr auto qi = Vec{Num{2.0}, Num{1.0}};
    constexpr auto wi = Vec{Num{0.25}, Num{-1.0}};
#if TIT_BRANCHLESS_KERNELS
    auto const qv = Vec<Num, 2>(q);
    return sum(filter(qv < qi, wi * Num{-3.0} * pow2(qi - qv)));
#else
    auto dW_dq = Num{0.0};
    if (q < qi[0]) {
      dW_dq += wi[0] * Num{-3.0} * pow2(qi[0] - q);
      if (q < qi[1]) {
        dW_dq += wi[1] * Num{-3.0} * pow2(qi[1] - q);
      }
    }
    return dW_dq;
#endif
  }

}; // class CubicSplineKernel

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Cubic B-spline (M4) smoothing kernel
/// with modified derivative (Thomas, Couchman, 1992).
class ThomasCouchmanKernel final : public BaseKernel {
public:

  /// Kernel weight.
  template<class Num, size_t Dim>
  static consteval auto weight() noexcept -> Num {
    return CubicSplineKernel::weight<Num, Dim>();
  }

  /// Unit support radius.
  template<class Num>
  static consteval auto unit_radius() noexcept -> Num {
    return CubicSplineKernel::unit_radius<Num>();
  }

  /// Value of the unit smoothing kernel at a point.
  template<class Num>
  static constexpr auto unit_value(Num q) noexcept -> Num {
    return CubicSplineKernel::unit_value(q);
  }

  /// Derivative of the unit smoothing kernel at a point.
  template<class Num>
  static constexpr auto unit_deriv(Num q) noexcept -> Num {
    // TODO: provide a branchless implementation.
    if (q < Num{2.0 / 3.0}) return Num{-1.0};
    if (q < Num{1.0}) return (Num{2.25} * q - Num{3.0}) * q;
    if (q < Num{2.0}) return Num{0.75} * pow2(Num{2.0} - q);
    return Num{0.0};
  }

}; // class ThomasCouchmanKernel

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// The quartic B-spline (M5) smoothing kernel.
class QuarticSplineKernel final : public BaseKernel {
public:

  /// Kernel weight.
  template<class Num, size_t Dim>
  static constexpr auto weight() noexcept -> Num {
    using std::numbers::inv_pi;
    if constexpr (Dim == 1) return Num{1.0 / 24.0};
    else if constexpr (Dim == 2) return Num{96.0 / 1199.0 * inv_pi};
    else if constexpr (Dim == 3) return Num{1.0 / 20.0 * inv_pi};
    else static_assert(false);
  }

  /// Unit support radius.
  template<class Num>
  static constexpr auto unit_radius() noexcept -> Num {
    return Num{2.5};
  }

  /// Value of the unit smoothing kernel at a point.
  template<class Num>
  static constexpr auto unit_value(Num q) noexcept -> Num {
    constexpr auto qi = Vec{Num{2.5}, Num{1.5}, Num{0.5}};
    constexpr auto wi = Vec{Num{1.0}, Num{-5.0}, Num{10.0}};
#if TIT_BRANCHLESS_KERNELS
    auto const qv = Vec<Num, 3>(q);
    return sum(filter(qv < qi, wi * pow4(qi - qv)));
#else
    auto W = Num{0.0};
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
  template<class Num>
  static constexpr auto unit_deriv(Num q) noexcept -> Num {
    constexpr auto qi = Vec{Num{2.5}, Num{1.5}, Num{0.5}};
    constexpr auto wi = Vec{Num{1.0}, Num{-5.0}, Num{10.0}};
#if TIT_BRANCHLESS_KERNELS
    auto const qv = Vec<Num, 3>(q);
    return sum(filter(qv < qi, wi * Num{-4.0} * pow3(qi - qv)));
#else
    auto dW_dq = Num{0.0};
    if (q < qi[0]) {
      dW_dq += wi[0] * Num{-4.0} * pow3(qi[0] - q);
      if (q < qi[1]) {
        dW_dq += wi[1] * Num{-4.0} * pow3(qi[1] - q);
        if (q < qi[2]) {
          dW_dq += wi[2] * Num{-4.0} * pow3(qi[2] - q);
        }
      }
    }
    return dW_dq;
#endif
  }

}; // class QuarticSplineKernel

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Quintic B-spline (M6) smoothing kernel.
class QuinticSplineKernel final : public BaseKernel {
public:

  /// Kernel weight.
  template<class Num, size_t Dim>
  static constexpr auto weight() noexcept {
    using std::numbers::inv_pi;
    if constexpr (Dim == 1) return Num{1.0 / 120.0};
    else if constexpr (Dim == 2) return Num{7.0 / 478.0 * inv_pi};
    else if constexpr (Dim == 3) return Num{1.0 / 120.0 * inv_pi}; // or 3/359?
    else static_assert(false);
  }

  /// Unit support radius.
  template<class Num>
  static constexpr auto unit_radius() noexcept -> Num {
    return Num{3.0};
  }

  /// Value of the unit smoothing kernel at a point.
  template<class Num>
  static constexpr auto unit_value(Num q) noexcept -> Num {
    constexpr auto qi = Vec{Num{3.0}, Num{2.0}, Num{1.0}};
    constexpr auto wi = Vec{Num{1.0}, Num{-6.0}, Num{15.0}};
#if TIT_BRANCHLESS_KERNELS
    auto const qv = Vec<Num, 3>(q);
    return sum(filter(qv < qi, wi * pow5(qi - qv)));
#else
    auto W = Num{0.0};
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
  template<class Num>
  static constexpr auto unit_deriv(Num q) noexcept -> Num {
    constexpr auto qi = Vec{Num{3.0}, Num{2.0}, Num{1.0}};
    constexpr auto wi = Vec{Num{1.0}, Num{-6.0}, Num{15.0}};
#if TIT_BRANCHLESS_KERNELS
    auto const qv = Vec<Num, 3>(q);
    return sum(filter(qv < qi, wi * Num{-5.0} * pow4(qi - qv)));
#else
    auto dW_dq = Num{0.0};
    if (q < qi[0]) {
      dW_dq += wi[0] * Num{-5.0} * pow4(qi[0] - q);
      if (q < qi[1]) {
        dW_dq += wi[1] * Num{-5.0} * pow4(qi[1] - q);
        if (q < qi[2]) {
          dW_dq += wi[2] * Num{-5.0} * pow4(qi[2] - q);
        }
      }
    }
    return dW_dq;
#endif
  }

}; // class QuinticSplineKernel

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Abstract Wendland's smoothing kernel.
class WendlandKernel : public BaseKernel {
public:

  /// Unit support radius.
  template<class Num>
  static constexpr auto unit_radius() noexcept -> Num {
    // Wendland's kernels always have a support radius of 2.
    return Num{2.0};
  }

  /// Value of the unit smoothing kernel at a point.
  template<class Num>
  constexpr auto unit_value(this auto& self, Num q) noexcept -> Num {
    return q < Num{2.0} ? self.unit_value_notrunc(q) : Num{0.0};
  }

  /// Derivative of the unit smoothing kernel at a point.
  template<class Num>
  constexpr auto unit_deriv(this auto& self, Num q) noexcept -> Num {
    return q < Num{2.0} ? self.unit_deriv_notrunc(q) : Num{0.0};
  }

}; // class WendlandKernel

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Wendland's quartic (C2) smoothing kernel (Wendland, 1995).
class QuarticWendlandKernel final : public WendlandKernel {
public:

  /// Kernel weight.
  template<class Num, size_t Dim>
  static constexpr auto weight() noexcept -> Num {
    using std::numbers::inv_pi;
    if constexpr (Dim == 1) return Num{3.0 / 4.0};
    else if constexpr (Dim == 2) return Num{7.0 / 4.0 * inv_pi};
    else if constexpr (Dim == 3) return Num{21.0 / 16.0 * inv_pi};
    else static_assert(false);
  }

  /// Value of the unit smoothing kernel at a point (no truncation).
  template<class Num>
  static constexpr auto unit_value_notrunc(Num q) noexcept -> Num {
    return (Num{1.0} + Num{2.0} * q) * pow4(Num{1.0} - Num{0.5} * q);
  }

  /// Derivative of the unit smoothing kernel at a point (no truncation).
  template<class Num>
  static constexpr auto unit_deriv_notrunc(Num q) noexcept -> Num {
    // Well known formula is dW/dq = -5 * q * (1 - q/2)^3, but it requires 5
    // multiplications. Formula that is used requires 4.
    return Num{5.0 / 8.0} * q * pow3(q - Num{2.0});
  }

}; // class QuarticWendlandKernel

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Wendland's 6-th order (C4) smoothing kernel (Wendland, 1995).
class SixthOrderWendlandKernel final : public WendlandKernel {
public:

  /// Kernel weight.
  template<class Num, size_t Dim>
  static constexpr auto weight() noexcept -> Num {
    using std::numbers::inv_pi;
    if constexpr (Dim == 1) return Num{27.0 / 16.0};
    else if constexpr (Dim == 2) return Num{9.0 / 4.0 * inv_pi};
    else if constexpr (Dim == 3) return Num{495.0 / 256.0 * inv_pi};
    else static_assert(false);
  }

  /// Value of the unit smoothing kernel at a point (no truncation).
  template<class Num>
  static constexpr auto unit_value_notrunc(Num q) noexcept -> Num {
    return horner(q, {Num{1.0}, Num{3.0}, Num{35.0 / 12.0}}) *
           pow6(Num{1.0} - Num{0.5} * q);
  }

  /// Derivative of the unit smoothing kernel at a point (no truncation).
  template<class Num>
  static constexpr auto unit_deriv_notrunc(Num q) noexcept -> Num {
    return Num{7.0 / 96.0} * q * //
           horner(q, {Num{2.0}, Num{5.0}}) * pow5(q - Num{2.0});
  }

}; // class SixthOrderWendlandKernel

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Wendland's 8-th order (C6) smoothing kernel (Wendland, 1995).
class EighthOrderWendlandKernel final : public WendlandKernel {
public:

  /// Kernel weight.
  template<class Num, size_t Dim>
  static constexpr auto weight() noexcept -> Num {
    using std::numbers::inv_pi;
    if constexpr (Dim == 1) return Num{15.0 / 8.0};
    else if constexpr (Dim == 2) return Num{39.0 / 14.0 * inv_pi};
    else if constexpr (Dim == 3) return Num{339.0 / 128.0 * inv_pi};
    else static_assert(false);
  }

  /// Value of the unit smoothing kernel at a point (no truncation).
  template<class Num>
  static constexpr auto unit_value_notrunc(Num q) noexcept -> Num {
    return horner(q, {Num{1.0}, Num{4.0}, Num{25.0 / 4.0}, Num{4.0}}) *
           pow8(Num{1.0} - Num{0.5} * q);
  }

  /// Derivative of the unit smoothing kernel at a point (no truncation).
  template<class Num>
  static constexpr auto unit_deriv_notrunc(Num q) noexcept -> Num {
    return Num{11.0 / 512.0} * q * horner(q, {Num{2.0}, Num{7.0}, Num{8.0}}) *
           pow7(q - Num{2.0});
  }

}; // class SixthOrderWendlandKernel

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
