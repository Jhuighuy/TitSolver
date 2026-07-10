/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "tit/core/vec.hpp"
#include "tit/geom/surface.hpp"
#include "tit/geom/tessellation.hpp"
#include "tit/geom/winding.hpp"
#include "tit/geom/winding/fast_winding.hpp"
#include "tit/testing/test.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto make_square(bool reverse = false) -> geom::Surface<Vec<double, 2>> {
  geom::Surface<Vec<double, 2>> surf;
  surf.append_vert({0.0, 0.0});
  surf.append_vert({1.0, 0.0});
  surf.append_vert({1.0, 1.0});
  surf.append_vert({0.0, 1.0});
  if (reverse) {
    surf.append_face({3, 0});
    surf.append_face({2, 3});
    surf.append_face({1, 2});
    surf.append_face({0, 1});
  } else {
    surf.append_face({0, 1});
    surf.append_face({1, 2});
    surf.append_face({2, 3});
    surf.append_face({3, 0});
  }
  return surf;
}

auto make_tetrahedron(bool reverse = false) -> geom::Surface<Vec<double, 3>> {
  geom::Surface<Vec<double, 3>> surf;
  surf.append_vert({0.0, 0.0, 0.0});
  surf.append_vert({1.0, 0.0, 0.0});
  surf.append_vert({0.0, 1.0, 0.0});
  surf.append_vert({0.0, 0.0, 1.0});
  if (reverse) {
    surf.append_face({0, 1, 2});
    surf.append_face({0, 3, 1});
    surf.append_face({0, 2, 3});
    surf.append_face({1, 3, 2});
  } else {
    surf.append_face({0, 2, 1});
    surf.append_face({0, 1, 3});
    surf.append_face({0, 3, 2});
    surf.append_face({1, 2, 3});
  }
  return surf;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::ExactWindingIndex") {
  SUBCASE("empty surface") {
    const geom::Surface<Vec<double, 2>> surf;
    const auto winding = geom::make_exact_winding(surf);
    CHECK_APPROX_EQ(winding(Vec{0.0, 0.0}), 0.0);
    CHECK_FALSE(winding.contains(Vec{0.0, 0.0}));
  }
  SUBCASE("degenerate face") {
    geom::Surface<Vec<double, 3>> surf;
    surf.append_vert({0.0, 0.0, 0.0});
    surf.append_vert({1.0, 0.0, 0.0});
    surf.append_vert({2.0, 0.0, 0.0});
    surf.append_face({0, 1, 2});
    surf.append_face({2, 1, 0});
    const auto winding = geom::make_exact_winding(surf);
    CHECK_APPROX_EQ(winding({0.0, 1.0, 0.0}), 0.0);
    CHECK_FALSE(winding.contains({0.0, 1.0, 0.0}));
  }
  SUBCASE("2D") {
    const auto surf = make_square();
    const auto winding = geom::make_exact_winding(surf);

    constexpr Vec p1{0.5, 0.5};
    CHECK_APPROX_EQ(winding(p1), 1.0);
    CHECK(winding.contains(p1));

    constexpr Vec p2{1.5, 0.5};
    CHECK_APPROX_EQ(winding(p2), 0.0);
    CHECK_FALSE(winding.contains(p2));
  }
  SUBCASE("3D") {
    const auto surf = make_tetrahedron();
    const auto winding = geom::make_exact_winding(surf);

    constexpr Vec p1{0.25, 0.25, 0.25};
    CHECK_APPROX_EQ(winding(p1), 1.0);
    CHECK(winding.contains(p1));

    constexpr Vec p2{1.0, 1.0, 1.0};
    CHECK_APPROX_EQ(winding(p2), 0.0);
    CHECK_FALSE(winding.contains(p2));
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::FastWindingIndex") {
  const geom::MakeFastWinding<double> make_fast_winding;
  SUBCASE("empty surface") {
    const geom::Surface<Vec<double, 2>> surf;
    const auto winding = make_fast_winding(surf);
    CHECK_APPROX_EQ(winding(Vec{0.0, 0.0}), 0.0);
    CHECK_FALSE(winding.contains(Vec{0.0, 0.0}));
  }
  SUBCASE("degenerate face") {
    geom::Surface<Vec<double, 3>> surf;
    surf.append_vert({0.0, 0.0, 0.0});
    surf.append_vert({1.0, 0.0, 0.0});
    surf.append_vert({2.0, 0.0, 0.0});
    surf.append_face({0, 1, 2});
    surf.append_face({2, 1, 0});
    const auto winding = make_fast_winding(surf);
    CHECK_APPROX_EQ(winding({0.0, 1.0, 0.0}), 0.0);
    CHECK_FALSE(winding.contains({0.0, 1.0, 0.0}));
  }
  SUBCASE("2D") {
    for (const auto reverse : {false, true}) {
      CAPTURE(reverse);
      const auto surf = geom::tessellate(make_square(reverse), 0.05);

      const auto exact_winding = geom::make_exact_winding(surf);
      const auto fast_winding = make_fast_winding(surf);

      for (const auto& point : {
               Vec{0.1, 0.1},
               Vec{0.5, 0.5},
               Vec{0.9, 0.75},
               Vec{-0.5, 0.5},
               Vec{0.5, 1.5},
               Vec{2.0, 2.0},
               Vec{100.0, 100.0},
           }) {
        const auto exact = exact_winding(point);
        CHECK_APPROX_EQ(fast_winding(point), exact, 6e-3);
        CHECK(fast_winding.contains(point) == (exact > 0.5));
      }
    }
  }
  SUBCASE("3D") {
    for (const auto reverse : {false, true}) {
      CAPTURE(reverse);
      const auto surf = geom::tessellate(make_tetrahedron(), 0.05);

      const auto exact_winding = geom::make_exact_winding(surf);
      const auto fast_winding = make_fast_winding(surf);

      for (const auto& point : {
               Vec{0.1, 0.1, 0.1},
               Vec{0.25, 0.25, 0.25},
               Vec{1.0, 1.0, 1.0},
               Vec{3.0, 2.0, 1.0},
               Vec{100.0, 100.0, 100.0},
           }) {
        CAPTURE(point);
        const auto exact = exact_winding(point);
        CHECK_APPROX_EQ(fast_winding(point), exact, 2e-3);
        for (const auto threshold : {0.0, 0.5, 1.0}) {
          CAPTURE(threshold);
          CHECK(fast_winding.contains(point, threshold) == (exact > threshold));
        }
      }
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
