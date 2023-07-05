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

#include <numbers>

#include "tit/utils/assert.hpp"
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

  constexpr const Derived& _self() const noexcept {
    return static_cast<const Derived&>(*this);
  }

public:

  /** Support radius. */
  template<class Real>
  constexpr Real radius(Real h) const noexcept {
    TIT_ASSERT(h > Real{0.0}, "Kernel width must be positive!");
    const auto radius = _self().template unit_radius<Real>();
    return radius * h;
  }

  /** Value of the smoothing kernel at point. */
  template<class Real, dim_t Dim>
  constexpr Real operator()(Point<Real, Dim> r, Real h) const noexcept {
    TIT_ASSERT(h > Real{0.0}, "Kernel width must be positive!");
    const auto h_inverse = inverse(h);
    const auto q = h_inverse * norm(r);
    const auto w = _self().template weight<Real, Dim>();
    return pow(h_inverse, Dim) * w * _self().unit_value(q);
  }

  /** Spatial gradient value of the smoothing kernel at point. */
  template<class Real, dim_t Dim>
  constexpr Vec<Real, Dim> grad(Point<Real, Dim> r, Real h) const noexcept {
    TIT_ASSERT(h > Real{0.0}, "Kernel width must be positive!");
    const auto h_inverse = inverse(h);
    const auto q = h_inverse * norm(r);
    const auto w = _self().template weight<Real, Dim>();
    return pow(h_inverse, Dim + 1) * w * _self().unit_deriv(q) * normalize(r);
  }

  /** Width derivative value of the smoothing kernel at point. */
  template<class Real, dim_t Dim>
  constexpr Real radius_deriv(Point<Real, Dim> r, Real h) const noexcept {
    TIT_ASSERT(h > Real{0.0}, "Kernel width must be positive!");
    const auto h_inverse = inverse(h);
    const auto q = h_inverse * norm(r);
    const auto w = _self().template weight<Real, Dim>();
    return pow(h_inverse, Dim + 1) * w *
           (-Dim * _self().unit_value(q) - q * _self().unit_deriv(q));
  }

}; // class Kernel

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** The gaussian smoothing kernel.
\******************************************************************************/
class GaussianKernel final : public Kernel<GaussianKernel> {
public:

  /** Kernel weight. */
  template<class Real, dim_t Dim>
  static consteval Real weight() noexcept {
    static_assert(1 <= Dim);
    return pow(std::numbers::inv_sqrtpi_v<Real>, Dim);
  }

  /** Unit support radius. */
  template<class Real>
  static consteval Real unit_radius() noexcept {
    // We truncate gaussian at the point, where it reaches minimal non-zero
    // value for the current floating-point type. It approximately is 9.3454
    // for float and 26.6157 for double. Gaussian value at this point is
    // approximately 10^-38 for float and 10^-308 for double.
    return sqrt(-log(std::numeric_limits<Real>::min()));
  }

  /** Value of the unit smoothing kernel at a point. */
  template<class Real>
  static constexpr Real unit_value(Real q) noexcept {
    return exp(-pow2(q));
  }

  /** Derivative value of the unit smoothing kernel at a point. */
  template<class Real>
  static constexpr Real unit_deriv(Real q) noexcept {
    return -Real{2.0} * q * exp(-pow2(q));
  }

}; // class GaussianKernel

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** The cubic B-spline (M4) smoothing kernel.
\******************************************************************************/
class CubicKernel final : public Kernel<CubicKernel> {
public:

  /** Kernel weight. */
  template<class Real, dim_t Dim>
  static consteval Real weight() noexcept {
    static_assert(1 <= Dim && Dim <= 3);
    switch (Dim) {
      case 1: return Real{2.0 / 3.0};
      case 2: return Real{10.0 / 7.0 * std::numbers::inv_pi};
      case 3: return std::numbers::inv_pi_v<Real>;
    }
  }

  /** Unit support radius. */
  template<class Real>
  static consteval Real unit_radius() noexcept {
    return Real{2.0};
  }

  /** Value of the unit smoothing kernel at a point. */
  template<class Real>
  static constexpr Real unit_value(Real q) noexcept {
    if (Real{0.0} <= q && q < Real{1.0}) {
      return Real{0.25} * pow3(Real{2.0} - q) - pow3(Real{1.0} - q);
    }
    if (Real{1.0} <= q && q < Real{2.0}) {
      return Real{0.25} * pow3(Real{2.0} - q);
    }
    return Real{0.0};
  }

  /** Derivative value of the unit smoothing kernel at a point. */
  template<class Real>
  static constexpr Real unit_deriv(Real q) noexcept {
    if (Real{0.0} <= q && q < Real{1.0}) {
      return Real{-0.75} * pow2(Real{2.0} - q) +
             Real{3.0} * pow2(Real{1.0} - q);
    }
    if (Real{1.0} <= q && q < Real{2.0}) {
      return Real{-0.75} * pow2(Real{2.0} - q);
    }
    return Real{0.0};
  }

}; // class CubicKernel

/******************************************************************************\
 ** The cubic B-spline (M4) smoothing kernel
 ** with Thomas-Couchman (1992) modified derivative.
\******************************************************************************/
class ThomasCouchmanKernel final : public Kernel<ThomasCouchmanKernel> {
public:

