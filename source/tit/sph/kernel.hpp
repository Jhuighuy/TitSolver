/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <cstddef>

#include "tit/core/assert.hpp"
#include "tit/core/math.hpp"
#include "tit/core/type.hpp"
#include "tit/core/vec.hpp"
#include "tit/geom/surface.hpp"
#include "tit/sph/field.hpp"
#include "tit/sph/particle_array.hpp"

namespace tit::sph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Kernel class.
//

/// Abstract smoothing kernel.
class Kernel {
public:

  /// Set of particle fields that are required.
  static constexpr TypeSet required_fields{r, h};

  /// Set of particle fields that are modified.
  static constexpr TypeSet modified_fields{/*empty*/};

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Support radius for particle.
  template<particle_view<required_fields> PV>
  constexpr auto radius(this auto& self, PV a) noexcept {
    return self.radius(h[a]);
  }

  /// Value of the smoothing kernel for two particles.
  /// @{
  template<particle_view<required_fields> PV>
  constexpr auto operator()(this auto& self, PV a, PV b) noexcept {
    static_assert(has_uniform<PV>(h));
    return self(r[a, b], h[a]);
  }
  template<particle_view<required_fields> PV>
  constexpr auto operator()(this auto& self, PV a, PV b, auto h_ab) noexcept {
    static_assert(!has_uniform<PV>(h));
    return self(r[a, b], h_ab);
  }
  /// @}

  /// Gradient of the smoothing kernel for two particles.
  /// @{
  template<particle_view<required_fields> PV>
  constexpr auto grad(this auto& self, PV a, PV b) noexcept {
    static_assert(has_uniform<PV>(h));
    return self.grad(r[a, b], h[a]);
  }
  template<particle_view<required_fields> PV>
  constexpr auto grad(this auto& self, PV a, PV b, auto h_ab) noexcept {
    static_assert(!has_uniform<PV>(h));
    return self.grad(r[a, b], h_ab);
  }
  /// @}

  /// Width derivative of the smoothing kernel for two points.
  /// @{
  template<particle_view<required_fields> PV>
  constexpr auto width_deriv(this auto& self, PV a, PV b) noexcept {
    static_assert(has_uniform<PV>(h));
    return self.width_deriv(r[a, b], h[a]);
  }
  template<particle_view<required_fields> PV>
  constexpr auto width_deriv(this auto& self, PV a, PV b, auto h_ab) noexcept {
    static_assert(!has_uniform<PV>(h));
    return self.width_deriv(r[a, b], h_ab);
  }
  /// @}

  /// Antigradient of the smoothing kernel for two particles.
  /// @{
  template<particle_view<required_fields> PV>
  constexpr auto antigrad(this auto& self, PV a, PV b) noexcept {
    static_assert(has_uniform<PV>(h));
    return self.antigrad(r[a, b], h[a]);
  }
  template<particle_view<required_fields> PV>
  constexpr auto antigrad(this auto& self, PV a, PV b, auto h_ab) noexcept {
    static_assert(!has_uniform<PV>(h));
    return self.antigrad(r[a, b], h_ab);
  }
  /// @}

  /// Kernel flux over a face.
  template<particle_view<required_fields> PV>
  constexpr auto flux(this auto& self,
                      const geom::Surface<particle_vec_t<PV>>::Face& face,
                      PV a) noexcept {
    return self.flux(face, r[a], h[a]);
  }

