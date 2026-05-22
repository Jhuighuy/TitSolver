/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <array>
#include <cstddef>
#include <numbers>

#include "tit/core/math.hpp"
#include "tit/core/utils.hpp"
#include "tit/core/vec.hpp"
#include "tit/geom/bbox.hpp"
#include "tit/geom/bsphere.hpp"
#include "tit/geom/shape/face.hpp"
#include "tit/sph/kernel.hpp"
#include "tit/testing/math/integrals.hpp"
#include "tit/testing/numbers/dual.hpp"
#include "tit/testing/test.hpp"

namespace tit {
namespace {

#define KERNEL_TYPES                                                           \
  TIT_PASS(sph::CubicSplineKernel,                                             \
           sph::QuarticSplineKernel,                                           \
           sph::QuinticSplineKernel,                                           \
           sph::QuarticWendlandKernel,                                         \
           sph::SixthOrderWendlandKernel,                                      \
           sph::EighthOrderWendlandKernel)

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE_TEMPLATE("sph::Kernel::operator()", Kernel, KERNEL_TYPES) {
  // Ensure that the kernel is normalized: the integral of the kernel
  // over the sphere is equal to 1 (for any `h`).
  const Kernel w{};
  SUBCASE("over sphere") {
    using std::numbers::pi;
    SUBCASE("2D") {
      using Sphere2D = geom::BSphere<Vec<double, 2>>;
      for (const double h : {1.0, 0.1, 0.01}) {
        CAPTURE(h);
        const auto r = w.radius(h);
        const auto i = integrate(std::bind_back(w, h), Sphere2D{{}, r});
        CHECK(approx_equal_to(i, 1.0));
      }
    }
    SUBCASE("3D") {
      using Sphere3D = geom::BSphere<Vec<double, 3>>;
      for (const double h : {1.0, 0.1, 0.01}) {
        CAPTURE(h);
        const auto r = w.radius(h);
        const auto i = integrate(std::bind_back(w, h), Sphere3D{{}, r});
        CHECK(approx_equal_to(i, 1.0));
      }
    }
  }

  // Also ensure the normalization over a box (instead of a sphere).
  // This check also ensures that the kernel is symmetric and vanishes
  // outside the support sphere.
  SUBCASE("over box") {
    SUBCASE("1D") {
      using Box1D = geom::BBox<Vec<double, 1>>;
      for (const double h : {1.0, 0.1, 0.01}) {
        CAPTURE(h);
        const auto r = w.radius(h);
        const auto i = integrate(std::bind_back(w, h), Box1D{-r, +r});
        CHECK(approx_equal_to(i, 1.0));
      }
    }
    SUBCASE("2D") {
      using Box2D = geom::BBox<Vec<double, 2>>;
      for (const double h : {1.0, 0.1, 0.01}) {
        CAPTURE(h);
        const auto r = w.radius(h);
        const auto i =
            integrate(std::bind_back(w, h), Box2D{{-r, -r}, {+r, +r}});
        CHECK(approx_equal_to(i, 1.0));
      }
    }
    SUBCASE("3D") {
      using Box3D = geom::BBox<Vec<double, 3>>;
      for (const double h : {1.0, 0.1, 0.01}) {
        CAPTURE(h);
        const auto r = w.radius(h);
        const auto i =
            integrate(std::bind_back(w, h), Box3D{{-r, -r, -r}, {+r, +r, +r}});
        CHECK(approx_equal_to(i, 1.0));
      }
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE_TEMPLATE("sph::Kernel::grad", Kernel, KERNEL_TYPES) {
  // Ensure that the kernel gradient is computed correctly:
  // calculate the derivative of the kernel value using dual numbers
  // and compare it to the exact kernel gradient.
  const Kernel w{};
  SUBCASE("single component") {
    for (const double h : {1.0, 0.1, 0.01}) {
      CAPTURE(h);
      REQUIRE(w.radius(h) >= h * std::numbers::sqrt3);
      const auto x = pow2(h) * Vec{0.1, 0.1, 0.1};
      const auto value =
          w(Vec{Dual{x[0], 1.0}, Dual{x[1]}, Dual{x[2]}}, Dual{h});
      const auto gradient = w.grad(x, h);
      CHECK(approx_equal_to(value.deriv(), gradient[0]));
    }
  }
  SUBCASE("multiple components") {
    for (const double h : {1.0, 0.1, 0.01}) {
      CAPTURE(h);
      REQUIRE(w.radius(h) >= h * std::numbers::sqrt3);
      const auto x = pow2(h) * Vec{0.1, 0.1, 0.1};
      const auto value =
          w(Vec{Dual{x[0], 1.0}, Dual{x[1], 1.0}, Dual{x[2], 1.0}}, Dual{h});
      const auto gradient = w.grad(x, h);
      CHECK(approx_equal_to(value.deriv(), sum(gradient)));
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE_TEMPLATE("sph::Kernel::width_deriv", Kernel, KERNEL_TYPES) {
  // Ensure that the kernel width derivative is computed correctly:
  // calculate the derivative of the kernel value using dual numbers
  // and compare it to the exact width derivative.
  const Kernel w{};
  for (const double h : {1.0, 0.1, 0.01}) {
    CAPTURE(h);
    REQUIRE(w.radius(h) >= h * std::numbers::sqrt3);
    const auto x = pow2(h) * Vec{0.1, 0.1, 0.1};
    const auto value = w(vec_cast<Dual<double>>(x), Dual{h, 1.0});
    const auto dw_dh = w.width_deriv(x, h);
    CHECK(approx_equal_to(value.deriv(), dw_dh));
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE_TEMPLATE("sph::Kernel::antigrad", Kernel, KERNEL_TYPES) {
  // Ensure that the compact antigradient field vanishes at the support radius,
  // points inward inside the support, and has the kernel value as its
  // divergence away from the origin.
  const Kernel w{};
  SUBCASE("support") {
    for (const double h : {1.0, 0.1, 0.01}) {
      CAPTURE(h);
      const auto r_support = w.radius(h);
      CHECK(w.antigrad(Vec{r_support}, h) == Vec{0.0});
      CHECK(w.antigrad(Vec{1.01 * r_support}, h) == Vec{0.0});
      CHECK(w.antigrad(Vec{r_support, 0.0}, h) == Vec{0.0, 0.0});
      CHECK(w.antigrad(Vec{1.01 * r_support, 0.0}, h) == Vec{0.0, 0.0});
      CHECK(w.antigrad(Vec{r_support, 0.0, 0.0}, h) == Vec{0.0, 0.0, 0.0});
      CHECK(w.antigrad(Vec{1.01 * r_support, 0.0, 0.0}, h) ==
            Vec{0.0, 0.0, 0.0});

      const auto x = Vec{0.9 * r_support, 0.0, 0.0};
      const auto S = w.antigrad(x, h);
      CHECK(dot(S, x) < 0.0);
      CHECK(norm2(S) > 0.0);
    }
  }
  SUBCASE("divergence") {
    SUBCASE("1D") {
      for (const double h : {1.0, 0.1, 0.01}) {
        CAPTURE(h);
        const auto x = h * Vec{0.37};
        const auto S = w.antigrad(Vec{Dual{x[0], 1.0}}, Dual{h});
        CHECK(approx_equal_to(S[0].deriv(), w(x, h)));
      }
    }
    SUBCASE("2D") {
      for (const double h : {1.0, 0.1, 0.01}) {
        CAPTURE(h);
        const auto x = h * Vec{0.37, 0.23};
        const auto S_x = w.antigrad(Vec{Dual{x[0], 1.0}, Dual{x[1]}}, Dual{h});
        const auto S_y = w.antigrad(Vec{Dual{x[0]}, Dual{x[1], 1.0}}, Dual{h});
        CHECK(approx_equal_to(S_x[0].deriv() + S_y[1].deriv(), w(x, h)));
      }
    }
    SUBCASE("3D") {
      for (const double h : {1.0, 0.1, 0.01}) {
        CAPTURE(h);
        const auto x = h * Vec{0.37, 0.23, 0.11};
        const auto S_x =
            w.antigrad(Vec{Dual{x[0], 1.0}, Dual{x[1]}, Dual{x[2]}}, Dual{h});
        const auto S_y =
            w.antigrad(Vec{Dual{x[0]}, Dual{x[1], 1.0}, Dual{x[2]}}, Dual{h});
        const auto S_z =
            w.antigrad(Vec{Dual{x[0]}, Dual{x[1]}, Dual{x[2], 1.0}}, Dual{h});
        CHECK(approx_equal_to(S_x[0].deriv() + S_y[1].deriv() + S_z[2].deriv(),
                              w(x, h)));
      }
    }
  }
}

template<class Kernel>
void check_flux(const Kernel& w,
                const geom::Face<Vec<double, 2>>& face,
                const Vec<double, 2>& x,
                const double h,
                const double tolerance = 1.0e-7) {
  const auto& [a, b] = face.verts();
  const auto e = b - a;
  const auto n = face.normal();
  const auto l = face.area();

  const auto expected = n * integrate(
                                [&](const Vec<double, 1>& p) {
                                  const auto t = p[0];
                                  const auto y = a + t * e;
                                  return l * w(x - y, h);
                                },
                                geom::BBox{Vec{0.0}, Vec{1.0}},
                                1.0e-9);
  REQUIRE(std::isfinite(norm(expected)));

  const auto actual = w.flux(face, x, h);
  REQUIRE(std::isfinite(norm(actual)));
  REQUIRE(norm(actual - dot(actual, n) * n) < 1.0e-12);

  CHECK(norm(actual - expected) < tolerance);
}

template<class Kernel>
void check_flux(const Kernel& w,
                const geom::Face<Vec<double, 3>>& face,
                const Vec<double, 3>& x,
                const double h,
                const double tolerance = 1.0e-7) {
  const auto& [a, b, c] = face.verts();
  const auto e1 = b - a;
  const auto e2 = c - a;
  const auto n = face.normal();
  const auto l = face.area();

  const auto expected = n * integrate(
                                [&](const Vec<double, 2>& p) {
                                  const auto s = p[0];
                                  const auto t = p[1];
                                  const auto y =
                                      a + s * ((1.0 - t) * e1 + t * e2);
                                  return 2.0 * l * s * w(x - y, h);
                                },
                                geom::BBox{Vec{0.0, 0.0}, Vec{1.0, 1.0}},
                                1.0e-9);
  REQUIRE(std::isfinite(norm(expected)));
  CAPTURE(norm(expected));

  const auto actual = w.flux(face, x, h);
  REQUIRE(std::isfinite(norm(actual)));
  REQUIRE(norm(actual - dot(actual, n) * n) < 1.0e-12);
  CAPTURE(norm(actual));

  CHECK(norm(actual - expected) < tolerance);
}

template<class Kernel>
void check_antigradient_flux(const Kernel& w,
                             const geom::Face<Vec<double, 2>>& face,
                             const Vec<double, 2>& x,
                             const double h,
                             const double tolerance = 1.0e-7) {
  const auto& [a, b] = face.verts();
  const auto e = b - a;
  const auto n = face.normal();
  const auto l = face.area();

  const auto expected = integrate(
      [&](const Vec<double, 1>& p) {
        const auto t = p[0];
        const auto y = a + t * e;
        return -l * dot(n, w.antigrad(x - y, h));
      },
      geom::BBox{Vec{0.0}, Vec{1.0}},
      1.0e-9);
  REQUIRE(std::isfinite(expected));

  const auto actual = w.antigrad_flux(face, x, h);
  REQUIRE(std::isfinite(actual));

  CHECK(abs(actual - expected) < tolerance);
}

template<class Kernel>
void check_antigradient_flux(const Kernel& w,
                             const geom::Face<Vec<double, 3>>& face,
                             const Vec<double, 3>& x,
                             const double h,
                             const double tolerance = 1.0e-7) {
  const auto& [a, b, c] = face.verts();
  const auto e1 = b - a;
  const auto e2 = c - a;
  const auto n = face.normal();
  const auto l = face.area();

  const auto expected = integrate(
      [&](const Vec<double, 2>& p) {
        const auto s = p[0];
        const auto t = p[1];
        const auto y = a + s * ((1.0 - t) * e1 + t * e2);
        return -2.0 * l * s * dot(n, w.antigrad(x - y, h));
      },
      geom::BBox{Vec{0.0, 0.0}, Vec{1.0, 1.0}},
      1.0e-9);
  REQUIRE(std::isfinite(expected));

  const auto actual = w.antigrad_flux(face, x, h);
  REQUIRE(std::isfinite(actual));

  CHECK(abs(actual - expected) < tolerance);
}

template<class Kernel, std::size_t Dim>
void check_fluxes(const Kernel& w,
                  const geom::Face<Vec<double, Dim>>& face,
                  const Vec<double, Dim>& x,
                  const double h,
                  const double kernel_tolerance = 1.0e-7,
                  const double antigrad_tolerance = 1.0e-7) {
  check_flux(w, face, x, h, kernel_tolerance);
  check_antigradient_flux(w, face, x, h, antigrad_tolerance);
}

TEST_CASE_TEMPLATE("sph::Kernel::flux", Kernel, KERNEL_TYPES) {
  const Kernel w{};
  SUBCASE("outside support") {
    SUBCASE("2D") {
      const geom::Face face{std::array{
          Vec{10.0, 0.0},
          Vec{11.0, 0.0},
      }};
      for (const double h : {1.0, 0.1, 0.01}) {
        CAPTURE(h);
        CHECK(w.flux(face, Vec{0.0, 0.0}, h) == Vec{0.0, 0.0});
        CHECK(w.antigrad_flux(face, Vec{0.0, 1.0}, h) == 0.0);
      }
    }
    SUBCASE("3D") {
      const geom::Face face{std::array{
          Vec{10.0, 0.0, 0.0},
          Vec{11.0, 0.0, 0.0},
          Vec{10.0, 1.0, 0.0},
      }};
      for (const double h : {1.0, 0.1, 0.01}) {
        CAPTURE(h);
        CHECK(w.flux(face, Vec{0.0, 0.0, 0.0}, h) == Vec{0.0, 0.0, 0.0});
        CHECK(w.antigrad_flux(face, Vec{0.0, 0.0, 1.0}, h) == 0.0);
      }
    }
  }
  SUBCASE("regular") {
    SUBCASE("2D") {
      const Vec x{0.1, 0.7};
      const geom::Face face{std::array{
          Vec{-0.2, -0.1},
          Vec{+0.3, +0.1},
      }};
      for (const double h : {2.0, 1.0, 0.5}) {
        CAPTURE(h);
        check_fluxes(w, face, x, h);
      }
    }
    SUBCASE("3D") {
      const Vec x{0.1, 0.0, 0.6};
      const geom::Face face{std::array{
          Vec{-0.2, -0.1, 0.0},
          Vec{+0.3, -0.1, 0.0},
          Vec{-0.1, +0.3, 0.0},
      }};
      for (const double h : {2.0, 1.0, 0.5}) {
        CAPTURE(h);
        check_fluxes(w, face, x, h);
      }
    }
  }
  SUBCASE("flipped") {
    SUBCASE("2D") {
      const Vec x{0.1, 0.7};
      const geom::Face face{std::array{
          Vec{+0.3, +0.1},
          Vec{-0.2, -0.1},
      }};
      for (const double h : {2.0, 1.0, 0.5}) {
        CAPTURE(h);
        check_fluxes(w, face, x, h);
      }
    }
    SUBCASE("3D") {
      const Vec x{0.1, 0.0, 0.6};
      const geom::Face face{std::array{
          Vec{-0.2, -0.1, 0.0},
          Vec{-0.1, +0.3, 0.0},
          Vec{+0.3, -0.1, 0.0},
      }};
      for (const double h : {2.0, 1.0, 0.5}) {
        CAPTURE(h);
        check_fluxes(w, face, x, h);
      }
    }
  }
  SUBCASE("on supporting line/plane") {
    SUBCASE("2D") {
      const Vec x{-1.0, -1.0};
      const geom::Face face{std::array{
          Vec{0.0, 0.0},
          Vec{1.0, 1.0},
      }};
      for (const double h : {2.0, 1.0, 0.5}) {
        CAPTURE(h);
        check_fluxes(w, face, x, h);
      }
    }
    SUBCASE("3D") {
      const Vec x{1.0, 1.0, 0.0};
      const geom::Face face{std::array{
          Vec{0.0, 0.0, 0.0},
          Vec{1.0, 0.0, 0.0},
          Vec{0.0, 1.0, 0.0},
      }};
      for (const double h : {2.0, 1.0, 0.5}) {
        CAPTURE(h);
        check_fluxes(w, face, x, h);
      }
    }
  }
  SUBCASE("on face") {
    SUBCASE("2D") {
      const Vec x{0.5, 0.5};
      const geom::Face face{std::array{
          Vec{0.0, 0.0},
          Vec{1.0, 1.0},
      }};
      for (const double h : {2.0, 1.0, 0.5}) {
        CAPTURE(h);
        check_flux(w, face, x, h);
        CHECK(std::isfinite(w.antigrad_flux(face, x, h)));
      }
    }
    SUBCASE("3D") {
      // Note: Here we do not check antigradient fluxes, because the 3D
      //       triangle trace is still singular in this implementation.
      const Vec x{0.5, 0.5, 0.0};
      const geom::Face face{std::array{
          Vec{0.0, 0.0, 0.0},
          Vec{1.0, 0.0, 0.0},
          Vec{0.0, 1.0, 0.0},
      }};
      for (const double h : {2.0, 1.0, 0.5}) {
        CAPTURE(h);
        check_flux(w, face, x, h);
      }
    }
  }
  SUBCASE("singularity value") {
    constexpr auto eps = 1.0e-8;
    SUBCASE("2D") {
      SUBCASE("on supporting line") {
        for (const double h : {1.0, 0.1, 0.01}) {
          CAPTURE(h);
          const auto r = w.radius(h);
          const geom::Face face{std::array{
              Vec{r, 0.0},
              Vec{3.0 * r, 0.0},
          }};
          const auto n = face.normal();
          const auto trace = w.antigrad_flux(face, Vec{0.0, 0.0}, h);
          const auto limit = w.antigrad_flux(face, h * eps * n, h);
          CHECK(approx_equal_to(trace, 0.0));
          CHECK(abs(trace - limit) < 1.0e-6);
        }
      }
      SUBCASE("on face") {
        for (const double h : {1.0, 0.1, 0.01}) {
          CAPTURE(h);
          const auto r = w.radius(h);
          const geom::Face face{std::array{
              Vec{-2.0 * r, 0.0},
              Vec{+2.0 * r, 0.0},
          }};
          const auto n = face.normal();
          const auto trace = w.antigrad_flux(face, Vec{0.0, 0.0}, h);
          const auto limit = w.antigrad_flux(face, h * eps * n, h);
          CHECK(approx_equal_to(trace, 0.5));
          CHECK(abs(trace - limit) < 1.0e-6);
        }
      }
      SUBCASE("on face vertex") {
        for (const double h : {1.0, 0.1, 0.01}) {
          CAPTURE(h);
          const auto r = w.radius(h);
          const geom::Face face{std::array{
              Vec{0.0, 0.0},
              Vec{2.0 * r, 0.0},
          }};
          const auto n = face.normal();
          const auto trace = w.antigrad_flux(face, Vec{0.0, 0.0}, h);
          const auto limit = w.antigrad_flux(face, h * eps * n, h);
          CHECK(approx_equal_to(trace, 0.25));
          CHECK(abs(trace - limit) < 1.0e-6);
        }
      }
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
