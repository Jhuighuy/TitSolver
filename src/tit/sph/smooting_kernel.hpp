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

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*\
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
\*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** Abstract smoothing kernel.
\******************************************************************************/
template<class Real, dim_t Dim>
class SmoothingKernel {
public:

  /** Destroy smoothing kernel. */
  virtual ~SmoothingKernel() = default;

  /** Support radius. */
  constexpr Real radius(Real h) const noexcept {
    TIT_ASSERT(h > 0.0, "Kernel width must be positive!");
    return _unit_radius() * h;
  }

  /** Value of the smoothing kernel at point. */
  constexpr Real operator()(Point<Real, Dim> r, Real h) const noexcept {
    TIT_ASSERT(h > 0.0, "Kernel width must be positive!");
    const auto h_inverse = inverse(h);
    const auto q = h_inverse * norm(r);
    return pow(h_inverse, Dim) * _unit_value(q);
  }

  /** Spatial gradient value of the smoothing kernel at point. */
  constexpr Vec<Real, Dim> grad(Point<Real, Dim> r, Real h) const noexcept {
    TIT_ASSERT(h > 0.0, "Kernel width must be positive!");
    const auto h_inverse = inverse(h);
    const auto q = h_inverse * norm(r);
    return pow(h_inverse, Dim + 2) * r * safe_divide(_unit_deriv(q), q);
  }

  /** Width derivative value of the smoothing kernel at point. */
  constexpr Real radius_deriv(Point<Real, Dim> r, Real h) const noexcept {
    TIT_ASSERT(h > 0.0, "Kernel width must be positive!");
    const auto h_inverse = inverse(h);
    const auto q = h_inverse * norm(r);
    return pow(h_inverse, Dim + 1) *
           (-Dim * _unit_value(q) - q * _unit_deriv(q));
  }

private:

  /** Unit support radius. */
  constexpr virtual Real _unit_radius() const noexcept = 0;

  /** Value of the unit smoothing kernel at a point. */
  constexpr virtual Real _unit_value(Real q) const noexcept = 0;

  /** Derivative value of the unit smoothing kernel at a point. */
  constexpr virtual Real _unit_deriv(Real q) const noexcept = 0;

}; // class SmoothingKernel

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*\
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
\*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** The gaussian smoothing kernel.
\******************************************************************************/
template<class Real, dim_t Dim>
  requires (1 <= Dim)
class GaussianSmoothingKernel : public SmoothingKernel<Real, Dim> {
private:

  static constexpr auto _weight = pow(std::numbers::inv_sqrtpi_v<Real>, Dim);

  constexpr Real _unit_radius() const noexcept override {
    return +std::numeric_limits<Real>::infinity();
  }

  constexpr Real _unit_value(Real q) const noexcept override {
    return _weight * exp(-pow2(q));
  }

  constexpr Real _unit_deriv(Real q) const noexcept override {
    return _weight * (-Real{2.0} * q * exp(-pow2(q)));
  }

}; // class GaussianSmoothingKernel

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*\
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
\*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** The cubic B-spline (M4) smoothing kernel.
\******************************************************************************/
template<class Real, dim_t Dim>
  requires (1 <= Dim && Dim <= 3)
class CubicSmoothingKernel : public SmoothingKernel<Real, Dim> {
protected:

  static constexpr auto _weight = []() -> Real {
    switch (Dim) {
      case 1: return Real{2.0 / 3.0};
      case 2: return Real{10.0 / 7.0 * std::numbers::inv_pi};
      case 3: return std::numbers::inv_pi_v<Real>;
    }
  }();

private:

  constexpr Real _unit_radius() const noexcept override {
    return Real{2.0};
  }

  constexpr Real _unit_value(Real q) const noexcept override {
    if (Real{0.0} <= q && q < Real{1.0}) {
      return _weight * (Real{0.25} * pow3(Real{2.0} - q) - pow3(Real{1.0} - q));
    }
    if (Real{1.0} <= q && q < Real{2.0}) {
      return _weight * (Real{0.25} * pow3(Real{2.0} - q));
    }
    return Real{0.0};
  }

  constexpr Real _unit_deriv(Real q) const noexcept override {
    if (Real{0.0} <= q && q < Real{1.0}) {
      return _weight * (Real{-0.75} * pow2(Real{2.0} - q) +
                        Real{3.0} * pow2(Real{1.0} - q));
    }
    if (Real{1.0} <= q && q < Real{2.0}) {
      return _weight * (Real{-0.75} * pow2(Real{2.0} - q));
    }
    return Real{0.0};
  }

}; // class CubicSmoothingKernel

/******************************************************************************\
 ** The cubic B-spline (M4) smoothing kernel
 ** with Thomas-Couchman (1992) modified derivative.
\******************************************************************************/
template<class Real, dim_t Dim>
  requires (1 <= Dim && Dim <= 3)
class ThomasCouchmanSmoothingKernel : public CubicSmoothingKernel<Real, Dim> {
private:

  using CubicSmoothingKernel<Real, Dim>::_weight;

