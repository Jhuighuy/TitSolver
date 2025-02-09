/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <algorithm>
#include <array>
#include <utility>

#include "tit/core/basic_types.hpp"
#include "tit/core/utils.hpp"
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
  iota_perm(points, perm);

  // Partition and ensure the result is correct.
  constexpr size_t axis = 0;
  constexpr auto pivot = 2.5;
  constexpr std::array<size_t, 12>
      expected_left_perm{0, 1, 2, 4, 5, 6, 8, 9, 10, 12, 13, 14};
  constexpr std::array<size_t, 4> expected_right_perm{3, 7, 11, 15};
  for (const auto reverse : {false, true}) {
    // Partition the points.
    auto [left_perm, right_perm] =
        geom::coord_bisection(points, perm, pivot, axis, reverse);
    if (reverse) std::swap(left_perm, right_perm);

    // Sort the permutations, since the exact order is not guaranteed.
    std::ranges::sort(left_perm), std::ranges::sort(right_perm);

    // Ensure the result is correct.
    CHECK_RANGE_EQ(left_perm, expected_left_perm);
    CHECK_RANGE_EQ(right_perm, expected_right_perm);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::DirBisection") {
  // Create points on a 4x4 lattice.
  std::array<Vec2D, 16> points{};
  for (size_t i = 0; i < 16; ++i) points[i] = {i % 4, i / 4};

  // Initialize the permutation.
  std::array<size_t, 16> perm{};
  iota_perm(points, perm);

  // Partition and ensure the result is correct.
  const auto dir = normalize(Vec2D{1, 1});
  constexpr auto pivot = 1.75;
  constexpr std::array<size_t, 6> expected_left_perm = {0, 1, 2, 4, 5, 8};
  constexpr std::array<size_t, 10> expected_right_perm =
      {3, 6, 7, 9, 10, 11, 12, 13, 14, 15};
  for (const auto reverse : {false, true}) {
    // Partition the points.
    auto [left_perm, right_perm] =
        geom::dir_bisection(points, perm, pivot, dir, reverse);
    if (reverse) std::swap(left_perm, right_perm);

    // Sort the permutations, since the exact order is not guaranteed.
    std::ranges::sort(left_perm), std::ranges::sort(right_perm);

    // Ensure the result is correct.
    CHECK_RANGE_EQ(left_perm, expected_left_perm);
    CHECK_RANGE_EQ(right_perm, expected_right_perm);
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
    iota_perm(points, perm);

    // Partition the points.
    constexpr size_t axis = 0;
    const auto [left_perm, right_perm] =
        geom::coord_median_split(points, perm, perm.begin() + 8, axis);

    // Sort the permutations, since the exact order is not guaranteed.
    std::ranges::sort(left_perm), std::ranges::sort(right_perm);

    // Ensure the result is correct.
    constexpr std::array<size_t, 8>
        expected_left_perm{0, 1, 4, 5, 8, 9, 12, 13};
    constexpr std::array<size_t, 8>
        expected_right_perm{2, 3, 6, 7, 10, 11, 14, 15};
    CHECK_RANGE_EQ(left_perm, expected_left_perm);
    CHECK_RANGE_EQ(right_perm, expected_right_perm);
  }
  SUBCASE("longes axis") {
    // Create points on a 5x4 lattice.
    std::array<Vec2D, 20> points{};
    for (size_t i = 0; i < 20; ++i) points[i] = {i % 5, i / 5};

    // Initialize the permutation.
    std::array<size_t, 20> perm{};
    iota_perm(points, perm);

    // Partition the points.
    const auto [left_perm, right_perm] =
        geom::coord_median_split(points, perm, perm.begin() + 12);

    // Sort the permutations, since the exact order is not guaranteed.
    std::ranges::sort(left_perm), std::ranges::sort(right_perm);

    // Ensure the result is correct.
    constexpr std::array<size_t, 12>
        expected_left_perm{0, 1, 2, 5, 6, 7, 10, 11, 12, 15, 16, 17};
    constexpr std::array<size_t, 8>
        expected_right_perm{3, 4, 8, 9, 13, 14, 18, 19};
    CHECK_RANGE_EQ(left_perm, expected_left_perm);
    CHECK_RANGE_EQ(right_perm, expected_right_perm);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::DirMedianSplit") {
  // Create points on a 4x4 lattice.
  std::array<Vec2D, 16> points{};
  for (size_t i = 0; i < 16; ++i) points[i] = {i % 4, i / 4};

  // Initialize the permutation.
  std::array<size_t, 16> perm{};
  iota_perm(points, perm);

  // Partition the points.
  const auto dir = normalize(Vec2D{1, 1});
  const auto [left_perm, right_perm] =
      geom::dir_median_split(points, perm, perm.begin() + 6, dir);

  // Sort the permutations, since the exact order is not guaranteed.
  std::ranges::sort(left_perm), std::ranges::sort(right_perm);

  // Ensure the result is correct.
  constexpr std::array<size_t, 6> expected_left_perm{0, 1, 2, 4, 5, 8};
  constexpr std::array<size_t, 10>
      expected_right_perm{3, 6, 7, 9, 10, 11, 12, 13, 14, 15};
  CHECK_RANGE_EQ(left_perm, expected_left_perm);
  CHECK_RANGE_EQ(right_perm, expected_right_perm);
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
  iota_perm(rotated_points, perm);

  // Partition the points.
  const auto [left_perm, right_perm] =
      geom::inertial_median_split(rotated_points, perm, perm.begin() + 12);

  // Sort the permutations, since the exact order is not guaranteed.
  std::ranges::sort(left_perm), std::ranges::sort(right_perm);

  // Ensure the result is correct.
  constexpr std::array<size_t, 12>
      expected_left_perm{0, 1, 2, 5, 6, 7, 10, 11, 12, 15, 16, 17};
  constexpr std::array<size_t, 8>
      expected_right_perm{3, 4, 8, 9, 13, 14, 18, 19};
  CHECK_RANGE_EQ(left_perm, expected_left_perm);
  CHECK_RANGE_EQ(right_perm, expected_right_perm);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
