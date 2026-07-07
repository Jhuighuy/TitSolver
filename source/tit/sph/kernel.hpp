/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <functional>
#include <inplace_vector>
#include <ranges>

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
  template<class Num, std::size_t Dim, class Self>
  constexpr auto flux(this Self& self,
                      const geom::Surface<Vec<Num, Dim>>::Face& face,
                      const Vec<Num, Dim>& x,
                      Num h) noexcept -> Vec<Num, Dim> {
    TIT_ASSERT(h > Num{0.0}, "Kernel width must be positive!");
    const auto h_inverse = inverse(h);
    const auto w = self.template weight<Num, Dim>() * h_inverse;
    const auto n = face.normal();
    const auto d = dot(x - face.a(), n) * h_inverse;
    const auto [... ps] = face.project(x);
    return n * w * self.unit_flux(abs(d), (ps * h_inverse)...);
  }

  /// Antigradient flux over a face.
  template<class Num, std::size_t Dim, class Self>
  constexpr auto antigrad_flux(this Self& self,
                               const geom::Surface<Vec<Num, Dim>>::Face& face,
                               const Vec<Num, Dim>& x,
                               Num h) noexcept -> Num {
    TIT_ASSERT(h > Num{0.0}, "Kernel width must be positive!");
    const auto h_inverse = inverse(h);
    const auto w = self.template weight<Num, Dim>();
    const auto d = dot(x - face.a(), face.normal()) * h_inverse;
    const auto [... ps] = face.project(x);
    return copysign(w, d) *
           self.unit_antigrad_flux(abs(d), (ps * h_inverse)...);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

protected:

  /// Integrate a scalar primitive over a clipped 2D segment support piece.
  template<class Num>
  static constexpr auto unit_segment_integral(Num cutoff,
                                              Num eta,
                                              Num z_min,
                                              Num z_max,
                                              auto&& primitive) noexcept
      -> Num {
    TIT_ASSERT(cutoff > Num{0.0}, "Cutoff must be positive!");
    TIT_ASSERT(eta >= Num{0.0}, "Eta must be non-negative!");
    if (eta >= cutoff) return {};

    // Clip the segment endpoints.
    const auto z_clip = sqrt(pow2(cutoff) - pow2(eta));
    const auto z_lo = std::max(z_min, -z_clip);
    const auto z_hi = std::min(z_max, +z_clip);
    if (z_lo >= z_hi) return {};

    // Evaluate the primitive.
    const auto eta_sqr = pow2(eta);
    const auto eval = [&eta, &eta_sqr, &primitive](Num z) {
      const auto rho = sqrt(pow2(z) + eta_sqr);
      const auto A = atan2(z, eta);
      const auto L = is_tiny(eta) ? Num{0.0} : asinh(z / eta);
      return std::invoke(primitive, eta, z, rho, A, L);
    };

    // Evaluate the integral.
    return eval(z_hi) - eval(z_lo);
  }

  /// Integrate a scalar radial primitive over a clipped 3D triangle support
  /// piece.
  template<class Num>
  static constexpr auto unit_triangle_integral(Num cutoff,
                                               Num eta,
                                               const Vec<Num, 2>& a,
                                               const Vec<Num, 2>& b,
                                               const Vec<Num, 2>& c,
                                               auto&& line_primitive,
                                               auto&& sector_primitive) noexcept
      -> Num {
    TIT_ASSERT(cutoff > Num{0.0}, "Cutoff must be positive!");
    TIT_ASSERT(eta >= Num{0.0}, "Eta must be non-negative!");
    if (eta >= cutoff) return {};
    const auto radius_sqr = pow2(cutoff) - pow2(eta);

    // Evaluate the sector integral.
    const auto sector_integral = std::invoke(sector_primitive, eta);

    // Evaluate the edge integral.
    const auto edge_integral =
        [&eta, &line_primitive, &radius_sqr, &sector_integral](
            const Vec<Num, 2>& p0,
            const Vec<Num, 2>& p1) noexcept -> Num {
      const auto edge = p1 - p0;
      const auto edge_len_sqr = norm2(edge);
      if (edge_len_sqr <= pow2(tiny_v<Num>)) return {};
      const auto edge_len = sqrt(edge_len_sqr);
      const auto tangent = edge / edge_len;

      // Evaluate the line primitive.
      const auto delta = det(p0, tangent);
      const auto delta_sqr = pow2(delta);
      const auto beta_sqr = pow2(eta) + delta_sqr;
      const auto beta = sqrt(beta_sqr);
      const auto eval_line = [&eta, //
                              &delta,
                              &beta,
                              &beta_sqr,
                              &line_primitive](Num z) {
        const auto rho = sqrt(pow2(z) + beta_sqr);
        const auto delta_abs = abs(delta);
        const auto delta_sign = copysign(Num{1.0}, delta);
        const auto A = atan2(z * delta_sign, delta_abs);
        const auto B =
            is_tiny(delta) ? A : atan2(eta * z * delta_sign, rho * delta_abs);
        const auto L = is_tiny(beta) ? Num{0.0} : asinh(z / beta);
        return std::invoke(line_primitive, eta, delta, z, rho, A, B, L);
      };

      // Split the edge by support-circle intersections in edge coordinates.
      const auto z_start = dot(p0, tangent);
      const auto z_finish = z_start + edge_len;
      std::inplace_vector<Num, 4> zs{z_start};
      if (radius_sqr > delta_sqr) {
        const auto z_clip = sqrt(radius_sqr - delta_sqr);
        if (z_start < -z_clip && -z_clip < z_finish) zs.push_back(-z_clip);
        if (z_start < +z_clip && +z_clip < z_finish) zs.push_back(+z_clip);
      }
      zs.push_back(z_finish);

      // Sum integrals over the edge segments.
      Num result{};
      for (const auto& [z_lo, z_hi] : std::views::pairwise(zs)) {
        if (is_tiny(z_hi - z_lo)) continue;
        if (pow2(avg(z_lo, z_hi)) + delta_sqr < radius_sqr) {
          result += eval_line(z_hi) - eval_line(z_lo);
        } else {
          const auto arc_angle =
              atan2(delta * (z_hi - z_lo), z_lo * z_hi + delta_sqr);
          result += sector_integral * arc_angle;
        }
      }
      return result;
    };

    // Evaluate the integral.
    return edge_integral(a, b) + edge_integral(b, c) + edge_integral(c, a);
  }

}; // class Kernel

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Generated kernel.
//

