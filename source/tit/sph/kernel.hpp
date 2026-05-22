/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <utility>

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

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  //
  // Temporary numerical integration for 3D fluxes.
  //

  /// Scalar kernel flux over a triangle.
  template<class Self, class Num>
  constexpr auto unit_flux(this Self& self,
                           Num eta,
                           const Vec<Num, 2>& a,
                           const Vec<Num, 2>& b,
                           const Vec<Num, 2>& c) noexcept -> Num {
    const auto f = [&self](Num q) { return self.unit_value(q); };
    return unit_flux_impl(f, eta, a, b, c);
  }

  /// Scalar antigradient flux over a triangle.
  template<class Self, class Num>
  constexpr auto unit_antigrad_flux(this Self& self,
                                    Num eta,
                                    const Vec<Num, 2>& a,
                                    const Vec<Num, 2>& b,
                                    const Vec<Num, 2>& c) noexcept -> Num {
    const auto f = [&self](Num q) {
      return self.template unit_antideriv_moment<3>(q) / pow<3>(q);
    };
    return eta * unit_flux_impl(f, eta, a, b, c);
  }

private:

  template<class Func, class Num>
  static constexpr auto unit_flux_impl(const Func& func,
                                       Num eta,
                                       const Vec<Num, 2>& a,
                                       const Vec<Num, 2>& b,
                                       const Vec<Num, 2>& c) noexcept -> Num {
    using Node = std::pair<Vec<Num, 3>, Num>;
    static constexpr auto quadrature = std::to_array<Node>({
        // clang-format off
        {{Num{1.0 / 3.0}, Num{1.0 / 3.0}, Num{1.0 / 3.0}}, Num{0.14431560767778716825}},

        {{Num{0.08141482341455368794}, Num{0.45929258829272315603}, Num{0.45929258829272315603}}, Num{0.09509163426728462479}},
        {{Num{0.45929258829272315603}, Num{0.08141482341455368794}, Num{0.45929258829272315603}}, Num{0.09509163426728462479}},
        {{Num{0.45929258829272315603}, Num{0.45929258829272315603}, Num{0.08141482341455368794}}, Num{0.09509163426728462479}},

        {{Num{0.65886138449647958676}, Num{0.17056930775176020662}, Num{0.17056930775176020662}}, Num{0.10321737053471825028}},
        {{Num{0.17056930775176020662}, Num{0.65886138449647958676}, Num{0.17056930775176020662}}, Num{0.10321737053471825028}},
        {{Num{0.17056930775176020662}, Num{0.17056930775176020662}, Num{0.65886138449647958676}}, Num{0.10321737053471825028}},

        {{Num{0.89890554336593788332}, Num{0.05054722831703105834}, Num{0.05054722831703105834}}, Num{0.03245849762319808031}},
        {{Num{0.05054722831703105834}, Num{0.89890554336593788332}, Num{0.05054722831703105834}}, Num{0.03245849762319808031}},
        {{Num{0.05054722831703105834}, Num{0.05054722831703105834}, Num{0.89890554336593788332}}, Num{0.03245849762319808031}},

        {{Num{0.00839477740995760534}, Num{0.26311282963463811342}, Num{0.72849239295540428124}}, Num{0.02723031417443499426}},
        {{Num{0.00839477740995760534}, Num{0.72849239295540428124}, Num{0.26311282963463811342}}, Num{0.02723031417443499426}},
        {{Num{0.26311282963463811342}, Num{0.00839477740995760534}, Num{0.72849239295540428124}}, Num{0.02723031417443499426}},
        {{Num{0.26311282963463811342}, Num{0.72849239295540428124}, Num{0.00839477740995760534}}, Num{0.02723031417443499426}},
        {{Num{0.72849239295540428124}, Num{0.00839477740995760534}, Num{0.26311282963463811342}}, Num{0.02723031417443499426}},
        {{Num{0.72849239295540428124}, Num{0.26311282963463811342}, Num{0.00839477740995760534}}, Num{0.02723031417443499426}},
        // clang-format on
    });

    const auto area = Num{0.5} * abs((b[0] - a[0]) * (c[1] - a[1]) -
                                     (b[1] - a[1]) * (c[0] - a[0]));
    const auto eval = [&a, &b, &c, &eta, &area, &func](const Vec<Num, 2>& u,
                                                       const Vec<Num, 2>& v,
                                                       const Vec<Num, 2>& w) {
      Num result{};
      for (const auto& [lambda, weight] : quadrature) {
        const auto p = lambda[0] * u + lambda[1] * v + lambda[2] * w;
        const auto y = a + p[0] * (b - a) + p[1] * (c - a);
        const auto q = sqrt(pow2(eta) + norm2(y));
        result += area * weight * func(q);
      }
      return result;
    };

    Num result{};
    constexpr std::size_t num_subdivs = 48;
    constexpr auto step = inverse(static_cast<Num>(num_subdivs));
    for (std::size_t i = 0; i < num_subdivs; ++i) {
      for (std::size_t j = 0; j < num_subdivs - i; ++j) {
        const auto u = vec_cast<Num>(Vec{i, j}) * step;
        const auto v = u + Vec{step, Num{0.0}};
        const auto w = u + Vec{Num{0.0}, step};
        result += eval(u, v, w);
        if (i + j + 1 < num_subdivs) {
          result += eval(v, v + Vec{Num{0.0}, step}, w);
        }
      }
    }
    return result / static_cast<Num>(pow2(num_subdivs));
  }

}; // class Kernel

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Spline kernels.
//

