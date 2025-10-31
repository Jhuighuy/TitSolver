/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <algorithm>
#include <array>
#include <numeric>
#include <utility>

#include "tit/core/basic_types.hpp"
#include "tit/core/vec.hpp"
#include "tit/geom/bipartition.hpp"
#include "tit/geom/point_range.hpp"
#include "tit/testing/test.hpp"

namespace tit {
namespace {

using Vec2D = Vec<double, 2>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::CoordBisection") {
  // Create points on a 4x4 lattice.
  std::array<Vec2D, 16> points{};
  for (size_t i = 0; i < 16; ++i) points[i] = {i % 4, i / 4};

  // Initialize the permutation.
  std::array<size_t, 16> perm{};
  std::ranges::iota(perm, size_t{0});

  // Partition and ensure the result is correct.
  constexpr size_t axis = 0;
  constexpr auto pivot = 2.5;
  for (const auto reverse : {false, true}) {
    // Partition the points.
    auto [left_perm, right_perm] =
        geom::coord_bisection(points, perm, pivot, axis, reverse);
    if (reverse) std::swap(left_perm, right_perm);

    // Sort the permutations, since the exact order is not guaranteed.
    std::ranges::sort(left_perm), std::ranges::sort(right_perm);

    // Ensure the result is correct.
    CHECK_RANGE_EQ(left_perm, {0, 1, 2, 4, 5, 6, 8, 9, 10, 12, 13, 14});
    CHECK_RANGE_EQ(right_perm, {3, 7, 11, 15});
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::DirBisection") {
  // Create points on a 4x4 lattice.
  std::array<Vec2D, 16> points{};
  for (size_t i = 0; i < 16; ++i) points[i] = {i % 4, i / 4};

  // Initialize the permutation.
  std::array<size_t, 16> perm{};
  std::ranges::iota(perm, size_t{0});

  // Partition and ensure the result is correct.
  const auto dir = normalize(Vec2D{1, 1});
  constexpr auto pivot = 1.75;
  for (const auto reverse : {false, true}) {
    // Partition the points.
    auto [left_perm, right_perm] =
        geom::dir_bisection(points, perm, pivot, dir, reverse);
    if (reverse) std::swap(left_perm, right_perm);

    // Sort the permutations, since the exact order is not guaranteed.
    std::ranges::sort(left_perm), std::ranges::sort(right_perm);

    // Ensure the result is correct.
    CHECK_RANGE_EQ(left_perm, {0, 1, 2, 4, 5, 8});
    CHECK_RANGE_EQ(right_perm, {3, 6, 7, 9, 10, 11, 12, 13, 14, 15});
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::CoordMedianSplit") {
  SUBCASE("specified axis") {
    // Create points on a 4x4 lattice.
    std::array<Vec2D, 16> points{};
    for (size_t i = 0; i < 16; ++i) points[i] = {i % 4, i / 4};

    // Initialize the permutation.
    std::array<size_t, 16> perm{};
    std::ranges::iota(perm, size_t{0});

    // Partition the points.
    constexpr size_t axis = 0;
    const auto [left_perm, right_perm] =
        geom::coord_median_split(points, perm, 8, axis);

    // Sort the permutations, since the exact order is not guaranteed.
    std::ranges::sort(left_perm), std::ranges::sort(right_perm);

    // Ensure the result is correct.
    CHECK_RANGE_EQ(left_perm, {0, 1, 4, 5, 8, 9, 12, 13});
    CHECK_RANGE_EQ(right_perm, {2, 3, 6, 7, 10, 11, 14, 15});
  }
  SUBCASE("longes axis") {
    // Create points on a 5x4 lattice.
    std::array<Vec2D, 20> points{};
    for (size_t i = 0; i < 20; ++i) points[i] = {i % 5, i / 5};

    // Initialize the permutation.
    std::array<size_t, 20> perm{};
    std::ranges::iota(perm, size_t{0});

    // Partition the points.
    const auto [left_perm, right_perm] =
        geom::coord_median_split(points, perm, 12);

    // Sort the permutations, since the exact order is not guaranteed.
    std::ranges::sort(left_perm), std::ranges::sort(right_perm);

    // Ensure the result is correct.
    CHECK_RANGE_EQ(left_perm, {0, 1, 2, 5, 6, 7, 10, 11, 12, 15, 16, 17});
    CHECK_RANGE_EQ(right_perm, {3, 4, 8, 9, 13, 14, 18, 19});
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::DirMedianSplit") {
  // Create points on a 4x4 lattice.
  std::array<Vec2D, 16> points{};
  for (size_t i = 0; i < 16; ++i) points[i] = {i % 4, i / 4};

  // Initialize the permutation.
  std::array<size_t, 16> perm{};
  std::ranges::iota(perm, size_t{0});

  // Partition the points.
  const auto dir = normalize(Vec2D{1, 1});
  const auto [left_perm, right_perm] =
      geom::dir_median_split(points, perm, 6, dir);

  // Sort the permutations, since the exact order is not guaranteed.
  std::ranges::sort(left_perm), std::ranges::sort(right_perm);

  // Ensure the result is correct.
  CHECK_RANGE_EQ(left_perm, {0, 1, 2, 4, 5, 8});
  CHECK_RANGE_EQ(right_perm, {3, 6, 7, 9, 10, 11, 12, 13, 14, 15});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::InertialMedianSplit") {
  // Create points on a 5x4 lattice "rotated" by 45 degrees.
  //
  // Note: we cannot use a square lattice as inertia tensor will always be
  // a scaled identity matrix, and the partitioning won't produce the
  // expected result.
  std::array<Vec2D, 20> rotated_points{};
  for (size_t i = 0; i < 20; ++i) {
    const Vec2D point{i % 5, i / 5};
    rotated_points[i] = {point[0] + point[1], point[0] - point[1]};
  }

  // Ensure that the inertia axis is correct.
  const auto inertia_axis = geom::compute_largest_inertia_axis(rotated_points);
  REQUIRE(inertia_axis);
  REQUIRE_APPROX_EQ(normalize(*inertia_axis), normalize(Vec2D{1, 1}));

  // Initialize the permutation.
  std::array<size_t, 20> perm{};
  std::ranges::iota(perm, size_t{0});

  // Partition the points.
  const auto [left_perm, right_perm] =
      geom::inertial_median_split(rotated_points, perm, 12);

  // Sort the permutations, since the exact order is not guaranteed.
  std::ranges::sort(left_perm), std::ranges::sort(right_perm);

  // Ensure the result is correct.
  CHECK_RANGE_EQ(left_perm, {0, 1, 2, 5, 6, 7, 10, 11, 12, 15, 16, 17});
  CHECK_RANGE_EQ(right_perm, {3, 4, 8, 9, 13, 14, 18, 19});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
