/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <array>
#include <ranges>

#include "tit/core/basic_types.hpp"
#include "tit/core/vec.hpp"
#include "tit/geom/partition/recursive_bisection.hpp"
#include "tit/testing/test.hpp"

namespace tit {
namespace {

using Vec2D = Vec<double, 2>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::CoordBisection") {
  // Create points on a 8x16 lattice.
  std::array<Vec2D, 128> points{};
  for (size_t i = 0; i < 128; ++i) points[i] = {i % 16, i / 16};

  // Partition the points using the coordinate bisection algorithm.
  std::array<size_t, 128> parts{};
  geom::recursive_coord_bisection(points, parts, 8);

  // Ensure the resulting partitioning is correct.
  //
  // 0 --------------------------------->
  // | 0 0 0 0 2 2 2 2 4 4 4 4 6 6 6 6    X
  // | 0 0 0 0 2 2 2 2 4 4 4 4 6 6 6 6
  // | 0 0 0 0 2 2 2 2 4 4 4 4 6 6 6 6
  // | 0 0 0 0 2 2 2 2 4 4 4 4 6 6 6 6
  // | 1 1 1 1 3 3 3 3 5 5 5 5 7 7 7 7
  // | 1 1 1 1 3 3 3 3 5 5 5 5 7 7 7 7
  // | 1 1 1 1 3 3 3 3 5 5 5 5 7 7 7 7
  // | 1 1 1 1 3 3 3 3 5 5 5 5 7 7 7 7
  // |
  // v
  //   Y
  for (const auto& [part, point] : std::views::zip(parts, points)) {
    const auto point_int = vec_cast<size_t>(point);
    CHECK(part == 2 * (point_int[0] / 4) + (point_int[1] / 4));
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::InertialBisection") {
  SUBCASE("axis-aligned") {
    // Create points on a 8x8 lattice.
    std::array<Vec2D, 64> points{};
    for (size_t i = 0; i < 64; ++i) points[i] = {i % 8, i / 8};

    // Partition the points using the inertial bisection algorithm.
    std::array<size_t, 64> parts{};
    geom::recursive_inertial_bisection(points, parts, 4);

    // Ensure the resulting partitioning is correct.
    // We shall have 4 parts in each quadrant:
    //
    // 0 ----------------->
    // | 0 0 0 0 2 2 2 2    X
    // | 0 0 0 0 2 2 2 2
    // | 0 0 0 0 2 2 2 2
    // | 0 0 0 0 2 2 2 2
    // | 1 1 1 1 3 3 3 3
    // | 1 1 1 1 3 3 3 3
    // | 1 1 1 1 3 3 3 3
    // | 1 1 1 1 3 3 3 3
    // |
    // v
    //   Y
    for (const auto& [part, point] : std::views::zip(parts, points)) {
      const auto point_int = vec_cast<size_t>(point);
      CHECK(part == (point_int[0] / 4) * 2 + (point_int[1] / 4));
    }
  }

  SUBCASE("rotated") {
    // Create points on a 10x8 lattice "rotated" by 45 degrees.
    //
    // Note: we cannot use a square lattice as inertia tensor will always be
    // a scaled identity matrix, and the partitioning won't produce the
    // expected result.
    std::array<Vec2D, 80> rotated_points{};
    for (size_t i = 0; i < 80; ++i) {
      const Vec2D point{i % 10, i / 10};
      rotated_points[i] = 0.5 * Vec2D{point[0] + point[1], point[0] - point[1]};
    }

    // Partition the points using the inertial bisection algorithm.
    std::array<size_t, 80> parts{};
    geom::recursive_inertial_bisection(rotated_points, parts, 4);

    // Ensure the resulting partitioning is correct.
    for (const auto& [part, rotated_point] :
         std::views::zip(parts, rotated_points)) {
      // Rotate the point back.
      const Vec2D point{rotated_point[0] + rotated_point[1],
                        rotated_point[0] - rotated_point[1]};

      // Ensure the partition index is correct.
      const auto point_int = vec_cast<size_t>(point);
      CHECK(part == (point_int[0] / 5) * 2 + (point_int[1] / 4));
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
