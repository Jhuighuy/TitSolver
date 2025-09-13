/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <numbers>

#include "tit/core/math.hpp"
#include "tit/core/utils.hpp"
#include "tit/core/vec.hpp"
#include "tit/geom/bbox.hpp"
#include "tit/sph/kernel.hpp"
#include "tit/testing/math/integrals.hpp"
#include "tit/testing/numbers/dual.hpp"
#include "tit/testing/test.hpp"

namespace tit {
namespace {

#define KERNEL_TYPES                                                           \
  TIT_PASS(sph::GaussianKernel,                                                \
           sph::CubicSplineKernel,                                             \
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
      for (const double h : {1.0, 0.1, 0.01}) {
        const auto r = w.radius(h);
        const auto i = integrate_cr(std::bind_back(w, h), r);
        CHECK(approx_equal_to(i, 1.0));
      }
    }
    SUBCASE("3D") {
      for (const double h : {1.0, 0.1, 0.01}) {
        const auto r = w.radius(h);
        const auto i = integrate_sp(std::bind_back(w, h), r);
        CHECK(approx_equal_to(i, 1.0));
      }
    }
  }

  // Also ensure the normalization over a box (instead of a sphere).
  // This check also ensures that the kernel is symmetric and vanishes
  // outside the support sphere.
  SUBCASE("over box") {
    SUBCASE("1D") {
      for (const double h : {1.0, 0.1, 0.01}) {
        using Box1D = geom::BBox<Vec<double, 1>>;
        const auto r = w.radius(h);
        const auto i = integrate(std::bind_back(w, h), Box1D{-r, +r});
        CHECK(approx_equal_to(i, 1.0));
      }
    }
    SUBCASE("2D") {
      for (const double h : {1.0, 0.1, 0.01}) {
        using Box2D = geom::BBox<Vec<double, 2>>;
        const auto r = w.radius(h);
        const auto i =
            integrate(std::bind_back(w, h), Box2D{{-r, -r}, {+r, +r}});
        CHECK(approx_equal_to(i, 1.0));
      }
    }
    SUBCASE("3D") {
      for (const double h : {1.0, 0.1, 0.01}) {
        using Box3D = geom::BBox<Vec<double, 3>>;
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
    REQUIRE(w.radius(h) >= h * std::numbers::sqrt3);
    const auto x = pow2(h) * Vec{0.1, 0.1, 0.1};
    const auto value = w(vec_cast<Dual>(x), Dual{h, 1.0});
    const auto dw_dh = w.width_deriv(x, h);
    CHECK(approx_equal_to(value.deriv(), dw_dh));
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
