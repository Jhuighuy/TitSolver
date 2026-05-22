/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <numbers>
#include <utility>

#include "tit/core/assert.hpp"
#include "tit/core/math.hpp"
#include "tit/core/type.hpp"
#include "tit/core/vec.hpp"
#include "tit/geom/shape/face.hpp"
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

  /// Kernel flux over a face.
  template<particle_view<required_fields> PV>
  constexpr auto flux(this auto& self,
                      const geom::Face<particle_vec_t<PV>>& face,
                      PV a) noexcept {
    static_assert(has_uniform<PV>(h));
    return self.flux(face, r[a], h[a]);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Support radius.
  template<class Num, class Self>
  constexpr auto radius(this Self& /*self*/, Num h) noexcept -> Num {
    TIT_ASSERT(h > Num{0.0}, "Kernel width must be positive!");
    return Self::template unit_radius<Num>() * h;
  }

  /// Value of the smoothing kernel at point.
  template<class Self, class Num, std::size_t Dim>
  constexpr auto operator()(this Self& self,
                            const Vec<Num, Dim>& x,
                            const Num& h) noexcept -> Num {
    TIT_ASSERT(h > Num{0.0}, "Kernel width must be positive!");
    const auto h_inverse = inverse(h);
    const auto w = self.template weight<Num, Dim>() * pow<Dim>(h_inverse);
    const auto q = h_inverse * norm(x);
    return w * self.unit_value(q);
  }

  /// Spatial gradient of the smoothing kernel at point.
  template<class Self, class Num, std::size_t Dim>
  constexpr auto grad(this Self& self,
                      const Vec<Num, Dim>& x,
                      const Num& h) noexcept -> Vec<Num, Dim> {
    TIT_ASSERT(h > Num{0.0}, "Kernel width must be positive!");
    const auto h_inverse = inverse(h);
    const auto w = self.template weight<Num, Dim>() * pow<Dim>(h_inverse);
    const auto q = h_inverse * norm(x);
    const auto grad_q = normalize(x) * h_inverse;
    return w * self.unit_deriv(q) * grad_q;
  }

  /// Width derivative of the smoothing kernel at point.
  template<class Self, class Num, std::size_t Dim>
  constexpr auto width_deriv(this Self& self,
                             const Vec<Num, Dim>& x,
                             const Num& h) noexcept -> Num {
    TIT_ASSERT(h > Num{0.0}, "Kernel width must be positive!");
    const auto h_inverse = inverse(h);
    const auto w = self.template weight<Num, Dim>() * pow<Dim>(h_inverse);
    const auto dw_dh = -Num{Dim} * w * h_inverse;
    const auto q = h_inverse * norm(x);
    const auto dq_dh = -q * h_inverse;
    return dw_dh * self.unit_value(q) + w * self.unit_deriv(q) * dq_dh;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Kernel flux over a face.
  template<class Self, class Num, std::size_t Dim>
  constexpr auto flux(this Self& self,
                      const geom::Face<Vec<Num, Dim>>& face,
                      const Vec<Num, Dim>& x,
                      const Num& h) noexcept -> Vec<Num, Dim> {
    TIT_ASSERT(h > Num{0.0}, "Kernel width must be positive!");
    if (is_tiny(face.area())) return {};
    if (!face.intersects_sphere(x, self.radius(h))) return {};
    const auto h_inverse = inverse(h);
    const auto w = self.template weight<Num, Dim>() * h_inverse;
    const auto n = face.normal();
    if constexpr (Dim == 2) {
      const auto& [a, b] = face.verts();
      const auto t = normalize(b - a);
      const auto d = dot(n, x - a) * h_inverse;
      const auto z_a = dot(x - a, t) * h_inverse;
      const auto z_b = dot(x - b, t) * h_inverse;
      return n * w *
             self.unit_flux(abs(d), std::min(z_a, z_b), std::max(z_a, z_b));
    } else if constexpr (Dim == 3) {
      const auto& [a, b, c] = face.verts();
      const auto d = dot(n, x - a) * h_inverse;
      const auto e1 = normalize(b - a);
      const auto e2 = cross(n, e1);
      const auto project = [&e1, &e2](const Vec<Num, 3>& y) {
        return Vec{dot(y, e1), dot(y, e2)};
      };
      return n * w *
             self.unit_flux(abs(d),
                            project(x - a) * h_inverse,
                            project(x - b) * h_inverse,
                            project(x - c) * h_inverse);
    } else {
      static_assert(false);
    }
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Scalar kernel flux over a triangle.
  /// @todo Replace this machinery with analytical expressions.
  template<class Self, class Num>
  constexpr auto unit_flux(this Self& self,
                           const Num& eta,
                           const Vec<Num, 2>& a,
                           const Vec<Num, 2>& b,
                           const Vec<Num, 2>& c) noexcept -> Num {
    const auto func = [&self](Num q) { return self.unit_value(q); };
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
    constexpr std::size_t num_subdivs = 8;
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

  using Kernel::unit_flux;

  /// Kernel weight.
  template<class Num, std::size_t Dim>
  static consteval auto weight() noexcept -> Num {
    if constexpr (Dim == 1) return Num{2.0 / 3.0};
    else if constexpr (Dim == 2) return Num{10.0 / 7.0 * std::numbers::inv_pi};
    else if constexpr (Dim == 3) return Num{std::numbers::inv_pi};
    else static_assert(false);
  }

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

  /// Scalar kernel flux over a 2D segment.
  template<class Num>
  constexpr auto unit_flux(const Num& eta,
                           const Num& z_min,
                           const Num& z_max) const noexcept -> Num {
    const auto primitive = [&eta](const Num& a, const Num& z) {
      const auto eta2 = pow2(eta);
      const auto rho = sqrt(pow2(z) + eta2);
      const auto log_term = is_tiny(eta) ? Num{0.0} : asinh(z / eta);
      const auto J0 = z;
      const auto J1 = Num{0.5} * (z * rho + eta2 * log_term);
      const auto J2 = z * pow<2>(rho) / Num{3.0} + Num{2.0 / 3.0} * eta2 * J0;
      const auto J3 = z * pow<3>(rho) / Num{4.0} + Num{3.0 / 4.0} * eta2 * J1;
      return horner(a, {-J3, Num{3.0} * J2, -Num{3.0} * J1, J0});
    };
    const auto term = [&eta, &z_min, &z_max, &primitive](const Num& a) {
      if (eta >= a) return Num{0.0};
      const auto z_clip = sqrt(pow2(a) - pow2(eta));
      const auto z_lo = std::max(z_min, -z_clip);
      const auto z_hi = std::min(z_max, +z_clip);
      if (z_lo >= z_hi) return Num{0.0};
      return primitive(a, z_hi) - primitive(a, z_lo);
    };
    return Num{0.25} * term(Num{2.0}) - term(Num{1.0});
  }

}; // class CubicSplineKernel

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// The quartic B-spline (M5) smoothing kernel.
class QuarticSplineKernel final : public Kernel {
public:

  using Kernel::unit_flux;

  /// Kernel weight.
  template<class Num, std::size_t Dim>
  static consteval auto weight() noexcept -> Num {
    using std::numbers::inv_pi;
    if constexpr (Dim == 1) return Num{1.0 / 24.0};
    else if constexpr (Dim == 2) return Num{96.0 / 1199.0 * inv_pi};
    else if constexpr (Dim == 3) return Num{1.0 / 20.0 * inv_pi};
    else static_assert(false);
  }

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

  /// Scalar kernel flux over a 2D segment.
  template<class Num>
  constexpr auto unit_flux(const Num& eta,
                           const Num& z_min,
                           const Num& z_max) const noexcept -> Num {
    const auto primitive = [&eta](const Num& a, const Num& z) {
      const auto eta2 = pow2(eta);
      const auto rho = sqrt(pow2(z) + eta2);
      const auto log_term = is_tiny(eta) ? Num{0.0} : asinh(z / eta);
      const auto J0 = z;
      const auto J1 = Num{0.5} * (z * rho + eta2 * log_term);
      const auto J2 = z * pow<2>(rho) / Num{3.0} + Num{2.0 / 3.0} * eta2 * J0;
      const auto J3 = z * pow<3>(rho) / Num{4.0} + Num{3.0 / 4.0} * eta2 * J1;
      const auto J4 = z * pow<4>(rho) / Num{5.0} + Num{4.0 / 5.0} * eta2 * J2;
      return horner(a, {J4, -Num{4.0} * J3, Num{6.0} * J2, -Num{4.0} * J1, J0});
    };
    const auto term = [&eta, &z_min, &z_max, &primitive](const Num& a) {
      if (eta >= a) return Num{0.0};
      const auto z_clip = sqrt(pow2(a) - pow2(eta));
      const auto z_lo = std::max(z_min, -z_clip);
      const auto z_hi = std::min(z_max, +z_clip);
      if (z_lo >= z_hi) return Num{0.0};
      return primitive(a, z_hi) - primitive(a, z_lo);
    };
    return term(Num{2.5}) - Num{5.0} * term(Num{1.5}) +
           Num{10.0} * term(Num{0.5});
  }

}; // class QuarticSplineKernel

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Quintic B-spline (M6) smoothing kernel.
class QuinticSplineKernel final : public Kernel {
public:

  using Kernel::unit_flux;

  /// Kernel weight.
  template<class Num, std::size_t Dim>
  static consteval auto weight() noexcept {
    using std::numbers::inv_pi;
    if constexpr (Dim == 1) return Num{1.0 / 120.0};
    else if constexpr (Dim == 2) return Num{7.0 / 478.0 * inv_pi};
    else if constexpr (Dim == 3) return Num{1.0 / 120.0 * inv_pi};
    else static_assert(false);
  }

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

  /// Scalar kernel flux over a 2D segment.
  template<class Num>
  constexpr auto unit_flux(const Num& eta,
                           const Num& z_min,
                           const Num& z_max) const noexcept -> Num {
    const auto primitive = [&eta](const Num& a, const Num& z) {
      const auto eta2 = pow2(eta);
      const auto rho = sqrt(pow2(z) + eta2);
      const auto log_term = is_tiny(eta) ? Num{0.0} : asinh(z / eta);
      const auto J0 = z;
      const auto J1 = Num{0.5} * (z * rho + eta2 * log_term);
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
    const auto term = [&eta, &z_min, &z_max, &primitive](const Num& a) {
      if (eta >= a) return Num{0.0};
      const auto z_clip = sqrt(pow2(a) - pow2(eta));
      const auto z_lo = std::max(z_min, -z_clip);
      const auto z_hi = std::min(z_max, +z_clip);
      if (z_lo >= z_hi) return Num{0.0};
      return primitive(a, z_hi) - primitive(a, z_lo);
    };
    return term(Num{3.0}) - Num{6.0} * term(Num{2.0}) +
           Num{15.0} * term(Num{1.0});
  }

}; // class QuinticSplineKernel

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Wendland kernels.
//

/// Abstract Wendland's smoothing kernel.
class WendlandKernel : public Kernel {
public:

  /// Unit support radius.
  template<class Num>
  static consteval auto unit_radius() noexcept -> Num {
    // Wendland's kernels always have a support radius of 2.
    return Num{2.0};
  }

  /// Value of the unit smoothing kernel at a point.
  template<class Num>
  constexpr auto unit_value(this auto& self, Num q) noexcept -> Num {
    return q < self.template unit_radius<Num>() ? //
               self.unit_value_notrunc(q) :
               Num{0.0};
  }

  /// Derivative of the unit smoothing kernel at a point.
  template<class Num>
  constexpr auto unit_deriv(this auto& self, Num q) noexcept -> Num {
    return q < self.template unit_radius<Num>() ? //
               self.unit_deriv_notrunc(q) :
               Num{0.0};
  }

}; // class WendlandKernel

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Wendland's quartic (C2) smoothing kernel (Wendland, 1995).
class QuarticWendlandKernel final : public WendlandKernel {
public:

  using WendlandKernel::unit_flux;

  /// Kernel weight.
  template<class Num, std::size_t Dim>
  static consteval auto weight() noexcept -> Num {
    using std::numbers::inv_pi;
    if constexpr (Dim == 1) return Num{3.0 / 4.0};
    else if constexpr (Dim == 2) return Num{7.0 / 4.0 * inv_pi};
    else if constexpr (Dim == 3) return Num{21.0 / 16.0 * inv_pi};
    else static_assert(false);
  }

  /// Value of the unit smoothing kernel at a point (no truncation).
  template<class Num>
  static constexpr auto unit_value_notrunc(Num q) noexcept -> Num {
    return (Num{1.0} + Num{2.0} * q) * pow<4>(Num{1.0} - Num{0.5} * q);
  }

  /// Derivative of the unit smoothing kernel at a point (no truncation).
  template<class Num>
  static constexpr auto unit_deriv_notrunc(Num q) noexcept -> Num {
    // Well known formula is dW/dq = -5 * q * (1 - q/2)^3, but it requires 5
    // multiplications. Formula that is used requires 4.
    return Num{5.0 / 8.0} * q * pow<3>(q - Num{2.0});
  }

  /// Scalar kernel flux over a 2D segment.
  template<class Num>
  constexpr auto unit_flux(const Num& eta,
                           const Num& z_min,
                           const Num& z_max) const noexcept -> Num {
    if (eta >= unit_radius<Num>()) return {};
    const auto z_clip = sqrt(pow2(unit_radius<Num>()) - pow2(eta));
    const auto z_lo = std::max(z_min, -z_clip);
    const auto z_hi = std::min(z_max, +z_clip);
    if (z_lo >= z_hi) return {};
    const auto eta2 = pow2(eta);
    const std::array rho_coeffs{
        horner<1.0, -5.0 / 3.0, -1.0 / 2.0>(eta2),
        horner<15.0 / 16.0, 5.0 / 128.0>(eta2) * eta2,
        horner<-5.0 / 6.0, -1.0 / 4.0>(eta2),
        horner<5.0 / 8.0, 5.0 / 192.0>(eta2),
        -Num{3.0 / 16.0},
        Num{1.0 / 48.0},
    };
    const auto primitive = [&eta, &eta2, &rho_coeffs](const Num& z) {
      const auto rho = sqrt(pow2(z) + eta2);
      const auto log_term = is_tiny(eta) ? Num{0.0} : asinh(z / eta);
      return log_term * rho_coeffs[1] * eta2 + z * horner(rho, rho_coeffs);
    };
    return primitive(z_hi) - primitive(z_lo);
  }

}; // class QuarticWendlandKernel

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Wendland's 6-th order (C4) smoothing kernel (Wendland, 1995).
class SixthOrderWendlandKernel final : public WendlandKernel {
public:

  using WendlandKernel::unit_flux;

  /// Kernel weight.
  template<class Num, std::size_t Dim>
  static consteval auto weight() noexcept -> Num {
    using std::numbers::inv_pi;
    if constexpr (Dim == 1) return Num{27.0 / 32.0};
    else if constexpr (Dim == 2) return Num{9.0 / 4.0 * inv_pi};
    else if constexpr (Dim == 3) return Num{495.0 / 256.0 * inv_pi};
    else static_assert(false);
  }

  /// Value of the unit smoothing kernel at a point (no truncation).
  template<class Num>
  static constexpr auto unit_value_notrunc(Num q) noexcept -> Num {
    return horner<1.0, 3.0, 35.0 / 12.0>(q) * pow<6>(Num{1.0} - Num{0.5} * q);
  }

  /// Derivative of the unit smoothing kernel at a point (no truncation).
  template<class Num>
  static constexpr auto unit_deriv_notrunc(Num q) noexcept -> Num {
    return Num{7.0 / 96.0} * q * horner<2.0, 5.0>(q) * pow<5>(q - Num{2.0});
  }

  /// Scalar kernel flux over a 2D segment.
  template<class Num>
  constexpr auto unit_flux(const Num& eta,
                           const Num& z_min,
                           const Num& z_max) const noexcept -> Num {
    if (eta >= unit_radius<Num>()) return {};
    const auto z_clip = sqrt(pow2(unit_radius<Num>()) - pow2(eta));
    const auto z_lo = std::max(z_min, -z_clip);
    const auto z_hi = std::min(z_max, +z_clip);
    if (z_lo >= z_hi) return {};
    const auto eta2 = pow2(eta);
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
    const auto primitive = [&eta, &eta2, &rho_coeffs](const Num& z) {
      const auto rho = sqrt(pow2(z) + eta2);
      const auto log_term = is_tiny(eta) ? Num{0.0} : asinh(z / eta);
      return log_term * rho_coeffs[1] * eta2 + z * horner(rho, rho_coeffs);
    };
    return primitive(z_hi) - primitive(z_lo);
  }

}; // class SixthOrderWendlandKernel

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Wendland's 8-th order (C6) smoothing kernel (Wendland, 1995).
class EighthOrderWendlandKernel final : public WendlandKernel {
public:

  using WendlandKernel::unit_flux;

  /// Kernel weight.
  template<class Num, std::size_t Dim>
  static consteval auto weight() noexcept -> Num {
    using std::numbers::inv_pi;
    if constexpr (Dim == 1) return Num{15.0 / 16.0};
    else if constexpr (Dim == 2) return Num{39.0 / 14.0 * inv_pi};
    else if constexpr (Dim == 3) return Num{1365.0 / 512.0 * inv_pi};
    else static_assert(false);
  }

  /// Value of the unit smoothing kernel at a point (no truncation).
  template<class Num>
  static constexpr auto unit_value_notrunc(Num q) noexcept -> Num {
    return horner<1.0, 4.0, 25.0 / 4.0, 4.0>(q) *
           pow<8>(Num{1.0} - Num{0.5} * q);
  }

  /// Derivative of the unit smoothing kernel at a point (no truncation).
  template<class Num>
  static constexpr auto unit_deriv_notrunc(Num q) noexcept -> Num {
    return Num{11.0 / 512.0} * q * horner<2.0, 7.0, 8.0>(q) *
           pow<7>(q - Num{2.0});
  }

  /// Scalar kernel flux over a 2D segment.
  template<class Num>
  constexpr auto unit_flux(const Num& eta,
                           const Num& z_min,
                           const Num& z_max) const noexcept -> Num {
    if (eta >= unit_radius<Num>()) return {};
    const auto z_clip = sqrt(pow2(unit_radius<Num>()) - pow2(eta));
    const auto z_lo = std::max(z_min, -z_clip);
    const auto z_hi = std::min(z_max, +z_clip);
    if (z_lo >= z_hi) return {};
    const auto eta2 = pow2(eta);
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
    const auto primitive = [&eta, &eta2, &rho_coeffs](const Num& z) {
      const auto rho = sqrt(pow2(z) + eta2);
      const auto log_term = is_tiny(eta) ? Num{0.0} : asinh(z / eta);
      return log_term * rho_coeffs[1] * eta2 + z * horner(rho, rho_coeffs);
    };
    return primitive(z_hi) - primitive(z_lo);
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