/// Cubic B-spline (M4) smoothing kernel.
class CubicSplineKernel final : public Kernel {
public:

  using Kernel::unit_antigrad_flux;
  using Kernel::unit_flux;

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

  /// Scalar kernel flux over a segment.
  template<class Num>
  static constexpr auto unit_flux(Num eta, Num z_min, Num z_max) noexcept
      -> Num {
    // clang-format off
    const auto primitive = [](Num a, Num eta_, Num z, Num rho, Num /*A*/, Num L) {
      // clang-format on
      const auto eta2 = pow2(eta_);
      const auto J0 = z;
      const auto J1 = Num{0.5} * (z * rho + eta2 * L);
      const auto J2 = z * pow<2>(rho) / Num{3.0} + Num{2.0 / 3.0} * eta2 * J0;
      const auto J3 = z * pow<3>(rho) / Num{4.0} + Num{3.0 / 4.0} * eta2 * J1;
      return horner(a, {-J3, Num{3.0} * J2, -Num{3.0} * J1, J0});
    };
    return Num{0.25} *
               unit_segment_integral(Num{2.0},
                                     eta,
                                     z_min,
                                     z_max,
                                     std::bind_front(primitive, Num{2.0})) -
           Num{1.0} *
               unit_segment_integral(Num{1.0},
                                     eta,
                                     z_min,
                                     z_max,
                                     std::bind_front(primitive, Num{1.0}));
  }

  /// Scalar antigradient flux over a segment.
  template<class Num>
  static constexpr auto unit_antigrad_flux(Num eta,
                                           Num z_min,
                                           Num z_max) noexcept -> Num {
    const auto primitive = [](Num a, Num eta_, Num z, Num rho, Num A, Num L) {
      const auto eta2 = pow2(eta_);
      const auto J0 = z;
      const auto J1 = Num{0.5} * (z * rho + eta2 * L);
      const auto J2 = z * pow<2>(rho) / Num{3.0} + Num{2.0 / 3.0} * eta2 * J0;
      const auto J3 = z * pow<3>(rho) / Num{4.0} + Num{3.0 / 4.0} * eta2 * J1;
      return A * pow<5>(a) / Num{20.0} + eta_ * horner(a,
                                                       {J3 / Num{5.0}, //
                                                        -Num{3.0 / 4.0} * J2,
                                                        J1,
                                                        -J0 / Num{2.0}});
    };
    return Num{0.25} *
               unit_segment_integral(Num{2.0},
                                     eta,
                                     z_min,
                                     z_max,
                                     std::bind_front(primitive, Num{2.0})) -
           Num{1.0} *
               unit_segment_integral(Num{1.0},
                                     eta,
                                     z_min,
                                     z_max,
                                     std::bind_front(primitive, Num{1.0}));
  }

}; // class CubicSplineKernel

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// The quartic B-spline (M5) smoothing kernel.
class QuarticSplineKernel final : public Kernel {
public:

  using Kernel::unit_antigrad_flux;
  using Kernel::unit_flux;

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

