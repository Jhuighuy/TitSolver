/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*\
 * Copyright (C) 2020-2023 Oleg Butakov                                       *
 *                                                                            *
 * Permission is hereby granted, free of charge, to any person obtaining a    *
 * copy of this software and associated documentation files (the "Software"), *
 * to deal in the Software without restriction, including without limitation  *
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,   *
 * and/or sell copies of the Software, and to permit persons to whom the      *
 * Software is furnished to do so, subject to the following conditions:       *
 *                                                                            *
 * The above copyright notice and this permission notice shall be included in *
 * all copies or substantial portions of the Software.                        *
 *                                                                            *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR *
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   *
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    *
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER *
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    *
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        *
 * DEALINGS IN THE SOFTWARE.                                                  *
\*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#pragma once

#include <limits>
#include <numbers>

#include "tit/utils/assert.hpp"
#include "tit/utils/config.hpp"
#include "tit/utils/math.hpp"
#include "tit/utils/vec.hpp"

namespace tit::sph {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** Abstract smoothing kernel.
\******************************************************************************/
template<class Derived>
class Kernel {
private:

  constexpr auto _self() const noexcept -> const Derived& {
    return static_cast<const Derived&>(*this);
  }

public:

  /** Kernel weight. */
  template<class Real, dim_t Dim>
  constexpr auto weight() const noexcept -> Real {
    // A shortcut in order not to write `.template weight<...>` all the time.
    return _self().template weight<Real, Dim>();
  }

  /** Support radius. */
  template<class Real>
  constexpr auto radius(Real h) const noexcept -> Real {
    TIT_ASSERT(h > Real{0.0}, "Kernel width must be positive!");
    const auto radius = _self().template unit_radius<Real>();
    return radius * h;
  }

  /** Value of the smoothing kernel at point. */
  template<class Real, dim_t Dim>
  constexpr auto operator()(Point<Real, Dim> r, Real h) const noexcept -> Real {
    TIT_ASSERT(h > Real{0.0}, "Kernel width must be positive!");
    const auto h_inverse = inverse(h);
    const auto w = weight<Real, Dim>() * pow(h_inverse, Dim);
    const auto q = h_inverse * norm(r);
    return w * _self().unit_value(q);
  }

  /** Spatial gradient value of the smoothing kernel at point. */
  template<class Real, dim_t Dim>
  constexpr auto grad(Point<Real, Dim> r, Real h) const noexcept
      -> Vec<Real, Dim> {
    TIT_ASSERT(h > Real{0.0}, "Kernel width must be positive!");
    const auto h_inverse = inverse(h);
    const auto w = weight<Real, Dim>() * pow(h_inverse, Dim);
    const auto q = h_inverse * norm(r);
    const auto grad_q = normalize(r) * h_inverse;
    return w * _self().unit_deriv(q) * grad_q;
  }

  /** Width derivative value of the smoothing kernel at point. */
  template<class Real, dim_t Dim>
  constexpr auto radius_deriv(Point<Real, Dim> r, Real h) const noexcept
      -> Real {
    TIT_ASSERT(h > Real{0.0}, "Kernel width must be positive!");
    const auto h_inverse = inverse(h);
    const auto w = weight<Real, Dim>() * pow(h_inverse, Dim);
    const auto dw_dh = -Dim * w * h_inverse;
    const auto q = h_inverse * norm(r);
    const auto dq_dh = -q * h_inverse;
    return dw_dh * _self().unit_value(q) + w * _self().unit_deriv(q) * dq_dh;
  }

}; // class Kernel

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** Gaussian smoothing kernel (Monaghan, 1992).
\******************************************************************************/
class GaussianKernel final : public Kernel<GaussianKernel> {
public:

  /** Kernel weight. */
  template<class Real, dim_t Dim>
  static constexpr Real weight() noexcept {
    static_assert(1 <= Dim);
    return pow(std::numbers::inv_sqrtpi_v<Real>, Dim);
  }

