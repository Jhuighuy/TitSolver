/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <algorithm>
#include <cstddef>
#include <vector>

#include "tit/core/vec.hpp"
#include "tit/geom/shape.hpp"
#include "tit/geom/shape/query.hpp"
#include "tit/testing/test.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto sorted(std::vector<std::size_t> v) -> std::vector<std::size_t> {
  std::ranges::sort(v);
  return v;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::ShapeQuery::query") {
  SUBCASE("empty shape") {
    SUBCASE("2D") {
      const geom::Shape<Vec<double, 2>> shape{};
      const geom::ShapeQuery range{shape};
      CHECK(range.query(Vec{0.0, 0.0}, 1.0).empty());
      CHECK(range.query(Vec{1.0, 1.0}, 0.0).empty());
    }
    SUBCASE("3D") {
      const geom::Shape<Vec<double, 3>> shape{};
      const geom::ShapeQuery range{shape};
      CHECK(range.query(Vec{0.0, 0.0, 0.0}, 1.0).empty());
      CHECK(range.query(Vec{1.0, 1.0, 1.0}, 0.0).empty());
    }
  }

  SUBCASE("2D: single segment") {
    geom::Shape<Vec<double, 2>> shape{};
    shape.append_vert(Vec{0.0, 0.0});
    shape.append_vert(Vec{2.0, 0.0});
    shape.append_face({0, 1});

    const geom::ShapeQuery range{shape};

    SUBCASE("sphere far away") {
      CHECK(range.query(Vec{5.0, 5.0}, 0.5).empty());
    }
    SUBCASE("sphere at midpoint") {
      const auto result = range.query(Vec{1.0, 0.0}, 0.5);
      const auto expected = std::vector<std::size_t>{0};
      CHECK(sorted(result) == expected);
    }
    SUBCASE("sphere touches endpoint") {
      const auto result = range.query(Vec{0.0, 0.0}, 0.5);
      const auto expected = std::vector<std::size_t>{0};
      CHECK(sorted(result) == expected);
    }
    SUBCASE("sphere contains whole segment") {
      const auto result = range.query(Vec{1.0, 0.0}, 5.0);
      const auto expected = std::vector<std::size_t>{0};
      CHECK(sorted(result) == expected);
    }
  }

  SUBCASE("2D: two segments, partial intersection") {
    geom::Shape<Vec<double, 2>> shape{};
    shape.append_vert(Vec{0.0, 0.0});
    shape.append_vert(Vec{1.0, 0.0});
    shape.append_vert(Vec{3.0, 0.0});
    shape.append_vert(Vec{4.0, 0.0});
    // Face 0: segment from (0,0) to (1,0)
    // Face 1: segment from (3,0) to (4,0)
    shape.append_face({0, 1});
    shape.append_face({2, 3});

    const geom::ShapeQuery range{shape};

    SUBCASE("sphere near first segment only") {
      const auto result = range.query(Vec{0.5, 0.0}, 0.5);
      const auto expected = std::vector<std::size_t>{0};
      CHECK(sorted(result) == expected);
    }
    SUBCASE("sphere near second segment only") {
      const auto result = range.query(Vec{3.5, 0.0}, 0.5);
      const auto expected = std::vector<std::size_t>{1};
      CHECK(sorted(result) == expected);
    }
    SUBCASE("sphere covers both segments") {
      const auto result = range.query(Vec{2.0, 0.0}, 3.0);
      const auto expected = std::vector<std::size_t>{0, 1};
      CHECK(sorted(result) == expected);
    }
  }

  SUBCASE("3D: single triangle") {
    geom::Shape<Vec<double, 3>> shape{};
    // Triangle in the XY plane at Z=0.
    shape.append_vert(Vec{0.0, 0.0, 0.0});
    shape.append_vert(Vec{2.0, 0.0, 0.0});
    shape.append_vert(Vec{0.0, 2.0, 0.0});
    shape.append_face({0, 1, 2});

    const geom::ShapeQuery range{shape};

    SUBCASE("sphere far away") {
      CHECK(range.query(Vec{10.0, 10.0, 10.0}, 0.5).empty());
    }
    SUBCASE("sphere at vertex") {
      const auto result = range.query(Vec{0.0, 0.0, 0.0}, 0.5);
      const auto expected = std::vector<std::size_t>{0};
      CHECK(sorted(result) == expected);
    }
    SUBCASE("sphere on edge midpoint") {
      const auto result = range.query(Vec{1.0, 0.0, 0.0}, 0.3);
      const auto expected = std::vector<std::size_t>{0};
      CHECK(sorted(result) == expected);
    }
    SUBCASE("sphere projected inside triangle, near plane") {
      const auto result = range.query(Vec{0.5, 0.5, 0.2}, 0.3);
      const auto expected = std::vector<std::size_t>{0};
      CHECK(sorted(result) == expected);
    }
    SUBCASE("sphere projected inside triangle, too far") {
      CHECK(range.query(Vec{0.5, 0.5, 5.0}, 0.5).empty());
    }
    SUBCASE("sphere projected outside triangle") {
      CHECK(range.query(Vec{3.0, 3.0, 0.1}, 0.5).empty());
    }
  }

  SUBCASE("3D: multiple triangles, partial intersection") {
    geom::Shape<Vec<double, 3>> shape{};
    // Triangle 0: XY plane at Z=0.
    shape.append_vert(Vec{0.0, 0.0, 0.0});
    shape.append_vert(Vec{1.0, 0.0, 0.0});
    shape.append_vert(Vec{0.0, 1.0, 0.0});
    // Triangle 1: XY plane at Z=5.
    shape.append_vert(Vec{0.0, 0.0, 5.0});
    shape.append_vert(Vec{1.0, 0.0, 5.0});
    shape.append_vert(Vec{0.0, 1.0, 5.0});
    shape.append_face({0, 1, 2});
    shape.append_face({3, 4, 5});

    const geom::ShapeQuery range{shape};

    SUBCASE("sphere near first triangle only") {
      const auto result = range.query(Vec{0.2, 0.2, 0.0}, 0.5);
      const auto expected = std::vector<std::size_t>{0};
      CHECK(sorted(result) == expected);
    }
    SUBCASE("sphere near second triangle only") {
      const auto result = range.query(Vec{0.2, 0.2, 5.0}, 0.5);
      const auto expected = std::vector<std::size_t>{1};
      CHECK(sorted(result) == expected);
    }
    SUBCASE("sphere large enough to cover both") {
      const auto result = range.query(Vec{0.0, 0.0, 2.5}, 3.0);
      const auto expected = std::vector<std::size_t>{0, 1};
      CHECK(sorted(result) == expected);
    }
  }

  SUBCASE("3D: degenerate triangle") {
    geom::Shape<Vec<double, 3>> shape{};
    // Degenerate triangle (collinear points).
    shape.append_vert(Vec{0.0, 0.0, 0.0});
    shape.append_vert(Vec{1.0, 0.0, 0.0});
    shape.append_vert(Vec{2.0, 0.0, 0.0});
    shape.append_face({0, 1, 2});

    const geom::ShapeQuery range{shape};

    // Degenerate triangle has zero area, so it's effectively just segments.
    // The sphere at the midpoint of the middle segment should still intersect.
    const auto result = range.query(Vec{1.0, 0.0, 0.0}, 0.3);
    const auto expected = std::vector<std::size_t>{0};
    CHECK(sorted(result) == expected);

    // Sphere above, not near any vertex or edge.
    CHECK(range.query(Vec{0.5, 5.0, 0.0}, 0.1).empty());
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