  /// Antigradient flux over a face.
  template<particle_view<required_fields> PV>
  constexpr auto antigrad_flux(
      this auto& self,
      const geom::Surface<particle_vec_t<PV>>::Face& face,
      PV a) noexcept {
    static_assert(has_uniform<PV>(h));
    return self.antigrad_flux(face, r[a], h[a]);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Kernel weight.
  template<class Num, std::size_t Dim, class Self>
  constexpr auto weight(this Self& /*self*/) noexcept -> Num {
    constexpr auto zero_moment =
        Self::template unit_antideriv_moment<Dim>(Num{});
    return inverse(zero_moment * unit_sphere_area_v<Dim, Num>);
  }

  /// Support radius.
  template<class Num, class Self>
  constexpr auto radius(this Self& /*self*/, Num h) noexcept -> Num {
    TIT_ASSERT(h > Num{0.0}, "Kernel width must be positive!");
    constexpr auto unit_radius = Self::template unit_radius<Num>();
    return unit_radius * h;
  }

  /// Value of the smoothing kernel at point.
  template<class Num, std::size_t Dim, class Self>
  constexpr auto operator()(this Self& self,
                            const Vec<Num, Dim>& x,
                            Num h) noexcept -> Num {
    TIT_ASSERT(h > Num{0.0}, "Kernel width must be positive!");
    const auto h_inverse = inverse(h);
    const auto w = self.template weight<Num, Dim>() * pow<Dim>(h_inverse);
    const auto q = h_inverse * norm(x);
    return w * self.unit_value(q);
  }

  /// Spatial gradient of the smoothing kernel at point.
  template<class Num, std::size_t Dim, class Self>
  constexpr auto grad(this Self& self, const Vec<Num, Dim>& x, Num h) noexcept
      -> Vec<Num, Dim> {
    TIT_ASSERT(h > Num{0.0}, "Kernel width must be positive!");
    const auto h_inverse = inverse(h);
    const auto w = self.template weight<Num, Dim>() * pow<Dim>(h_inverse);
    const auto q = h_inverse * norm(x);
    const auto grad_q = normalize(x) * h_inverse;
    return w * self.unit_deriv(q) * grad_q;
  }

  /// Width derivative of the smoothing kernel at point.
  template<class Num, std::size_t Dim, class Self>
  constexpr auto width_deriv(this Self& self,
                             const Vec<Num, Dim>& x,
                             Num h) noexcept -> Num {
    TIT_ASSERT(h > Num{0.0}, "Kernel width must be positive!");
    const auto h_inverse = inverse(h);
    const auto w = self.template weight<Num, Dim>() * pow<Dim>(h_inverse);
    const auto dw_dh = -Num{Dim} * w * h_inverse;
    const auto q = h_inverse * norm(x);
    const auto dq_dh = -q * h_inverse;
    return dw_dh * self.unit_value(q) + w * self.unit_deriv(q) * dq_dh;
  }

  /// Spatial antigradient of the smoothing kernel at point.
  /// @note The antigradient contains a $\|x\|^{-d}$ factor.
  template<class Num, std::size_t Dim, class Self>
  constexpr auto antigrad(this Self& self,
                          const Vec<Num, Dim>& x,
                          Num h) noexcept -> Vec<Num, Dim> {
    TIT_ASSERT(h > Num{0.0}, "Kernel width must be positive!");
    const auto h_inverse = inverse(h);
    const auto w = self.template weight<Num, Dim>() * pow<Dim>(h_inverse);
    const auto q = norm(x) * h_inverse;
    TIT_ASSERT(!is_tiny(q), "Kernel antigradient is singular at the origin!");
    return -x * w * self.template unit_antideriv_moment<Dim>(q) / pow<Dim>(q);
  }

  /// Kernel flux over a face.
  /// @todo Below is a very crude approximation. Replace with proper formulas.
  template<class Num, std::size_t Dim, class Self>
  constexpr auto flux(this Self& self,
                      const geom::Surface<Vec<Num, Dim>>::Face& face,
                      const Vec<Num, Dim>& x,
                      Num h) noexcept -> Vec<Num, Dim> {
    TIT_ASSERT(h > Num{0.0}, "Kernel width must be positive!");
    return face.wnormal() * self(x - face.center(), h);
  }

  /// Antigradient flux over a face.
  /// @todo Below is a very crude approximation. Replace with proper formulas.
  template<class Num, std::size_t Dim, class Self>
  constexpr auto antigrad_flux(this Self& self,
                               const geom::Surface<Vec<Num, Dim>>::Face& face,
                               const Vec<Num, Dim>& x,
                               Num& h) noexcept -> Num {
    return -dot(face.wnormal(), self.antigrad(x - face.center(), h));
  }

}; // class Kernel

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Spline kernels.
//

/// Cubic B-spline (M4) smoothing kernel.
class CubicSplineKernel final : public Kernel {
public:

  /// Unit support radius.
  template<class Num>
  static consteval auto unit_radius() noexcept -> Num {
    return Num{2.0};
  }