  /** Unit support radius. */
  template<class Real>
  static constexpr auto unit_radius() noexcept -> Real {
    // We truncate gaussian at the point, where it reaches minimal non-zero
    // value for the current floating-point type. It approximately is 9.3454
    // for float and 26.6157 for double. Gaussian value at this point is
    // approximately 10^-38 for float and 10^-308 for double.
    return sqrt(-log(std::numeric_limits<Real>::min()));
  }

  /** Value of the unit smoothing kernel at a point. */
  template<class Real>
  static constexpr auto unit_value(Real q) noexcept -> Real {
    return exp(-pow2(q));
  }

  /** Derivative value of the unit smoothing kernel at a point. */
  template<class Real>
  static constexpr auto unit_deriv(Real q) noexcept -> Real {
    return -Real{2.0} * q * exp(-pow2(q));
  }

}; // class GaussianKernel

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** Cubic B-spline (M4) smoothing kernel.
\******************************************************************************/
class CubicKernel final : public Kernel<CubicKernel> {
public:

  /** Kernel weight. */
  template<class Real, dim_t Dim>
  static constexpr auto weight() noexcept -> Real {
    static_assert(1 <= Dim && Dim <= 3);
    switch (Dim) {
      case 1: return Real{2.0 / 3.0};
      case 2: return Real{10.0 / 7.0 * std::numbers::inv_pi};
      case 3: return std::numbers::inv_pi_v<Real>;
    }
  }

  /** Unit support radius. */
  template<class Real>
  static constexpr auto unit_radius() noexcept -> Real {
    return Real{2.0};
  }

  /** Value of the unit smoothing kernel at a point. */
  template<class Real>
  static constexpr auto unit_value(Real q) noexcept -> Real {
    constexpr auto qi = Vec{Real{2.0}, Real{1.0}};
    constexpr auto wi = Vec{Real{0.25}, Real{-1.0}};
#if TIT_BRANCHLESS_KERNELS
    const auto qv = Vec<Real, 2>(q);
    return sum(merge(qv < qi, wi * pow3(qi - qv)));
#else
    auto k = Real{0.0};
    if (q < qi[0]) {
      k += wi[0] * pow3(qi[0] - q);
      if (q < qi[1]) {
        k += wi[1] * pow3(qi[1] - q);
      }
    }
    return k;
#endif
  }

  /** Derivative value of the unit smoothing kernel at a point. */
  template<class Real>
  static constexpr Real unit_deriv(Real q) noexcept {
    constexpr auto qi = Vec{Real{2.0}, Real{1.0}};
    constexpr auto wi = Vec{Real{0.25}, Real{-1.0}};
#if TIT_BRANCHLESS_KERNELS
    const auto qv = Vec<Real, 2>(q);
    return sum(merge(qv < qi, wi * Real{-3.0} * pow2(qi - qv)));
#else
    auto dk_dq = Real{0.0};
    if (q < qi[0]) {
      dk_dq += wi[0] * Real{-3.0} * pow2(qi[0] - q);
      if (q < qi[1]) {
        dk_dq += wi[1] * Real{-3.0} * pow2(qi[1] - q);
      }
    }
    return dk_dq;
#endif
  }

}; // class CubicKernel

/******************************************************************************\
 ** The quartic B-spline (M5) smoothing kernel.
\******************************************************************************/
class QuarticKernel final : public Kernel<QuarticKernel> {
public:

  /** Kernel weight. */
  template<class Real, dim_t Dim>
  static constexpr auto weight() noexcept -> Real {
    static_assert(1 <= Dim && Dim <= 3);
    switch (Dim) {
      case 1: return Real{1.0 / 24.0};
      case 2: return Real{96.0 / 1199.0 * std::numbers::inv_pi};
      case 3: return Real{1.0 / 20.0 * std::numbers::inv_pi};
    }
  }

  /** Unit support radius. */
  template<class Real>
  static constexpr auto unit_radius() noexcept -> Real {
    return Real{2.5};
  }

