/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <numbers>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/math.hpp"
#include "tit/core/type.hpp"
#include "tit/core/vec.hpp"
#include "tit/sph/field.hpp"
#include "tit/sph/particle_array.hpp"

namespace tit::sph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Kernel class.
//

/// Abstract smoothing kernel.
template<class Num>
class Kernel {
public:

  /// Set of particle fields that are required.
  static constexpr TypeSet fields{r};

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Construct a kernel.
  constexpr explicit Kernel(Num h) noexcept : h_{h} {
    TIT_ASSERT(h_ > Num{0.0}, "Kernel width must be positive!");
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Support radius.
  /// @{
  constexpr auto radius(this const auto& self) noexcept -> Num {
    return self.radius(self.h_);
  }
  constexpr auto radius(this const auto& self, Num h) noexcept -> Num {
    return self.unit_radius() * h;
  }
  /// @}

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Value of the smoothing kernel for two particles.
  /// @{
  template<particle_view_n<Num, r> PV>
  constexpr auto operator()(this const auto& self, PV a, PV b) noexcept {
    return self(a, b, self.h_);
  }
  template<particle_view_n<Num, r> PV>
  constexpr auto operator()(this const auto& self, PV a, PV b, Num h) noexcept {
    return self(r[a, b], h);
  }
  /// @}

  /// Value of the smoothing kernel at point.
  /// @{
  template<size_t Dim>
  constexpr auto operator()(this const auto& self,
                            const Vec<Num, Dim>& x) noexcept {
    return self(x, self.h_);
  }
  template<size_t Dim>
  constexpr auto operator()(this const auto& self,
                            const Vec<Num, Dim>& x,
                            Num h) -> Num {
    const auto h_inverse = inverse(h);
    const auto w = self.template weight<Dim>() * pow(h_inverse, Dim);
    const auto q = h_inverse * norm(x);
    return w * self.unit_value(q);
  }
  /// @}

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Gradient of the smoothing kernel for two particles.
  /// @{
  template<particle_view_n<Num, fields> PV>
  constexpr auto grad(this const auto& self, PV a, PV b) noexcept {
    return self.grad(a, b, self.h_);
  }
  template<particle_view_n<Num, fields> PV>
  constexpr auto grad(this const auto& self, PV a, PV b, Num h) noexcept {
    return self.grad(r[a, b], h);
  }
  /// @}

  /// Gradient of the smoothing kernel at point.
  /// @{
  template<size_t Dim>
  constexpr auto grad(this const auto& self, const Vec<Num, Dim>& x) noexcept {
    return self.grad(x, self.h_);
  }
  template<size_t Dim>
  constexpr auto grad(this const auto& self,
                      const Vec<Num, Dim>& x,
                      Num h) noexcept -> Vec<Num, Dim> {
    const auto h_inverse = inverse(h);
    const auto w = self.template weight<Dim>() * pow(h_inverse, Dim);
    const auto q = h_inverse * norm(x);
    const auto grad_q = normalize(x) * h_inverse;
    return w * self.unit_deriv(q) * grad_q;
  }
  /// @}

private:

  Num h_;

}; // class Kernel

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Gaussian kernel.
//

/// Gaussian smoothing kernel (Monaghan, 1992).
template<class Num>
class GaussianKernel final : public Kernel<Num> {
public:

  using Kernel<Num>::Kernel;

  /// Kernel weight.
  template<size_t Dim>
  static consteval auto weight() noexcept -> Num {
    static_assert(1 <= Dim);
    return Num{pow(std::numbers::inv_sqrtpi, Dim)};
  }

  /// Unit support radius.
  static consteval auto unit_radius() noexcept -> Num {
    return -log(tiny_v<Num>);
  }

  /// Value of the unit smoothing kernel at a point.
  static constexpr auto unit_value(Num q) noexcept -> Num {
    return exp(-pow2(q));
  }

  /// Derivative of the unit smoothing kernel at a point.
  static constexpr auto unit_deriv(Num q) noexcept -> Num {
    return -Num{2.0} * q * exp(-pow2(q));
  }

}; // class GaussianKernel

template<class Num>
GaussianKernel(Num) -> GaussianKernel<Num>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Spline kernels.
//

/// Cubic B-spline (M4) smoothing kernel.
template<class Num>
class CubicSplineKernel final : public Kernel<Num> {
public:

  using Kernel<Num>::Kernel;

  /// Kernel weight.
  template<size_t Dim>
  static consteval auto weight() noexcept -> Num {
    if constexpr (Dim == 1) return Num{2.0 / 3.0};
    else if constexpr (Dim == 2) return Num{10.0 / 7.0 * std::numbers::inv_pi};
    else if constexpr (Dim == 3) return Num{std::numbers::inv_pi};
    else static_assert(false);
  }

  /// Unit support radius.
  static consteval auto unit_radius() noexcept -> Num {
    return Num{2.0};
  }