/// Smoothing kernel whose implementation is generated.
template<class Tag>
class KernelGen final : public Kernel {
public:

  /// Unit support radius.
  template<class Num>
  static consteval auto unit_radius() noexcept -> Num;

  /// Value of the unit smoothing kernel at a point.
  template<class Num>
  static constexpr auto unit_value(Num q) noexcept -> Num;

  /// Derivative of the unit smoothing kernel at a point.
  template<class Num>
  static constexpr auto unit_deriv(Num q) noexcept -> Num;

  /// Tail moment of the unit smoothing kernel.
  template<std::size_t Dim, class Num>
  static constexpr auto unit_antideriv_moment(Num q) noexcept -> Num;

  /// Scalar kernel flux over a segment.
  template<class Num>
  static constexpr auto unit_flux(Num eta, Num z_min, Num z_max) noexcept
      -> Num;

  /// Scalar kernel flux over a triangle.
  template<class Num>
  static constexpr auto unit_flux(Num eta,
                                  const Vec<Num, 2>& a,
                                  const Vec<Num, 2>& b,
                                  const Vec<Num, 2>& c) noexcept -> Num;

  /// Scalar antigradient flux over a segment.
  template<class Num>
  static constexpr auto unit_antigrad_flux(Num eta,
                                           Num z_min,
                                           Num z_max) noexcept -> Num;

  /// Scalar antigradient flux over a triangle.
  template<class Num>
  static constexpr auto unit_antigrad_flux(Num eta,
                                           const Vec<Num, 2>& a,
                                           const Vec<Num, 2>& b,
                                           const Vec<Num, 2>& c) noexcept
      -> Num;

}; // class KernelGen

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Spline kernels.
//

namespace impl {
struct CubicSplineTag {};
struct QuarticSplineTag {};
struct QuinticSplineTag {};
} // namespace impl

/// Cubic B-spline (M4) smoothing kernel.
using CubicSplineKernel = KernelGen<impl::CubicSplineTag>;

/// The quartic B-spline (M5) smoothing kernel.
using QuarticSplineKernel = KernelGen<impl::QuarticSplineTag>;

/// Quintic B-spline (M6) smoothing kernel.
using QuinticSplineKernel = KernelGen<impl::QuinticSplineTag>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Wendland kernels.
//

namespace impl {
struct QuarticWendlandTag {};
struct SixthOrderWendlandTag {};
struct EighthOrderWendlandTag {};
} // namespace impl

/// Wendland's quartic (C2) smoothing kernel (Wendland, 1995).
using QuarticWendlandKernel = KernelGen<impl::QuarticWendlandTag>;

/// Wendland's 6-th order (C4) smoothing kernel (Wendland, 1995).
using SixthOrderWendlandKernel = KernelGen<impl::SixthOrderWendlandTag>;

/// Wendland's 8-th order (C6) smoothing kernel (Wendland, 1995).
using EighthOrderWendlandKernel = KernelGen<impl::EighthOrderWendlandTag>;

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

// NOLINTNEXTLINE(misc-header-include-cycle)
#include "tit/sph/kernel.inl.hpp" // IWYU pragma: keep