  /** Value of the unit smoothing kernel at a point. */
  template<class Real>
  static constexpr auto unit_value(Real q) noexcept -> Real {
    constexpr auto qi = Vec{Real{2.5}, Real{1.5}, Real{0.5}};
    constexpr auto wi = Vec{Real{1.0}, Real{-5.0}, Real{10.0}};
#if TIT_BRANCHLESS_KERNELS
    const auto qv = Vec<Real, 3>(q);
    return sum(merge(qv < qi, wi * pow4(qi - qv)));
#else
    auto k = Real{0.0};
    if (q < qi[0]) {
      k += wi[0] * pow4(qi[0] - q);
      if (q < qi[1]) {
        k += wi[1] * pow4(qi[1] - q);
        if (q < qi[2]) {
          k += wi[2] * pow4(qi[2] - q);
        }
      }
    }
    return k;
#endif
  }

  /** Derivative value of the unit smoothing kernel at a point. */
  template<class Real>
  static constexpr auto unit_deriv(Real q) noexcept -> Real {
    constexpr auto qi = Vec{Real{2.5}, Real{1.5}, Real{0.5}};
    constexpr auto wi = Vec{Real{1.0}, Real{-5.0}, Real{10.0}};
#if TIT_BRANCHLESS_KERNELS
    const auto qv = Vec<Real, 3>(q);
    return sum(merge(qv < qi, wi * Real{-4.0} * pow3(qi - qv)));
#else
    auto dk_dq = Real{0.0};
    if (q < qi[0]) {
      dk_dq += wi[0] * Real{-4.0} * pow3(qi[0] - q);
      if (q < qi[1]) {
        dk_dq += wi[1] * Real{-4.0} * pow3(qi[1] - q);
        if (q < qi[2]) {
          dk_dq += wi[2] * Real{-4.0} * pow3(qi[2] - q);
        }
      }
    }
    return dk_dq;
#endif
  }

}; // class QuarticKernel

/******************************************************************************\
 ** Quintic B-spline (M6) smoothing kernel.
\******************************************************************************/
class QuinticKernel final : public Kernel<QuinticKernel> {
public:

  /** Kernel weight. */
  template<class Real, dim_t Dim>
  static constexpr auto weight() noexcept {
    static_assert(1 <= Dim && Dim <= 3);
    switch (Dim) {
      case 1: return Real{1.0 / 120.0};
      case 2: return Real{7.0 / 478.0 * std::numbers::inv_pi};
      case 3: return Real{1.0 / 120.0 * std::numbers::inv_pi}; // or 3/359?
    }
  }

  /** Unit support radius. */
  template<class Real>
  static constexpr auto unit_radius() noexcept -> Real {
    return Real{3.0};
  }

  /** Value of the unit smoothing kernel at a point. */
  template<class Real>
  static constexpr auto unit_value(Real q) noexcept -> Real {
    constexpr auto qi = Vec{Real{3.0}, Real{2.0}, Real{1.0}};
    constexpr auto wi = Vec{Real{1.0}, Real{-6.0}, Real{15.0}};
#if TIT_BRANCHLESS_KERNELS
    const auto qv = Vec<Real, 3>(q);
    return sum(merge(qv < qi, wi * pow5(qi - qv)));
#else
    auto k = Real{0.0};
    if (q < qi[0]) {
      k += wi[0] * pow5(qi[0] - q);
      if (q < qi[1]) {
        k += wi[1] * pow5(qi[1] - q);
        if (q < qi[2]) {
          k += wi[2] * pow5(qi[2] - q);
        }
      }
    }
    return k;
#endif
  }

