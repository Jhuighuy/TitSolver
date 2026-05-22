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
#include "tit/geom/segment.hpp"
#include "tit/geom/triangle.hpp"
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

TEST_CASE_TEMPLATE("sph::Kernel::flux", Kernel, KERNEL_TYPES) {
  const Kernel w{};
  SUBCASE("2D") {
    const auto estimate = [&w](const geom::Segment<Vec<double, 2>>& seg,
                               const Vec<double, 2>& x,
                               double h) {
      return seg.normal() *
             integrate(
                 [&w, &x, &h](const Vec<double, 2>& y) { return w(x - y, h); },
                 seg);
    };
    SUBCASE("segment fully outside support") {
      constexpr Vec x{10.0, 10.0};
      constexpr geom::Segment seg{Vec{0.0, 0.0}, Vec{2.0, 0.0}};
      for (const auto h : {1.0, 0.1, 0.01}) {
        CAPTURE(h);
        CHECK_APPROX_EQ(estimate(seg, x, h), Vec{0.0, 0.0});
        CHECK_APPROX_EQ(w.flux(seg, x, h), Vec{0.0, 0.0});
      }
    }
    SUBCASE("segment partially intersects support") {
      for (const auto h : {1.0, 0.1, 0.01}) {
        CAPTURE(h);
        const geom::Segment seg{Vec{0.0, 0.0}, Vec{2.0 * h, 0.0}};
        const geom::Segment seg_flipped{seg.b(), seg.a()};
        for (const auto o : {1.0, 0.9, 0.5, 0.1, 0.0, -0.1, -0.5, -0.9, -1.0}) {
          CAPTURE(o);
          for (const auto s : {1.0, 0.9, 0.5, 0.1, 0.0}) {
            CAPTURE(s);
            const Vec x{1.0 + o * h, s * h};
            CHECK_APPROX_EQ(w.flux(seg, x, h), estimate(seg, x, h));
            CHECK_APPROX_EQ(w.flux(seg, x, h), -w.flux(seg_flipped, x, h));
          }
        }
      }
    }
    SUBCASE("segment fully inside support") {
      for (const auto h : {1.0, 0.1, 0.01}) {
        CAPTURE(h);
        const geom::Segment seg{Vec{-h / 4, 0.0}, Vec{h / 4, 0.0}};
        const geom::Segment seg_flipped{seg.b(), seg.a()};
        for (const auto o : {1.0, 0.9, 0.5, 0.1, 0.0, -0.1, -0.5, -0.9, -1.0}) {
          CAPTURE(o);
          for (const auto s : {1.0, 0.9, 0.5, 0.1, 0.0}) {
            CAPTURE(s);
            const Vec x{o * h, s * h};
            CHECK_APPROX_EQ(w.flux(seg, x, h), estimate(seg, x, h));
            CHECK_APPROX_EQ(w.flux(seg, x, h), -w.flux(seg_flipped, x, h));
          }
        }
      }
    }
  }
  SUBCASE("3D") {
    const auto estimate = [&w](const geom::Triangle<Vec<double, 3>>& tri,
                               const Vec<double, 3>& x,
                               double h) {
      return tri.normal() *
             integrate(
                 [&w, &x, &h](const Vec<double, 3>& y) { return w(x - y, h); },
                 tri);
    };
    SUBCASE("triangle fully outside support") {
      constexpr Vec x{10.0, 10.0, 10.0};
      constexpr geom::Triangle tri{
          Vec{2.0, 0.0, 0.0},
          Vec{0.0, 2.0, 0.0},
          Vec{0.0, 0.0, 2.0},
      };
      for (const auto h : {1.0, 0.1, 0.01}) {
        CAPTURE(h);
        CHECK_APPROX_EQ(estimate(tri, x, h), Vec{0.0, 0.0, 0.0});
        CHECK_APPROX_EQ(w.flux(tri, x, h), Vec{0.0, 0.0, 0.0});
      }
    }
    SUBCASE("triangle partially intersects support") {
      for (const auto h : {1.0, 0.1, 0.01}) {
        CAPTURE(h);
        const geom::Triangle tri{
            Vec{3.0 * h, 0.0, 0.0},
            Vec{0.0, 3.0 * h, 0.0},
            Vec{0.0, 0.0, 3.0 * h},
        };
        const geom::Triangle tri_flipped{tri.c(), tri.b(), tri.a()};
        for (const auto o : {1.0, 0.9, 0.5, 0.1, 0.0, -0.1, -0.5, -0.9, -1.0}) {
          CAPTURE(o);
          for (const auto s : {1.0, 0.9, 0.5, 0.1, 0.0}) {
            CAPTURE(s);
            const Vec x{1.0 + o * h, 1.0 + s * h, 1.0 - s * h};
            CHECK_APPROX_EQ(w.flux(tri, x, h), estimate(tri, x, h));
            CHECK_APPROX_EQ(w.flux(tri, x, h), -w.flux(tri_flipped, x, h));
          }
        }
      }
    }
    SUBCASE("triangle fully inside support") {
      for (const auto h : {1.0, 0.1, 0.01}) {
        CAPTURE(h);
        const geom::Triangle tri{
            Vec{h / 6.0, 0.0, 0.0},
            Vec{0.0, h / 6.0, 0.0},
            Vec{0.0, 0.0, h / 6.0},
        };
        const geom::Triangle tri_flipped{tri.c(), tri.b(), tri.a()};
        for (const auto o : {1.0, 0.9, 0.5, 0.1, 0.0, -0.1, -0.5, -0.9, -1.0}) {
          CAPTURE(o);
          for (const auto s : {1.0, 0.9, 0.5, 0.1, 0.0}) {
            CAPTURE(s);
            const Vec x{o * h, s * h, s * h};
            CHECK_APPROX_EQ(w.flux(tri, x, h), estimate(tri, x, h));
            CHECK_APPROX_EQ(w.flux(tri, x, h), -w.flux(tri_flipped, x, h));
          }
        }
      }
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE_TEMPLATE("sph::Kernel::antigrad_flux", Kernel, KERNEL_TYPES) {
  const Kernel w{};
  SUBCASE("2D") {
    const auto estimate = [&w](const geom::Segment<Vec<double, 2>>& seg,
                               const Vec<double, 2>& x,
                               double h) {
      return -dot(seg.normal(),
                  integrate(
                      [&w, &x, &h](const Vec<double, 2>& y) {
                        return w.antigrad(x - y, h);
                      },
                      seg));
    };
    SUBCASE("segment fully outside support") {
      constexpr Vec x{10.0, 10.0};
      constexpr geom::Segment seg{Vec{0.0, 0.0}, Vec{2.0, 0.0}};
      for (const auto h : {1.0, 0.1, 0.01}) {
        CAPTURE(h);
        CHECK_APPROX_EQ(estimate(seg, x, h), 0.0);
        CHECK_APPROX_EQ(w.antigrad_flux(seg, x, h), 0.0);
      }
    }
    SUBCASE("segment partially intersects support") {
      for (const auto h : {1.0, 0.1, 0.01}) {
        CAPTURE(h);
        const geom::Segment seg{Vec{0.0, 0.0}, Vec{2.0 * h, 0.0}};
        const geom::Segment seg_flipped{seg.b(), seg.a()};
        for (const auto o : {1.0, 0.9, 0.5, 0.1, 0.0, -0.1, -0.5, -0.9, -1.0}) {
          CAPTURE(o);
          for (const auto s : {1.0, 0.9, 0.5, 0.1}) {
            CAPTURE(s);
            const Vec x{1.0 + o * h, s * h};
            const auto flux = w.antigrad_flux(seg, x, h);
            CHECK_APPROX_EQ(flux, estimate(seg, x, h));
            CHECK_APPROX_EQ(flux, -w.antigrad_flux(seg_flipped, x, h));
          }
        }
      }
    }
    SUBCASE("segment fully inside support") {
      for (const auto h : {1.0, 0.1, 0.01}) {
        CAPTURE(h);
        for (const auto o : {1.0, 0.9, 0.5, 0.1, 0.0, -0.1, -0.5, -0.9, -1.0}) {
          CAPTURE(o);
          for (const auto s : {1.0, 0.9, 0.5, 0.1}) {
            CAPTURE(s);
            const Vec x{o * h, s * h};
            const geom::Segment seg{Vec{-h / 2, 0.0}, Vec{h / 2, 0.0}};
            const geom::Segment seg_flipped{seg.b(), seg.a()};
            const auto flux = w.antigrad_flux(seg, x, h);
            CHECK_APPROX_EQ(flux, estimate(seg, x, h));
            CHECK_APPROX_EQ(flux, -w.antigrad_flux(seg_flipped, x, h));
          }
        }
      }
    }
    SUBCASE("point on the segment line") {
      // Note: When a point is on the segment line, distance to the plane is
      //       prone to signed zero, so the sign of the flux fluctuates
      //       between positive and negative values. Therefore, we only check
      //       the absolute values.
      constexpr geom::Segment seg{Vec{0.0, 0.0}, Vec{2.0, 2.0}};
      SUBCASE("outside segment") {
        for (const auto o : {1.0, 0.9, 0.5, 0.1}) {
          CAPTURE(o);
          for (const auto h : {1.0, 0.1, 0.01}) {
            CAPTURE(h);
            CHECK_APPROX_EQ(w.antigrad_flux(seg, seg.a() - Vec{o, o}, h), 0.0);
            CHECK_APPROX_EQ(w.antigrad_flux(seg, seg.b() + Vec{o, o}, h), 0.0);
          }
        }
      }
      SUBCASE("on segment vertices") {
        for (const auto h : {1.0, 0.1, 0.01}) {
          CAPTURE(h);
          CHECK_APPROX_EQ(abs(w.antigrad_flux(seg, seg.a(), h)), 0.25);
          CHECK_APPROX_EQ(abs(w.antigrad_flux(seg, seg.b(), h)), 0.25);
        }
      }
      SUBCASE("inside segment") {
        for (const auto o : {0.9, 0.5, 0.1, 0.0, -0.1, -0.5, -0.9}) {
          CAPTURE(o);
          const auto t = 0.5 * (o + 1.0);
          for (const auto h : {1.0, 0.1, 0.01}) {
            CAPTURE(h);
            CHECK_APPROX_EQ(
                abs(w.antigrad_flux(seg, seg.a() + t * seg.ba(), h)),
                0.5);
          }
        }
      }
    }
  }
  SUBCASE("3D") {
    const auto estimate = [&w](const geom::Triangle<Vec<double, 3>>& tri,
                               const Vec<double, 3>& x,
                               double h) {
      return -dot(tri.normal(),
                  integrate(
                      [&w, &x, &h](const Vec<double, 3>& y) {
                        return w.antigrad(x - y, h);
                      },
                      tri));
    };
    SUBCASE("triangle fully outside support") {
      constexpr Vec x{10.0, 10.0, 10.0};
      constexpr geom::Triangle tri{
          Vec{2.0, 0.0, 0.0},
          Vec{0.0, 2.0, 0.0},
          Vec{0.0, 0.0, 2.0},
      };
      for (const auto h : {1.0, 0.1, 0.01}) {
        CAPTURE(h);
        CHECK_APPROX_EQ(estimate(tri, x, h), 0.0);
        CHECK_APPROX_EQ(w.antigrad_flux(tri, x, h), 0.0);
      }
    }
    SUBCASE("triangle partially intersects support") {
      for (const auto h : {1.0, 0.1, 0.01}) {
        CAPTURE(h);
        const geom::Triangle tri{
            Vec{3.0 * h, 0.0, 0.0},
            Vec{0.0, 3.0 * h, 0.0},
            Vec{0.0, 0.0, 3.0 * h},
        };
        const geom::Triangle tri_flipped{tri.c(), tri.b(), tri.a()};
        for (const auto o : {1.0, 0.9, 0.5, 0.1, -0.1, -0.5, -0.9, -1.0}) {
          CAPTURE(o);
          for (const auto s : {1.0, 0.9, 0.5, 0.1}) {
            CAPTURE(s);
            const Vec x{1.0 + o * h, 1.0 + s * h, 1.0 - s * h};
            const auto flux = w.antigrad_flux(tri, x, h);
            CHECK_APPROX_EQ(flux, estimate(tri, x, h));
            CHECK_APPROX_EQ(flux, -w.antigrad_flux(tri_flipped, x, h));
          }
        }
      }
    }
    SUBCASE("triangle fully inside support") {
      for (const auto h : {1.0, 0.1, 0.01}) {
        CAPTURE(h);
        const geom::Triangle tri{
            Vec{h / 6.0, 0.0, 0.0},
            Vec{0.0, h / 6.0, 0.0},
            Vec{0.0, 0.0, h / 6.0},
        };
        const geom::Triangle tri_flipped{tri.c(), tri.b(), tri.a()};
        for (const auto o : {1.0, 0.9, 0.5, 0.1, -0.1, -0.5, -0.9, -1.0}) {
          CAPTURE(o);
          for (const auto s : {1.0, 0.9, 0.5, 0.1}) {
            CAPTURE(s);
            const Vec x{o * h, s * h, -s * h};
            const auto flux = w.antigrad_flux(tri, x, h);
            CHECK_APPROX_EQ(flux, estimate(tri, x, h));
            CHECK_APPROX_EQ(flux, -w.antigrad_flux(tri_flipped, x, h));
          }
        }
      }
    }
    SUBCASE("point on the triangle plane") {
      // Note: When a point is on the triangle plane, distance to the plane is
      //       prone to signed zero, so the sign of the flux fluctuates
      //       between positive and negative values. Therefore, we only check
      //       the absolute values.
      constexpr geom::Triangle tri{
          Vec{1.0, 0.0, 0.0},
          Vec{0.0, 1.0, 0.0},
          Vec{0.0, 0.0, 1.0},
      };
      SUBCASE("outside triangle") {
        for (const auto o : {1.0, 0.9, 0.5, 0.1}) {
          CAPTURE(o);
          for (const auto h : {1.0, 0.1, 0.01}) {
            CAPTURE(h);
            CHECK_APPROX_EQ(
                w.antigrad_flux(tri, tri.a() + Vec{2 * o, -o, -o}, h),
                0.0);
            CHECK_APPROX_EQ(
                w.antigrad_flux(tri, tri.b() + Vec{-o, 2 * o, -o}, h),
                0.0);
            CHECK_APPROX_EQ(
                w.antigrad_flux(tri, tri.c() + Vec{-o, -o, 2 * o}, h),
                0.0);
          }
        }
      }
      /// @todo Since we are doing only numerical integration, we need to
      ///       cannot properly test singularities yet.
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