  /// Value of the unit smoothing kernel at a point.
  static constexpr auto unit_value(Num q) noexcept -> Num {
    constexpr Vec qi{Num{2.0}, Num{1.0}};
    constexpr Vec wi{Num{0.25}, Num{-1.0}};
    const Vec<Num, 2> qv(q);
    return sum(filter(qv < qi, wi * pow<3>(qi - qv)));
  }

  /// Derivative of the unit smoothing kernel at a point.
  static constexpr auto unit_deriv(Num q) noexcept -> Num {
    constexpr Vec qi{Num{2.0}, Num{1.0}};
    constexpr Vec wi{Num{-0.75}, Num{3.0}};
    const Vec<Num, 2> qv(q);
    return sum(filter(qv < qi, wi * pow2(qi - qv)));
  }

}; // class CubicSplineKernel

template<class Num>
CubicSplineKernel(Num) -> CubicSplineKernel<Num>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// The quartic B-spline (M5) smoothing kernel.
template<class Num>
class QuarticSplineKernel final : public Kernel<Num> {
public:

  using Kernel<Num>::Kernel;

  /// Kernel weight.
  template<size_t Dim>
  static consteval auto weight() noexcept -> Num {
    using std::numbers::inv_pi;
    if constexpr (Dim == 1) return Num{1.0 / 24.0};
    else if constexpr (Dim == 2) return Num{96.0 / 1199.0 * inv_pi};
    else if constexpr (Dim == 3) return Num{1.0 / 20.0 * inv_pi};
    else static_assert(false);
  }

  /// Unit support radius.
  static consteval auto unit_radius() noexcept -> Num {
    return Num{2.5};
  }

  /// Value of the unit smoothing kernel at a point.
  static constexpr auto unit_value(Num q) noexcept -> Num {
    constexpr Vec qi{Num{2.5}, Num{1.5}, Num{0.5}};
    constexpr Vec wi{Num{1.0}, Num{-5.0}, Num{10.0}};
    const Vec<Num, 3> qv(q);
    return sum(filter(qv < qi, wi * pow<4>(qi - qv)));
  }

  /// Derivative value of the unit smoothing kernel at a point.
  static constexpr auto unit_deriv(Num q) noexcept -> Num {
    constexpr Vec qi{Num{2.5}, Num{1.5}, Num{0.5}};
    constexpr Vec wi{Num{-4.0}, Num{20.0}, Num{-40.0}};
    const Vec<Num, 3> qv(q);
    return sum(filter(qv < qi, wi * pow<3>(qi - qv)));
  }

}; // class QuarticSplineKernel

template<class Num>
QuarticSplineKernel(Num) -> QuarticSplineKernel<Num>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Quintic B-spline (M6) smoothing kernel.
template<class Num>
class QuinticSplineKernel final : public Kernel<Num> {
public:

  using Kernel<Num>::Kernel;

  /// Kernel weight.
  template<size_t Dim>
  static consteval auto weight() noexcept {
    using std::numbers::inv_pi;
    if constexpr (Dim == 1) return Num{1.0 / 120.0};
    else if constexpr (Dim == 2) return Num{7.0 / 478.0 * inv_pi};
    else if constexpr (Dim == 3) return Num{1.0 / 120.0 * inv_pi};
    else static_assert(false);
  }

  /// Unit support radius.
  static consteval auto unit_radius() noexcept -> Num {
    return Num{3.0};
  }

  /// Value of the unit smoothing kernel at a point.
  static constexpr auto unit_value(Num q) noexcept -> Num {
    constexpr Vec qi{Num{3.0}, Num{2.0}, Num{1.0}};
    constexpr Vec wi{Num{1.0}, Num{-6.0}, Num{15.0}};
    const Vec<Num, 3> qv(q);
    return sum(filter(qv < qi, wi * pow<5>(qi - qv)));
  }

  /// Derivative of the unit smoothing kernel at a point.
  static constexpr auto unit_deriv(Num q) noexcept -> Num {
    constexpr Vec qi{Num{3.0}, Num{2.0}, Num{1.0}};
    constexpr Vec wi{Num{-5.0}, Num{30.0}, Num{-75.0}};
    const Vec<Num, 3> qv(q);
    return sum(filter(qv < qi, wi * pow<4>(qi - qv)));
  }

}; // class QuinticSplineKernel

template<class Num>
QuinticSplineKernel(Num) -> QuinticSplineKernel<Num>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Wendland kernels.
//

/// Abstract Wendland's smoothing kernel.
template<class Num>
class WendlandKernel : public Kernel<Num> {
public:

  using Kernel<Num>::Kernel;

  /// Unit support radius.
  static consteval auto unit_radius() noexcept -> Num {
    // Wendland's kernels always have a support radius of 2.
    return Num{2.0};
  }

  /// Value of the unit smoothing kernel at a point.
  constexpr auto unit_value(this auto& self, Num q) noexcept -> Num {
    return q < Num{2.0} ? self.unit_value_notrunc(q) : Num{0.0};
  }