  /// Value of the unit smoothing kernel at a point.
  template<class Num>
  static constexpr auto unit_value(Num q) noexcept -> Num {
    constexpr Vec qi{Num{2.0}, Num{1.0}};
    constexpr Vec wi{Num{0.25}, Num{-1.0}};
    const Vec<Num, 2> qv(q);
    return sum(filter(qv < qi, wi * pow<3>(qi - qv)));
  }

  /// Derivative of the unit smoothing kernel at a point.
  template<class Num>
  static constexpr auto unit_deriv(Num q) noexcept -> Num {
    constexpr Vec qi{Num{2.0}, Num{1.0}};
    constexpr Vec wi{Num{-0.75}, Num{3.0}};
    const Vec<Num, 2> qv(q);
    return sum(filter(qv < qi, wi * pow2(qi - qv)));
  }

  /// Tail moment of the unit smoothing kernel.
  template<std::size_t Dim, class Num>
  static constexpr auto unit_antideriv_moment(Num q) noexcept -> Num {
    constexpr Vec qi{Num{2.0}, Num{1.0}};
    constexpr Vec wi{Num{0.25}, Num{-1.0}};
    const Vec<Num, 2> qv(q);
    const auto y = qi - qv;
    const auto t = wi * pow<4>(y);
    if constexpr (Dim == 1) {
      return sum(filter(qv < qi, t / Num{4}));
    } else if constexpr (Dim == 2) {
      return sum(filter(qv < qi, t * (qi / Num{4} - y / Num{5})));
    } else if constexpr (Dim == 3) {
      return sum(filter(
          qv < qi,
          t * (pow2(qi) / Num{4} - qi * y / Num{2.5} + pow2(y) / Num{6})));
    } else {
      static_assert(false);
    }
  }

}; // class CubicSplineKernel

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// The quartic B-spline (M5) smoothing kernel.
class QuarticSplineKernel final : public Kernel {
public:

  /// Unit support radius.
  template<class Num>
  static consteval auto unit_radius() noexcept -> Num {
    return Num{2.5};
  }

  /// Value of the unit smoothing kernel at a point.
  template<class Num>
  static constexpr auto unit_value(Num q) noexcept -> Num {
    constexpr Vec qi{Num{2.5}, Num{1.5}, Num{0.5}};
    constexpr Vec wi{Num{1.0}, Num{-5.0}, Num{10.0}};
    const Vec<Num, 3> qv(q);
    return sum(filter(qv < qi, wi * pow<4>(qi - qv)));
  }

  /// Derivative value of the unit smoothing kernel at a point.
  template<class Num>
  static constexpr auto unit_deriv(Num q) noexcept -> Num {
    constexpr Vec qi{Num{2.5}, Num{1.5}, Num{0.5}};
    constexpr Vec wi{Num{-4.0}, Num{20.0}, Num{-40.0}};
    const Vec<Num, 3> qv(q);
    return sum(filter(qv < qi, wi * pow<3>(qi - qv)));
  }

  /// Tail moment of the unit smoothing kernel.
  template<std::size_t Dim, class Num>
  static constexpr auto unit_antideriv_moment(Num q) noexcept -> Num {
    constexpr Vec qi{Num{2.5}, Num{1.5}, Num{0.5}};
    constexpr Vec wi{Num{1.0}, Num{-5.0}, Num{10.0}};
    const Vec<Num, 3> qv(q);
    const auto y = qi - qv;
    const auto t = wi * pow<5>(y);
    if constexpr (Dim == 1) {
      return sum(filter(qv < qi, t / Num{5}));
    } else if constexpr (Dim == 2) {
      return sum(filter(qv < qi, t * (qi / Num{5} - y / Num{6})));
    } else if constexpr (Dim == 3) {
      return sum(filter(
          qv < qi,
          t * (pow2(qi) / Num{5} - qi * y / Num{3.0} + pow2(y) / Num{7})));
    } else {
      static_assert(false);
    }
  }

}; // class QuarticSplineKernel

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Quintic B-spline (M6) smoothing kernel.
class QuinticSplineKernel final : public Kernel {
public:

  /// Unit support radius.
  template<class Num>
  static consteval auto unit_radius() noexcept -> Num {
    return Num{3.0};
  }

