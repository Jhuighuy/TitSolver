/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <array>
// #include <iostream>

// #include "tit/core/basic_types.hpp"
#include "tit/core/vec.hpp"

#include "tit/geom/utils.hpp"

#include "tit/testing/test.hpp"

namespace tit {
namespace {

using Vec2D = Vec<double, 2>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::count_points") {
  SUBCASE("empty") {
    constexpr std::array<Vec2D, 0> points{};
    CHECK(geom::count_points(points) == 0.0);
  }
  SUBCASE("non-empty") {
    constexpr std::array points{Vec2D{0, 0}, Vec2D{1, 1}, Vec2D{2, 2}};
    CHECK(geom::count_points(points) == 3.0);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#if 0
TEST_CASE("geom::compute_largest_inertia_axis") {
  // Create points on a 3x2 lattice rotated by 30 degrees:
  std::array<Vec2D, 6> rotated_points{};
  for (size_t i = 0; i < 6; ++i) {
    const Vec2D point{i % 2, i / 2};
    rotated_points[i] = {2 * point[0] - point[1], 2 * point[1] + point[0]};
    std::cerr << point << ", " << rotated_points[i] << std::endl;
  }

  SUBCASE("normal") {
    const auto axis = geom::compute_largest_inertia_axis(rotated_points);
    REQUIRE(axis);
    const auto expected_axis = normalize(Vec2D{2.0, 1.0});
    std::cerr << axis.value() << std::endl;
    CHECK_APPROX_EQ(normalize(axis.value()), expected_axis);
  }
}
#endif

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