  /// Derivative of the unit smoothing kernel at a point.
  constexpr auto unit_deriv(this auto& self, Num q) noexcept -> Num {
    return q < Num{2.0} ? self.unit_deriv_notrunc(q) : Num{0.0};
  }

}; // class WendlandKernel

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Wendland's quartic (C2) smoothing kernel (Wendland, 1995).
template<class Num>
class QuarticWendlandKernel final : public WendlandKernel<Num> {
public:

  using WendlandKernel<Num>::WendlandKernel;

  /// Kernel weight.
  template<size_t Dim>
  static consteval auto weight() noexcept -> Num {
    using std::numbers::inv_pi;
    if constexpr (Dim == 1) return Num{3.0 / 4.0};
    else if constexpr (Dim == 2) return Num{7.0 / 4.0 * inv_pi};
    else if constexpr (Dim == 3) return Num{21.0 / 16.0 * inv_pi};
    else static_assert(false);
  }

  /// Value of the unit smoothing kernel at a point (no truncation).
  static constexpr auto unit_value_notrunc(Num q) noexcept -> Num {
    return (Num{1.0} + Num{2.0} * q) * pow<4>(Num{1.0} - Num{0.5} * q);
  }

  /// Derivative of the unit smoothing kernel at a point (no truncation).
  static constexpr auto unit_deriv_notrunc(Num q) noexcept -> Num {
    // Well known formula is dW/dq = -5 * q * (1 - q/2)^3, but it requires 5
    // multiplications. Formula that is used requires 4.
    return Num{5.0 / 8.0} * q * pow<3>(q - Num{2.0});
  }

}; // class QuarticWendlandKernel

template<class Num>
QuarticWendlandKernel(Num) -> QuarticWendlandKernel<Num>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Wendland's 6-th order (C4) smoothing kernel (Wendland, 1995).
template<class Num>
class SixthOrderWendlandKernel final : public WendlandKernel<Num> {
public:

  using WendlandKernel<Num>::WendlandKernel;

  /// Kernel weight.
  template<size_t Dim>
  static consteval auto weight() noexcept -> Num {
    using std::numbers::inv_pi;
    if constexpr (Dim == 1) return Num{27.0 / 32.0};
    else if constexpr (Dim == 2) return Num{9.0 / 4.0 * inv_pi};
    else if constexpr (Dim == 3) return Num{495.0 / 256.0 * inv_pi};
    else static_assert(false);
  }

  /// Value of the unit smoothing kernel at a point (no truncation).
  static constexpr auto unit_value_notrunc(Num q) noexcept -> Num {
    return horner(q, {Num{1.0}, Num{3.0}, Num{35.0 / 12.0}}) *
           pow<6>(Num{1.0} - Num{0.5} * q);
  }

  /// Derivative of the unit smoothing kernel at a point (no truncation).
  static constexpr auto unit_deriv_notrunc(Num q) noexcept -> Num {
    return Num{7.0 / 96.0} * q * horner(q, {Num{2.0}, Num{5.0}}) *
           pow<5>(q - Num{2.0});
  }

}; // class SixthOrderWendlandKernel

template<class Num>
SixthOrderWendlandKernel(Num) -> SixthOrderWendlandKernel<Num>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Wendland's 8-th order (C6) smoothing kernel (Wendland, 1995).
template<class Num>
class EighthOrderWendlandKernel final : public WendlandKernel<Num> {
public:

  using WendlandKernel<Num>::WendlandKernel;

  /// Kernel weight.
  template<size_t Dim>
  static consteval auto weight() noexcept -> Num {
    using std::numbers::inv_pi;
    if constexpr (Dim == 1) return Num{15.0 / 16.0};
    else if constexpr (Dim == 2) return Num{39.0 / 14.0 * inv_pi};
    else if constexpr (Dim == 3) return Num{1365.0 / 512.0 * inv_pi};
    else static_assert(false);
  }

  /// Value of the unit smoothing kernel at a point (no truncation).
  static constexpr auto unit_value_notrunc(Num q) noexcept -> Num {
    return horner(q, {Num{1.0}, Num{4.0}, Num{25.0 / 4.0}, Num{4.0}}) *
           pow<8>(Num{1.0} - Num{0.5} * q);
  }

  /// Derivative of the unit smoothing kernel at a point (no truncation).
  static constexpr auto unit_deriv_notrunc(Num q) noexcept -> Num {
    return Num{11.0 / 512.0} * q * horner(q, {Num{2.0}, Num{7.0}, Num{8.0}}) *
           pow<7>(q - Num{2.0});
  }

}; // class EighthOrderWendlandKernel

template<class Num>
EighthOrderWendlandKernel(Num) -> EighthOrderWendlandKernel<Num>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Smoothing kernel type.
template<class K, class Num>
concept kernel = std::same_as<K, GaussianKernel<Num>> ||
                 std::same_as<K, CubicSplineKernel<Num>> ||
                 std::same_as<K, QuarticSplineKernel<Num>> ||
                 std::same_as<K, QuinticSplineKernel<Num>> ||
                 std::same_as<K, QuarticWendlandKernel<Num>> ||
                 std::same_as<K, SixthOrderWendlandKernel<Num>> ||
                 std::same_as<K, EighthOrderWendlandKernel<Num>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