  /// Value of the unit smoothing kernel at a point.
  template<class Num>
  static constexpr auto unit_value(Num q) noexcept -> Num {
    constexpr Vec qi{Num{3.0}, Num{2.0}, Num{1.0}};
    constexpr Vec wi{Num{1.0}, Num{-6.0}, Num{15.0}};
    const Vec<Num, 3> qv(q);
    return sum(filter(qv < qi, wi * pow<5>(qi - qv)));
  }

  /// Derivative of the unit smoothing kernel at a point.
  template<class Num>
  static constexpr auto unit_deriv(Num q) noexcept -> Num {
    constexpr Vec qi{Num{3.0}, Num{2.0}, Num{1.0}};
    constexpr Vec wi{Num{-5.0}, Num{30.0}, Num{-75.0}};
    const Vec<Num, 3> qv(q);
    return sum(filter(qv < qi, wi * pow<4>(qi - qv)));
  }

  /// Tail moment of the unit smoothing kernel.
  template<std::size_t Dim, class Num>
  static constexpr auto unit_antideriv_moment(Num q) noexcept -> Num {
    constexpr Vec qi{Num{3.0}, Num{2.0}, Num{1.0}};
    constexpr Vec wi{Num{1.0}, Num{-6.0}, Num{15.0}};
    const Vec<Num, 3> qv(q);
    const auto y = qi - qv;
    const auto t = wi * pow<6>(y);
    if constexpr (Dim == 1) {
      return sum(filter(qv < qi, t / Num{6}));
    } else if constexpr (Dim == 2) {
      return sum(filter(qv < qi, t * (qi / Num{6} - y / Num{7})));
    } else if constexpr (Dim == 3) {
      return sum(filter(
          qv < qi,
          t * (pow2(qi) / Num{6.0} - qi * y / Num{3.5} + pow2(y) / Num{8.0})));
    } else {
      static_assert(false);
    }
  }

}; // class QuinticSplineKernel

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Wendland kernels.
//

/// Wendland's quartic (C2) smoothing kernel (Wendland, 1995).
class QuarticWendlandKernel final : public Kernel {
public:

  /// Unit support radius.
  template<class Num>
  static consteval auto unit_radius() noexcept -> Num {
    return Num{2.0};
  }

  /// Value of the unit smoothing kernel at a point.
  template<class Num>
  static constexpr auto unit_value(Num q) noexcept -> Num {
    if (q >= unit_radius<Num>()) return {};
    return (Num{1.0} + Num{2.0} * q) * pow<4>(Num{1.0} - Num{0.5} * q);
  }

  /// Derivative of the unit smoothing kernel at a point.
  template<class Num>
  static constexpr auto unit_deriv(Num q) noexcept -> Num {
    // Well known formula is dW/dq = -5 * q * (1 - q/2)^3, but it requires 5
    // multiplications. Formula that is used requires 4.
    if (q >= unit_radius<Num>()) return {};
    return Num{5.0 / 8.0} * q * pow<3>(q - Num{2.0});
  }

  /// Tail moment of the unit smoothing kernel.
  template<std::size_t Dim, class Num>
  static constexpr auto unit_antideriv_moment(Num q) noexcept -> Num {
    if (q >= unit_radius<Num>()) return {};
    if constexpr (Dim == 1) {
      return Num{2.0 / 3.0} * (q + Num{1.0}) * pow<5>(Num{1.0} - Num{0.5} * q);
    } else if constexpr (Dim == 2) {
      return Num{2.0 / 7.0} * horner<1.0, 2.5, 2.0>(q) *
             pow<5>(Num{1.0} - Num{0.5} * q);
    } else if constexpr (Dim == 3) {
      return Num{4.0 / 21.0} *
             horner<1.0, 5.0 / 2.0, 15.0 / 4.0, 21.0 / 8.0>(q) *
             pow<5>(Num{1.0} - Num{0.5} * q);
    } else {
      static_assert(false);
    }
  }

}; // class QuarticWendlandKernel

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Wendland's 6-th order (C4) smoothing kernel (Wendland, 1995).
class SixthOrderWendlandKernel final : public Kernel {
public:

  /// Unit support radius.
  template<class Num>
  static consteval auto unit_radius() noexcept -> Num {
    return Num{2.0};
  }

  /// Value of the unit smoothing kernel at a point.
  template<class Num>
  static constexpr auto unit_value(Num q) noexcept -> Num {
    if (q >= unit_radius<Num>()) return {};
    return horner<1.0, 3.0, 35.0 / 12.0>(q) * pow<6>(Num{1.0} - Num{0.5} * q);
  }