  /// Scalar kernel flux over a segment.
  template<class Num>
  static constexpr auto unit_flux(Num eta, Num z_min, Num z_max) noexcept
      -> Num {
    // clang-format off
    const auto primitive = [](Num a, Num eta_, Num z, Num rho, Num /*A*/, Num L) {
      // clang-format on
      const auto eta2 = pow2(eta_);
      const auto J0 = z;
      const auto J1 = Num{0.5} * (z * rho + eta2 * L);
      const auto J2 = z * pow<2>(rho) / Num{3.0} + Num{2.0 / 3.0} * eta2 * J0;
      const auto J3 = z * pow<3>(rho) / Num{4.0} + Num{3.0 / 4.0} * eta2 * J1;
      const auto J4 = z * pow<4>(rho) / Num{5.0} + Num{4.0 / 5.0} * eta2 * J2;
      return horner(a, {J4, -Num{4.0} * J3, Num{6.0} * J2, -Num{4.0} * J1, J0});
    };
    return Num{1.0} *
               unit_segment_integral(Num{2.5},
                                     eta,
                                     z_min,
                                     z_max,
                                     std::bind_front(primitive, Num{2.5})) -
           Num{5.0} *
               unit_segment_integral(Num{1.5},
                                     eta,
                                     z_min,
                                     z_max,
                                     std::bind_front(primitive, Num{1.5})) +
           Num{10.0} *
               unit_segment_integral(Num{0.5},
                                     eta,
                                     z_min,
                                     z_max,
                                     std::bind_front(primitive, Num{0.5}));
  }

  /// Scalar antigradient flux over a segment.
  template<class Num>
  static constexpr auto unit_antigrad_flux(Num eta,
                                           Num z_min,
                                           Num z_max) noexcept -> Num {
    const auto primitive = [](Num a, Num eta_, Num z, Num rho, Num A, Num L) {
      const auto eta2 = pow2(eta_);
      const auto J0 = z;
      const auto J1 = Num{0.5} * (z * rho + eta2 * L);
      const auto J2 = z * pow<2>(rho) / Num{3.0} + Num{2.0 / 3.0} * eta2 * J0;
      const auto J3 = z * pow<3>(rho) / Num{4.0} + Num{3.0 / 4.0} * eta2 * J1;
      const auto J4 = z * pow<4>(rho) / Num{5.0} + Num{4.0 / 5.0} * eta2 * J2;
      return A * pow<6>(a) / Num{30.0} + eta_ * horner(a,
                                                       {-J4 / Num{6.0},
                                                        Num{4.0 / 5.0} * J3,
                                                        -Num{3.0 / 2.0} * J2,
                                                        Num{4.0 / 3.0} * J1,
                                                        -J0 / Num{2.0}});
    };
    return Num{1.0} *
               unit_segment_integral(Num{2.5},
                                     eta,
                                     z_min,
                                     z_max,
                                     std::bind_front(primitive, Num{2.5})) -
           Num{5.0} *
               unit_segment_integral(Num{1.5},
                                     eta,
                                     z_min,
                                     z_max,
                                     std::bind_front(primitive, Num{1.5})) +
           Num{10.0} *
               unit_segment_integral(Num{0.5},
                                     eta,
                                     z_min,
                                     z_max,
                                     std::bind_front(primitive, Num{0.5}));
  }

}; // class QuarticSplineKernel

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Quintic B-spline (M6) smoothing kernel.
class QuinticSplineKernel final : public Kernel {
public:

  using Kernel::unit_antigrad_flux;
  using Kernel::unit_flux;

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