  /** Kernel weight. */
  template<class Real, dim_t Dim>
  static consteval Real weight() noexcept {
    return CubicKernel::weight<Real, Dim>();
  }

  /** Unit support radius. */
  template<class Real>
  static consteval Real unit_radius() noexcept {
    return CubicKernel::unit_radius<Real>();
  }

  /** Value of the unit smoothing kernel at a point. */
  template<class Real>
  static constexpr Real unit_value(Real q) noexcept {
    return CubicKernel::unit_value(q);
  }

  /** Derivative value of the unit smoothing kernel at a point. */
  template<class Real>
  static constexpr Real unit_deriv(Real q) noexcept {
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

/******************************************************************************\
 ** The quartic B-spline (M5) smoothing kernel.
\******************************************************************************/
class QuarticKernel final : public Kernel<QuarticKernel> {
public:

  /** Kernel weight. */
  template<class Real, dim_t Dim>
  static consteval Real weight() noexcept {
    static_assert(1 <= Dim && Dim <= 3);
    switch (Dim) {
      case 1: return Real{1.0 / 24.0};
      case 2: return Real{96.0 / 1199.0 * std::numbers::inv_pi};
      case 3: return Real{1.0 / 2.0 * std::numbers::inv_pi};
    }
  }

  /** Unit support radius. */
  template<class Real>
  static consteval Real unit_radius() noexcept {
    return Real{2.5};
  }

  /** Value of the unit smoothing kernel at a point. */
  template<class Real>
  static constexpr Real unit_value(Real q) noexcept {
    if (Real{0.0} <= q && q < Real{0.5}) {
      return Real{1.0} * pow4(Real{2.5} - q) -
             Real{5.0} * pow4(Real{1.5} - q) + //
             Real{10.0} * pow4(Real{0.5} - q);
    }
    if (Real{0.5} <= q && q < Real{1.5}) {
      return Real{1.0} * pow4(Real{2.5} - q) - //
             Real{5.0} * pow4(Real{1.5} - q);
    }
    if (Real{1.5} <= q && q < Real{2.5}) {
      return Real{1.0} * pow4(Real{2.5} - q);
    }
    return Real{0.0};
  }

  /** Derivative value of the unit smoothing kernel at a point. */
  template<class Real>
  static constexpr Real unit_deriv(Real q) noexcept {
    if (Real{0.0} <= q && q < Real{0.5}) {
      return Real{-4.0} * pow3(Real{2.5} - q) +
             Real{20.0} * pow3(Real{1.5} - q) -
             Real{40.0} * pow3(Real{0.5} - q);
    }
    if (Real{0.5} <= q && q < Real{1.5}) {
      return Real{-4.0} * pow3(Real{2.5} - q) +
             Real{20.0} * pow3(Real{1.5} - q);
    }
    if (Real{1.5} <= q && q < Real{2.5}) {
      return Real{-4.0} * pow3(Real{2.5} - q);
    }
    return Real{0.0};
  }

}; // class QuarticKernel

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** The quintic B-spline (M6) smoothing kernel.
\******************************************************************************/
class QuinticKernel final : public Kernel<QuinticKernel> {
public:

  /** Kernel weight. */
  template<class Real, dim_t Dim>
  static consteval Real weight() noexcept {
    static_assert(1 <= Dim && Dim <= 3);
    switch (Dim) {
      case 1: return Real{1.0 / 120.0};
      case 2: return Real{7.0 / 478.0 * std::numbers::inv_pi};
      case 3: return Real{1.0 / 120.0 * std::numbers::inv_pi};
    }
  }

  /** Unit support radius. */
  template<class Real>
  static consteval Real unit_radius() noexcept {
    return Real{3.0};
  }

  /** Value of the unit smoothing kernel at a point. */
  template<class Real>
  static constexpr Real unit_value(Real q) noexcept {
    if (Real{0.0} <= q && q < Real{1.0}) {
      return Real{1.0} * pow5(Real{3.0} - q) -
             Real{6.0} * pow5(Real{2.0} - q) + //
             Real{15.0} * pow5(Real{1.0} - q);
    }
    if (Real{1.0} <= q && q < Real{2.0}) {
      return Real{1.0} * pow5(Real{3.0} - q) - //
             Real{6.0} * pow5(Real{2.0} - q);
    }
    if (Real{2.0} <= q && q < Real{3.0}) {
      return Real{1.0} * pow5(Real{3.0} - q);
    }
    return Real{0.0};
  }

  /** Derivative value of the unit smoothing kernel at a point. */
  template<class Real>
  static constexpr Real unit_deriv(Real q) noexcept {
    if (Real{0.0} <= q && q < Real{1.0}) {
      return Real{-5.0} * pow4(Real{3.0} - q) +
             Real{30.0} * pow4(Real{2.0} - q) -
             Real{75.0} * pow4(Real{1.0} - q);
    }
    if (Real{1.0} <= q && q < Real{2.0}) {
      return Real{-5.0} * pow4(Real{3.0} - q) +
             Real{30.0} * pow4(Real{2.0} - q);
    }
    if (Real{2.0} <= q && q < Real{3.0}) {
      return Real{-5.0} * pow4(Real{3.0} - q);
    }
    return Real{0.0};
  }

}; // class QuinticKernel

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit::sph