  /// Derivative of the unit smoothing kernel at a point.
  template<class Num>
  static constexpr auto unit_deriv(Num q) noexcept -> Num {
    if (q >= unit_radius<Num>()) return {};
    return Num{7.0 / 96.0} * q * horner<2.0, 5.0>(q) * pow<5>(q - Num{2.0});
  }

  /// Tail moment of the unit smoothing kernel.
  template<std::size_t Dim, class Num>
  static constexpr auto unit_antideriv_moment(Num q) noexcept -> Num {
    if (q >= unit_radius<Num>()) return {};
    if constexpr (Dim == 1) {
      return Num{16.0 / 27.0} * horner<1.0, 29.0 / 16.0, 35.0 / 32.0>(q) *
             pow<7>(Num{1.0} - Num{0.5} * q);
    } else if constexpr (Dim == 2) {
      return Num{2.0 / 9.0} *
             horner<1.0, 7.0 / 2.0, 19.0 / 4.0, 21.0 / 8.0>(q) *
             pow<7>(Num{1.0} - Num{0.5} * q);
    } else if constexpr (Dim == 3) {
      return Num{64.0 / 495.0} *
             horner<1.0, 7.0 / 2.0, 7.0, 507.0 / 64.0, 525.0 / 128.0>(q) *
             pow<7>(Num{1.0} - Num{0.5} * q);
    } else {
      static_assert(false);
    }
  }

}; // class SixthOrderWendlandKernel

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Wendland's 8-th order (C6) smoothing kernel (Wendland, 1995).
class EighthOrderWendlandKernel final : public Kernel {
public:

  /// Unit support radius.
  template<class Num>
  static consteval auto unit_radius() noexcept -> Num {
    return Num{2.0};
  }

  /// Value of the unit smoothing kernel at a point.
  template<class Num>
  static constexpr auto unit_value(Num q) noexcept -> Num {
    if (q >= unit_radius<Num>()) return {};
    return horner<1.0, 4.0, 25.0 / 4.0, 4.0>(q) *
           pow<8>(Num{1.0} - Num{0.5} * q);
  }

  /// Derivative of the unit smoothing kernel at a point.
  template<class Num>
  static constexpr auto unit_deriv(Num q) noexcept -> Num {
    if (q >= unit_radius<Num>()) return {};
    return Num{11.0 / 512.0} * q * horner<2.0, 7.0, 8.0>(q) *
           pow<7>(q - Num{2.0});
  }

  /// Tail moment of the unit smoothing kernel.
  template<std::size_t Dim, class Num>
  static constexpr auto unit_antideriv_moment(Num q) noexcept -> Num {
    if (q >= unit_radius<Num>()) return {};
    if constexpr (Dim == 1) {
      return Num{8.0 / 15.0} *
             horner<1.0, 21.0 / 8.0, 45.0 / 16.0, 5.0 / 4.0>(q) *
             pow<9>(Num{1.0} - Num{0.5} * q);
    } else if constexpr (Dim == 2) {
      return Num{7.0 / 39.0} *
             horner<1.0, 9.0 / 2.0, 237.0 / 28.0, 453.0 / 56.0, 24.0 / 7.0>(q) *
             pow<9>(Num{1.0} - Num{0.5} * q);
    } else if constexpr (Dim == 3) {
      // clang-format off
      return Num{128.0 / 1365.0} *
             horner<1.0, 9.0 / 2.0, 45.0 / 4.0, 2185.0 / 128.0, 3825.0 / 256.0, 195.0 / 32.0>(q) *
             pow<9>(Num{1.0} - Num{0.5} * q);
      // clang-format on
    } else {
      static_assert(false);
    }
  }

}; // class EighthOrderWendlandKernel

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Smoothing kernel type.
template<class K>
concept kernel = std::same_as<K, CubicSplineKernel> ||
                 std::same_as<K, QuarticSplineKernel> ||
                 std::same_as<K, QuinticSplineKernel> ||
                 std::same_as<K, QuarticWendlandKernel> ||
                 std::same_as<K, SixthOrderWendlandKernel> ||
                 std::same_as<K, EighthOrderWendlandKernel>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
