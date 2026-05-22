/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <numbers>

#include "tit/core/math.hpp"
#include "tit/core/utils.hpp"
#include "tit/core/vec.hpp"
#include "tit/geom/bbox.hpp"
#include "tit/geom/bsphere.hpp"
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
      for (const auto h : {1.0, 0.1, 0.01}) {
        CAPTURE(h);
        const auto r = w.radius(h);
        const auto i = integrate(std::bind_back(w, h), //
                                 geom::BSphere{Vec{0.0, 0.0}, r});
        CHECK_APPROX_EQ(i, 1.0);
      }
    }
    SUBCASE("3D") {
      for (const auto h : {1.0, 0.1, 0.01}) {
        CAPTURE(h);
        const auto r = w.radius(h);
        const auto i = integrate(std::bind_back(w, h),
                                 geom::BSphere{Vec{0.0, 0.0, 0.0}, r});
        CHECK_APPROX_EQ(i, 1.0);
      }
    }
  }

  // Also ensure the normalization over a box (instead of a sphere).
  // This check also ensures that the kernel is symmetric and vanishes
  // outside the support sphere.
  SUBCASE("over box") {
    SUBCASE("1D") {
      for (const auto h : {1.0, 0.1, 0.01}) {
        CAPTURE(h);
        const auto r = w.radius(h);
        const auto i = integrate(std::bind_back(w, h), //
                                 geom::BBox{Vec{-r}, Vec{+r}});
        CHECK_APPROX_EQ(i, 1.0);
      }
    }
    SUBCASE("2D") {
      for (const auto h : {1.0, 0.1, 0.01}) {
        CAPTURE(h);
        const auto r = w.radius(h);
        const auto i = integrate(std::bind_back(w, h),
                                 geom::BBox{Vec{-r, -r}, Vec{+r, +r}});
        CHECK_APPROX_EQ(i, 1.0);
      }
    }
    SUBCASE("3D") {
      for (const auto h : {1.0, 0.1, 0.01}) {
        CAPTURE(h);
        const auto r = w.radius(h);
        const auto i = integrate(std::bind_back(w, h),
                                 geom::BBox{Vec{-r, -r, -r}, Vec{+r, +r, +r}});
        CHECK_APPROX_EQ(i, 1.0);
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
    for (const auto h : {1.0, 0.1, 0.01}) {
      CAPTURE(h);
      REQUIRE(w.radius(h) >= h * std::numbers::sqrt3);
      const auto x = pow2(h) * Vec{0.1, 0.1, 0.1};
      const auto value =
          w(Vec{Dual{x[0], 1.0}, Dual{x[1]}, Dual{x[2]}}, Dual{h});
      const auto gradient = w.grad(x, h);
      CHECK_APPROX_EQ(value.deriv(), gradient[0]);
    }
  }
  SUBCASE("multiple components") {
    for (const auto h : {1.0, 0.1, 0.01}) {
      CAPTURE(h);
      REQUIRE(w.radius(h) >= h * std::numbers::sqrt3);
      const auto x = pow2(h) * Vec{0.1, 0.1, 0.1};
      const auto value =
          w(Vec{Dual{x[0], 1.0}, Dual{x[1], 1.0}, Dual{x[2], 1.0}}, Dual{h});
      const auto gradient = w.grad(x, h);
      CHECK_APPROX_EQ(value.deriv(), sum(gradient));
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE_TEMPLATE("sph::Kernel::width_deriv", Kernel, KERNEL_TYPES) {
  // Ensure that the kernel width derivative is computed correctly:
  // calculate the derivative of the kernel value using dual numbers
  // and compare it to the exact width derivative.
  const Kernel w{};
  for (const auto h : {1.0, 0.1, 0.01}) {
    CAPTURE(h);
    REQUIRE(w.radius(h) >= h * std::numbers::sqrt3);
    const auto x = pow2(h) * Vec{0.1, 0.1, 0.1};
    const auto value = w(vec_cast<Dual<double>>(x), Dual{h, 1.0});
    const auto dw_dh = w.width_deriv(x, h);
    CHECK_APPROX_EQ(value.deriv(), dw_dh);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE_TEMPLATE("sph::Kernel::antigrad", Kernel, KERNEL_TYPES) {
  // Ensure that the antigradient is computed correctly:
  // calculate the antigradient field using dual numbers and compare its
  // divergence to the exact kernel value.
  const Kernel w{};
  SUBCASE("1D") {
    for (const auto h : {1.0, 0.1, 0.01}) {
      CAPTURE(h);
      const auto x = h * Vec{0.37};
      const auto A = w.antigrad(Vec{Dual{x[0], 1.0}}, Dual{h});
      CHECK_APPROX_EQ(A[0].deriv(), w(x, h));
    }
  }
  SUBCASE("2D") {
    for (const auto h : {1.0, 0.1, 0.01}) {
      CAPTURE(h);
      const auto x = h * Vec{0.37, 0.23};
      const auto A_x = w.antigrad(Vec{Dual{x[0], 1.0}, Dual{x[1]}}, Dual{h});
      const auto A_y = w.antigrad(Vec{Dual{x[0]}, Dual{x[1], 1.0}}, Dual{h});
      const auto div_A = A_x[0].deriv() + A_y[1].deriv();
      CHECK_APPROX_EQ(div_A, w(x, h));
    }
  }
  SUBCASE("3D") {
    for (const auto h : {1.0, 0.1, 0.01}) {
      CAPTURE(h);
      const auto x = h * Vec{0.37, 0.23, 0.11};
      const auto A_x =
          w.antigrad(Vec{Dual{x[0], 1.0}, Dual{x[1]}, Dual{x[2]}}, Dual{h});
      const auto A_y =
          w.antigrad(Vec{Dual{x[0]}, Dual{x[1], 1.0}, Dual{x[2]}}, Dual{h});
      const auto A_z =
          w.antigrad(Vec{Dual{x[0]}, Dual{x[1]}, Dual{x[2], 1.0}}, Dual{h});
      const auto div_A = A_x[0].deriv() + A_y[1].deriv() + A_z[2].deriv();
      CHECK_APPROX_EQ(div_A, w(x, h));
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