  /// Scalar kernel flux over a segment.
  template<class Num>
  static constexpr auto unit_flux(Num eta, Num z_min, Num z_max) noexcept
      -> Num {
    // clang-format off
    const auto primitive = [](Num a, Num eta_, Num z, Num rho, Num /*A*/, Num L) {
      // clang-format on
      const auto eta2 = pow2(eta_);
      const auto J0 = z;
      const auto J1 = Num{0.5} * (z * rho + eta2 * L);
      const auto J2 = z * pow<2>(rho) / Num{3.0} + Num{2.0 / 3.0} * eta2 * J0;
      const auto J3 = z * pow<3>(rho) / Num{4.0} + Num{3.0 / 4.0} * eta2 * J1;
      const auto J4 = z * pow<4>(rho) / Num{5.0} + Num{4.0 / 5.0} * eta2 * J2;
      const auto J5 = z * pow<5>(rho) / Num{6.0} + Num{5.0 / 6.0} * eta2 * J3;
      return horner(a,
                    {-J5,
                     Num{5.0} * J4,
                     -Num{10.0} * J3,
                     Num{10.0} * J2,
                     -Num{5.0} * J1,
                     J0});
    };
    return Num{1.0} *
               unit_segment_integral(Num{3.0},
                                     eta,
                                     z_min,
                                     z_max,
                                     std::bind_front(primitive, Num{3.0})) -
           Num{6.0} *
               unit_segment_integral(Num{2.0},
                                     eta,
                                     z_min,
                                     z_max,
                                     std::bind_front(primitive, Num{2.0})) +
           Num{15.0} *
               unit_segment_integral(Num{1.0},
                                     eta,
                                     z_min,
                                     z_max,
                                     std::bind_front(primitive, Num{1.0}));
  }

  /// Scalar antigradient flux over a segment.
  template<class Num>
  static constexpr auto unit_antigrad_flux(Num eta,
                                           Num z_min,
                                           Num z_max) noexcept -> Num {
    const auto primitive = [](Num a, Num eta_, Num z, Num rho, Num A, Num L) {
      const auto eta2 = pow2(eta_);
      const auto J0 = z;
      const auto J1 = Num{0.5} * (z * rho + eta2 * L);
      const auto J2 = z * pow<2>(rho) / Num{3.0} + Num{2.0 / 3.0} * eta2 * J0;
      const auto J3 = z * pow<3>(rho) / Num{4.0} + Num{3.0 / 4.0} * eta2 * J1;
      const auto J4 = z * pow<4>(rho) / Num{5.0} + Num{4.0 / 5.0} * eta2 * J2;
      const auto J5 = z * pow<5>(rho) / Num{6.0} + Num{5.0 / 6.0} * eta2 * J3;
      return A * pow<7>(a) / Num{42.0} + eta_ * horner(a,
                                                       {J5 / Num{7.0},
                                                        -Num{5.0 / 6.0} * J4,
                                                        Num{2.0} * J3,
                                                        -Num{5.0 / 2.0} * J2,
                                                        Num{5.0 / 3.0} * J1,
                                                        -J0 / Num{2.0}});
    };
    return Num{1.0} *
               unit_segment_integral(Num{3.0},
                                     eta,
                                     z_min,
                                     z_max,
                                     std::bind_front(primitive, Num{3.0})) -
           Num{6.0} *
               unit_segment_integral(Num{2.0},
                                     eta,
                                     z_min,
                                     z_max,
                                     std::bind_front(primitive, Num{2.0})) +
           Num{15.0} *
               unit_segment_integral(Num{1.0},
                                     eta,
                                     z_min,
                                     z_max,
                                     std::bind_front(primitive, Num{1.0}));
  }

}; // class QuinticSplineKernel

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Wendland kernels.
//

/// Wendland's quartic (C2) smoothing kernel (Wendland, 1995).
class QuarticWendlandKernel final : public Kernel {
public:

  using Kernel::unit_antigrad_flux;
  using Kernel::unit_flux;

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

  /// Scalar kernel flux over a segment.
  template<class Num>
  static constexpr auto unit_flux(Num eta, Num z_min, Num z_max) noexcept
      -> Num {
    constexpr auto primitive = [](Num eta_, Num z, Num rho, Num /*A*/, Num L) {
      const auto eta2 = pow2(eta_);
      const std::array rho_coeffs{
          horner<1.0, -5.0 / 3.0, -1.0 / 2.0>(eta2),
          horner<15.0 / 16.0, 5.0 / 128.0>(eta2) * eta2,
          horner<-5.0 / 6.0, -1.0 / 4.0>(eta2),
          horner<5.0 / 8.0, 5.0 / 192.0>(eta2),
          -Num{3.0 / 16.0},
          Num{1.0 / 48.0},
      };
      return L * rho_coeffs[1] * eta2 + z * horner(rho, rho_coeffs);
    };
    return unit_segment_integral(Num{2.0}, eta, z_min, z_max, primitive);
  }

