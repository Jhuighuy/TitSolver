/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <array>

#include "tit/core/mat.hpp"
#include "tit/core/vec.hpp"
#include "tit/geom/bbox.hpp"
#include "tit/geom/point_range.hpp"
#include "tit/testing/test.hpp"

namespace tit {
namespace {

using Vec2D = Vec<double, 2>;
using Mat2D = Mat<double, 2>;
using Box2D = geom::BBox<Vec2D>;

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

TEST_CASE("geom::compute_center") {
  constexpr std::array points{Vec2D{0, 0}, Vec2D{1, 1}, Vec2D{2, 2}};
  constexpr Vec2D expected_center{1, 1};
  SUBCASE("as is") {
    CHECK(geom::compute_center(points) == expected_center);
  }
  SUBCASE("permuted") {
    constexpr std::array perm{1, 2, 0};
    CHECK(geom::compute_center(points, perm) == expected_center);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::compute_bbox") {
  constexpr std::array points{Vec2D{0, 0}, Vec2D{1, 1}, Vec2D{2, 2}};
  constexpr Box2D expected_bbox{{0, 0}, {2, 2}};
  SUBCASE("as is") {
    const auto box = geom::compute_bbox(points);
    CHECK(box.low() == expected_bbox.low());
    CHECK(box.high() == expected_bbox.high());
  }
  SUBCASE("permuted") {
    constexpr std::array perm{1, 2, 0};
    const auto box = geom::compute_bbox(points, perm);
    CHECK(box.low() == expected_bbox.low());
    CHECK(box.high() == expected_bbox.high());
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::compute_inertia_tensor") {
  constexpr std::array points{Vec2D{0, 0}, Vec2D{1, 1}, Vec2D{2, 2}};
  constexpr Mat2D expected_tensor{{2.0, 2.0}, {2.0, 2.0}};
  SUBCASE("as is") {
    const auto tensor = geom::compute_inertia_tensor(points);
    CHECK(tensor == expected_tensor);
  }
  SUBCASE("permuted") {
    constexpr std::array perm{1, 2, 0};
    const auto tensor = geom::compute_inertia_tensor(points, perm);
    CHECK(tensor == expected_tensor);
  }
}

TEST_CASE("geom::compute_largest_inertia_axis") {
  constexpr std::array points{Vec2D{0, 1}, Vec2D{1, 0}, Vec2D{1, 1}};
  const auto expected_axis = normalize(Vec2D{1, -1});
  SUBCASE("as is") {
    const auto axis = geom::compute_largest_inertia_axis(points);
    REQUIRE(axis);
    CHECK_APPROX_EQ(normalize(*axis), expected_axis);
  }
  SUBCASE("permuted") {
    constexpr std::array perm{1, 2, 0};
    const auto axis = geom::compute_largest_inertia_axis(points, perm);
    REQUIRE(axis);
    CHECK_APPROX_EQ(normalize(*axis), expected_axis);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