  constexpr Real _unit_deriv(Real q) const noexcept override {
    if (Real{0.0} <= q && q < Real{2.0 / 3.0}) {
      return _weight * (-Real{1.0});
    }
    if (Real{2.0 / 3.0} <= q && q < Real{1.0}) {
      return _weight * (Real{2.25} * q - Real{3.0}) * q;
    }
    if (Real{1.0} <= q && q < Real{2.0}) {
      return _weight * (Real{0.75} * pow2(Real{2.0} - q));
    }
    return Real{0.0};
  }

}; // class ThomasCouchmanSmoothingKernel

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*\
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
\*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** The quartic B-spline (M5) smoothing kernel.
\******************************************************************************/
template<class Real, dim_t Dim>
  requires (1 <= Dim && Dim <= 3)
class QuarticSmoothingKernel final : public SmoothingKernel<Real, Dim> {
private:

  static constexpr auto _weight = []() -> Real {
    switch (Dim) {
      case 1: return Real{1.0 / 24.0};
      case 2: return Real{96.0 / 1199.0 * std::numbers::inv_pi};
      case 3: return Real{1.0 / 2.0 * std::numbers::inv_pi};
    }
  }();

  constexpr Real _unit_radius() const noexcept override {
    return Real{2.5};
  }

  constexpr Real _unit_value(Real q) const noexcept override {
    if (Real{0.0} <= q && q < Real{0.5}) {
      return _weight * (Real{1.0} * pow4(Real{2.5} - q) - //
                        Real{5.0} * pow4(Real{1.5} - q) +
                        Real{10.0} * pow4(Real{0.5} - q));
    }
    if (Real{0.5} <= q && q < Real{1.5}) {
      return _weight * (Real{1.0} * pow4(Real{2.5} - q) - //
                        Real{5.0} * pow4(Real{1.5} - q));
    }
    if (Real{1.5} <= q && q < Real{2.5}) {
      return _weight * (Real{1.0} * pow4(Real{2.5} - q));
    }
    return Real{0.0};
  }

  constexpr Real _unit_deriv(Real q) const noexcept override {
    if (Real{0.0} <= q && q < Real{0.5}) {
      return _weight * (Real{-4.0} * pow3(Real{2.5} - q) +
                        Real{20.0} * pow3(Real{1.5} - q) -
                        Real{40.0} * pow3(Real{0.5} - q));
    }
    if (Real{0.5} <= q && q < Real{1.5}) {
      return _weight * (Real{-4.0} * pow3(Real{2.5} - q) +
                        Real{20.0} * pow3(Real{1.5} - q));
    }
    if (Real{1.5} <= q && q < Real{2.5}) {
      return _weight * (Real{-4.0} * pow3(Real{2.5} - q));
    }
    return Real{0.0};
  }

}; // class QuarticSmoothingKernel

/******************************************************************************\
 ** The quintic B-spline (M6) smoothing kernel.
\******************************************************************************/
template<class Real, dim_t Dim>
  requires (1 <= Dim && Dim <= 3)
class QuinticSmoothingKernel final : public SmoothingKernel<Real, Dim> {
private:

  static constexpr auto _weight = []() -> Real {
    switch (Dim) {
      case 1: return Real{1.0 / 120.0};
      case 2: return Real{7.0 / 478.0 * std::numbers::inv_pi};
      case 3: return Real{1.0 / 120.0 * std::numbers::inv_pi};
    }
  }();

  constexpr Real _unit_radius() const noexcept override {
    return Real{3.0};
  }

  constexpr Real _unit_value(Real q) const noexcept override {
    if (Real{0.0} <= q && q < Real{1.0}) {
      return _weight * (Real{1.0} * pow5(Real{3.0} - q) - //
                        Real{6.0} * pow5(Real{2.0} - q) +
                        Real{15.0} * pow5(Real{1.0} - q));
    }
    if (Real{1.0} <= q && q < Real{2.0}) {
      return _weight * (Real{1.0} * pow5(Real{3.0} - q) - //
                        Real{6.0} * pow5(Real{2.0} - q));
    }
    if (Real{2.0} <= q && q < Real{3.0}) {
      return _weight * (pow5(Real{3.0} - q));
    }
    return Real{0.0};
  }

  constexpr Real _unit_deriv(Real q) const noexcept override {
    if (Real{0.0} <= q && q < Real{1.0}) {
      return _weight * (Real{-5.0} * pow4(Real{3.0} - q) +
                        Real{30.0} * pow4(Real{2.0} - q) -
                        Real{75.0} * pow4(Real{1.0} - q));
    }
    if (Real{1.0} <= q && q < Real{2.0}) {
      return _weight * (Real{-5.0} * pow4(Real{3.0} - q) +
                        Real{30.0} * pow4(Real{2.0} - q));
    }
    if (Real{2.0} <= q && q < Real{3.0}) {
      return _weight * (Real{-5.0} * pow4(Real{3.0} - q));
    }
    return Real{0.0};
  }

}; // class QuinticSmoothingKernel

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*\
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
\*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit::sph