  /// Scalar antigradient flux over a segment.
  template<class Num>
  static constexpr auto unit_antigrad_flux(Num eta,
                                           Num z_min,
                                           Num z_max) noexcept -> Num {
    constexpr auto primitive = [](Num eta_, Num z, Num rho, Num A, Num L) {
      const auto eta2 = pow2(eta_);
      const std::array rho_coeffs{
          horner<-1.0 / 2.0, 5.0 / 12.0, 1.0 / 12.0>(eta2),
          horner<-3.0 / 16.0, -5.0 / 896.0>(eta2) * eta2,
          horner<5.0 / 24.0, 1.0 / 24.0>(eta2),
          horner<-1.0 / 8.0, -5.0 / 1344.0>(eta2),
          Num{1.0 / 32.0},
          -Num{1.0 / 336.0},
      };
      return Num{2.0 / 7.0} * A +
             eta_ * (L * rho_coeffs[1] * eta2 + z * horner(rho, rho_coeffs));
    };
    return unit_segment_integral(Num{2.0}, eta, z_min, z_max, primitive);
  }

}; // class QuarticWendlandKernel

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Wendland's 6-th order (C4) smoothing kernel (Wendland, 1995).
class SixthOrderWendlandKernel final : public Kernel {
public:

  using Kernel::unit_antigrad_flux;
  using Kernel::unit_flux;

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

  /// Scalar kernel flux over a segment.
  template<class Num>
  static constexpr auto unit_flux(Num eta, Num z_min, Num z_max) noexcept
      -> Num {
    constexpr auto primitive = [](Num eta_, Num z, Num rho, Num /*A*/, Num L) {
      const auto eta2 = pow2(eta_);
      const auto eta4 = pow2(eta2);
      const std::array rho_coeffs{
          horner<1.0, -14.0 / 9.0, 7.0 / 3.0, 1.0, 1.0 / 54.0>(eta2),
          horner<-35.0 / 24.0, -35.0 / 256.0>(eta2) * eta4,
          horner<-7.0 / 9.0, 7.0 / 6.0, 1.0 / 2.0, 1.0 / 108.0>(eta2),
          horner<-35.0 / 36.0, -35.0 / 384.0>(eta2) * eta2,
          horner<7.0 / 8.0, 3.0 / 8.0, 1.0 / 144.0>(eta2),
          horner<-7.0 / 9.0, -7.0 / 96.0>(eta2),
          horner<5.0 / 16.0, 5.0 / 864.0>(eta2),
          -Num{1.0 / 16.0},
          Num{35.0 / 6912.0},
      };
      return L * rho_coeffs[1] * eta2 + z * horner(rho, rho_coeffs);
    };
    return unit_segment_integral(Num{2.0}, eta, z_min, z_max, primitive);
  }

  /// Scalar antigradient flux over a segment.
  template<class Num>
  static constexpr auto unit_antigrad_flux(Num eta,
                                           Num z_min,
                                           Num z_max) noexcept -> Num {
    constexpr auto primitive = [](Num eta_, Num z, Num rho, Num A, Num L) {
      const auto eta2 = pow2(eta_);
      const auto eta4 = pow2(eta2);
      // clang-format off
      const std::array rho_coeffs{
          horner<-1.0 / 2.0, 7.0 / 18.0, -7.0 / 18.0, -1.0 / 8.0, -1.0 / 540.0>(eta2),
          horner<5.0 / 24.0, 35.0 / 2304.0>(eta2) * eta4,
          horner<7.0 / 36.0, -7.0 / 36.0, -1.0 / 16.0, -1.0 / 1080.0>(eta2),
          horner<5.0 / 36.0, 35.0 / 3456.0>(eta2) * eta2,
          horner<-7.0 / 48.0, -3.0 / 64.0, -1.0 / 1440.0>(eta2),
          horner<1.0 / 9.0, 7.0 / 864.0>(eta2),
          horner<-5.0 / 128.0, -1.0 / 1728.0>(eta2),
          Num{1.0 / 144.0},
          -Num{7.0 / 13824.0},
      };
      // clang-format on
      return Num{2.0 / 9.0} * A +
             eta_ * (L * rho_coeffs[1] * eta2 + z * horner(rho, rho_coeffs));
    };
    return unit_segment_integral(Num{2.0}, eta, z_min, z_max, primitive);
  }

}; // class SixthOrderWendlandKernel

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Wendland's 8-th order (C6) smoothing kernel (Wendland, 1995).
class EighthOrderWendlandKernel final : public Kernel {
public:

  using Kernel::unit_antigrad_flux;
  using Kernel::unit_flux;

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

  /// Scalar kernel flux over a segment.
  template<class Num>
  static constexpr auto unit_flux(Num eta, Num z_min, Num z_max) noexcept
      -> Num {
    constexpr auto primitive = [](Num eta_, Num z, Num rho, Num /*A*/, Num L) {
      const auto eta2 = pow2(eta_);
      const auto eta4 = pow2(eta2);
      const auto eta6 = eta4 * eta2;
      // clang-format off
      const std::array rho_coeffs{
          horner<1.0, -11.0 / 6.0, 11.0 / 5.0, -33.0 / 10.0, -11.0 / 6.0, -1.0 / 12.0>(eta2),
          horner<1155.0 / 512.0, 693.0 / 2048.0, 231.0 / 65536.0>(eta2) * eta6,
          horner<-11.0 / 12.0, 11.0 / 10.0, -33.0 / 20.0, -11.0 / 12.0, -1.0 / 24.0>(eta2),
          horner<385.0 / 256.0, 231.0 / 1024.0, 77.0 / 32768.0>(eta2) * eta4,
          horner<33.0 / 40.0, -99.0 / 80.0, -11.0 / 16.0, -1.0 / 32.0>(eta2),
          horner<77.0 / 64.0, 231.0 / 1280.0, 77.0 / 40960.0>(eta2) * eta2,
          horner<-33.0 / 32.0, -55.0 / 96.0, -5.0 / 192.0>(eta2),
          horner<33.0 / 32.0, 99.0 / 640.0, 33.0 / 20480.0>(eta2),
          horner<-385.0 / 768.0, -35.0 / 1536.0>(eta2),
          horner<11.0 / 80.0, 11.0 / 7680.0>(eta2),
          -Num{21.0 / 1024.0},
          Num{1.0 / 768.0},
      };
      // clang-format on
      return L * rho_coeffs[1] * eta2 + z * horner(rho, rho_coeffs);
    };
    return unit_segment_integral(Num{2.0}, eta, z_min, z_max, primitive);
  }

  /// Scalar antigradient flux over a segment.
  template<class Num>
  static constexpr auto unit_antigrad_flux(Num eta,
                                           Num z_min,
                                           Num z_max) noexcept -> Num {
    constexpr auto primitive = [](Num eta_, Num z, Num rho, Num A, Num L) {
      const auto eta2 = pow2(eta_);
      const auto eta4 = pow2(eta2);
      const auto eta6 = eta4 * eta2;
      // clang-format off
      const std::array rho_coeffs{
          horner<-1.0 / 2.0, 11.0 / 24.0, -11.0 / 30.0, 33.0 / 80.0, 11.0 / 60.0, 1.0 / 144.0>(eta2),
          horner<-385.0 / 1536.0, -63.0 / 2048.0, -231.0 / 851968.0>(eta2) * eta6,
          horner<11.0 / 48.0, -11.0 / 60.0, 33.0 / 160.0, 11.0 / 120.0, 1.0 / 288.0>(eta2),
          horner<-385.0 / 2304.0, -21.0 / 1024.0, -77.0 / 425984.0>(eta2) * eta4,
          horner<-11.0 / 80.0, 99.0 / 640.0, 11.0 / 160.0, 1.0 / 384.0>(eta2),
          horner<-77.0 / 576.0, -21.0 / 1280.0, -77.0 / 532480.0>(eta2) * eta2,
          horner<33.0 / 256.0, 11.0 / 192.0, 5.0 / 2304.0>(eta2),
          horner<-11.0 / 96.0, -9.0 / 640.0, -33.0 / 266240.0>(eta2),
          horner<77.0 / 1536.0, 35.0 / 18432.0>(eta2),
          horner<-1.0 / 80.0, -11.0 / 99840.0>(eta2),
          Num{7.0 / 4096.0},
          -Num{1.0 / 9984.0},
      };
      // clang-format on
      return Num{7.0 / 39.0} * A +
             eta_ * (L * rho_coeffs[1] * eta2 + z * horner(rho, rho_coeffs));
    };
    return unit_segment_integral(Num{2.0}, eta, z_min, z_max, primitive);
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