  /** Derivative value of the unit smoothing kernel at a point. */
  template<class Real>
  static constexpr auto unit_deriv(Real q) noexcept -> Real {
    constexpr auto qi = Vec{Real{3.0}, Real{2.0}, Real{1.0}};
    constexpr auto wi = Vec{Real{1.0}, Real{-6.0}, Real{15.0}};
#if TIT_BRANCHLESS_KERNELS
    const auto qv = Vec<Real, 3>(q);
    return sum(merge(qv < qi, wi * Real{-5.0} * pow4(qi - qv)));
#else
    auto dk_dq = Real{0.0};
    if (q < qi[0]) {
      dk_dq += wi[0] * Real{-5.0} * pow4(qi[0] - q);
      if (q < qi[1]) {
        dk_dq += wi[1] * Real{-5.0} * pow4(qi[1] - q);
        if (q < qi[2]) {
          dk_dq += wi[2] * Real{-5.0} * pow4(qi[2] - q);
        }
      }
    }
    return dk_dq;
#endif
  }

}; // class QuinticKernel

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** The quintic (C2) smoothing kernel (Wendland, 1995).
\******************************************************************************/
class WendlandQuarticKernel final : public Kernel<WendlandQuarticKernel> {
public:

  /** Kernel weight. */
  template<class Real, dim_t Dim>
  static constexpr auto weight() noexcept -> Real {
    static_assert(1 <= Dim && Dim <= 3);
    switch (Dim) {
      case 1: return Real{3.0 / 4.0};
      case 2: return Real{7.0 / 4.0 * std::numbers::inv_pi};
      case 3: return Real{21.0 / 16.0 * std::numbers::inv_pi};
    }
  }

  /** Unit support radius. */
  template<class Real>
  static constexpr auto unit_radius() noexcept -> Real {
    return Real{2.0};
  }

  /** Value of the unit smoothing kernel at a point. */
  template<class Real>
  static constexpr auto unit_value(Real q) noexcept -> Real {
    const auto k = [&]() {
      return (Real{1.0} + Real{2.0} * q) * pow4(Real{1.0} - Real{0.5} * q);
    };
#if TIT_BRANCHLESS_KERNELS
    return merge(q < Real{2.0}, k());
#else
    return (q < Real{2.0}) ? k() : Real{0.0};
#endif
  }

  /** Derivative value of the unit smoothing kernel at a point. */
  template<class Real>
  static constexpr auto unit_deriv(Real q) noexcept -> Real {
    const auto dk_dq = [&]() {
      // Common formula is: -5 * q * (1 - q/2)^3, but it requires 5
      // multiplications. Formula that is used requires 4.
      return Real{5.0 / 8.0} * q * pow3(q - Real{2.0});
    };
#if TIT_BRANCHLESS_KERNELS
    return merge(q < Real{2.0}, dk_dq());
#else
    return (q < Real{2.0}) ? dk_dq() : Real{0.0};
#endif
  }

}; // class WendlandQuarticKernel

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** Cubic B-spline (M4) smoothing kernel
 ** with modified derivative (Thomas, Couchman, 1992).
\******************************************************************************/
class ThomasCouchmanKernel final : public Kernel<ThomasCouchmanKernel> {
public:

  /** Kernel weight. */
  template<class Real, dim_t Dim>
  static consteval auto weight() noexcept -> Real {
    return CubicKernel::weight<Real, Dim>();
  }

  /** Unit support radius. */
  template<class Real>
  static consteval auto unit_radius() noexcept -> Real {
    return CubicKernel::unit_radius<Real>();
  }

  /** Value of the unit smoothing kernel at a point. */
  template<class Real>
  static constexpr auto unit_value(Real q) noexcept -> Real {
    return CubicKernel::unit_value(q);
  }

  /** Derivative value of the unit smoothing kernel at a point. */
  template<class Real>
  static constexpr auto unit_deriv(Real q) noexcept -> Real {
    if (Real{0.0} <= q && q < Real{2.0 / 3.0}) {
      return -Real{1.0};
    }
    if (Real{2.0 / 3.0} <= q && q < Real{1.0}) {
      return (Real{2.25} * q - Real{3.0}) * q;
    }
    if (Real{1.0} <= q && q < Real{2.0}) {
      return Real{0.75} * pow2(Real{2.0} - q);
    }
    return Real{0.0};
  }

}; // class ThomasCouchmanKernel

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit::sph
