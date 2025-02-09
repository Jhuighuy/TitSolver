/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <array>

#include "tit/core/basic_types.hpp"
#include "tit/core/vec.hpp"

#include "tit/geom/bbox.hpp"
#include "tit/geom/grid.hpp"

#include "tit/testing/test.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::Grid") {
  SUBCASE("zero initialization") {
    const geom::Grid<Vec<double, 2>> grid{};
    CHECK(grid.box().low() == Vec{0.0, 0.0});
    CHECK(grid.box().high() == Vec{0.0, 0.0});
    CHECK(grid.num_cells() == Vec{0UZ, 0UZ});
    CHECK(grid.flat_num_cells() == 0);
    CHECK(grid.cell_extents() == Vec{0.0, 0.0});
  }
  SUBCASE("from box") {
    const geom::BBox box{Vec{0.0, 0.0}, Vec{2.0, 2.0}};
    const geom::Grid grid{box, {2, 1}};
    CHECK(grid.box().low() == Vec{0.0, 0.0});
    CHECK(grid.box().high() == Vec{2.0, 2.0});
    CHECK(grid.num_cells() == Vec{2UZ, 1UZ});
    CHECK(grid.flat_num_cells() == 2);
    CHECK(grid.cell_extents() == Vec{1.0, 2.0});
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::Grid::num_cells") {
  // Initialize the grid and check the initial values.
  const geom::BBox box{Vec{0.0, 0.0}, Vec{8.0, 8.0}};
  geom::Grid grid{box, {2, 4}};
  CHECK(grid.num_cells() == Vec{2UZ, 4UZ});
  CHECK(grid.flat_num_cells() == 8);
  CHECK(grid.cell_extents() == Vec{4.0, 2.0});

  // Update the number of cells and check the new values.
  grid.set_num_cells({4, 2});
  CHECK(grid.num_cells() == Vec{4UZ, 2UZ});
  CHECK(grid.flat_num_cells() == 8);
  CHECK(grid.cell_extents() == Vec{2.0, 4.0});

  // Extend the number of cells and check the new values.
  grid.extend(2);
  CHECK(grid.num_cells() == Vec{8UZ, 6UZ});
  CHECK(grid.flat_num_cells() == 48);
  CHECK(grid.cell_extents() == Vec{2.0, 4.0});
  CHECK(grid.box().low() == Vec{-4.0, -8.0});
  CHECK(grid.box().high() == Vec{12.0, 16.0});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::Grid::cell_extents") {
  SUBCASE("divides") {
    const geom::BBox box{Vec{0.0, 0.0}, Vec{8.0, 8.0}};
    const auto grid = geom::Grid{box}.set_cell_extents({4.0, 2.0});
    CHECK(grid.cell_extents() == Vec{4.0, 2.0});
    CHECK(grid.num_cells() == Vec{2UZ, 4UZ});
  }
  SUBCASE("does not divide") {
    const geom::BBox box{Vec{0.0, 0.0}, Vec{8.0, 8.0}};
    const auto grid = geom::Grid{box}.set_cell_extents({5.0, 2.5});
    CHECK(grid.cell_extents() == Vec{4.0, 2.0});
    CHECK(grid.num_cells() == Vec{2UZ, 4UZ});
  }
  SUBCASE("too large") {
    const geom::BBox box{Vec{0.0, 0.0}, Vec{4.0, 4.0}};
    const auto grid = geom::Grid{box}.set_cell_extents({5.0, 7.0});
    CHECK(grid.cell_extents() == Vec{4.0, 4.0});
    CHECK(grid.num_cells() == Vec{1UZ, 1UZ});
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::Grid::cell_index") {
  const geom::BBox box{Vec{0.0, 0.0}, Vec{4.0, 4.0}};
  const geom::Grid grid{box, {2, 2}};

  CHECK(grid.cell_index({0.0, 0.0}) == Vec{0UZ, 0UZ});
  CHECK(grid.flat_cell_index({0.0, 0.0}) == 0);
  CHECK(grid.flatten_cell_index({0, 0}) == 0);

  CHECK(grid.cell_index({1.0, 1.0}) == Vec{0UZ, 0UZ});
  CHECK(grid.flat_cell_index({1.0, 1.0}) == 0);
  CHECK(grid.flatten_cell_index({0, 0}) == 0);

  CHECK(grid.cell_index({2.0, 1.0}) == Vec{1UZ, 0UZ});
  CHECK(grid.flat_cell_index({2.0, 1.0}) == 2);
  CHECK(grid.flatten_cell_index({1, 0}) == 2);

  CHECK(grid.cell_index({1.0, 2.0}) == Vec{0UZ, 1UZ});
  CHECK(grid.flat_cell_index({1.0, 2.0}) == 1);
  CHECK(grid.flatten_cell_index({0, 1}) == 1);

  CHECK(grid.cell_index({2.0, 2.0}) == Vec{1UZ, 1UZ});
  CHECK(grid.flat_cell_index({2.0, 2.0}) == 3);
  CHECK(grid.flatten_cell_index({1, 1}) == 3);

  CHECK(grid.cell_index({3.0, 3.0}) == Vec{1UZ, 1UZ});
  CHECK(grid.flat_cell_index({3.0, 3.0}) == 3);
  CHECK(grid.flatten_cell_index({1, 1}) == 3);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("geom::Grid::cells") {
  SUBCASE("all") {
    const geom::BBox box{Vec{0.0, 0.0}, Vec{3.0, 3.0}};
    const geom::Grid grid{box, {3, 3}};
    constexpr auto expected_cells = std::to_array<Vec<size_t, 2>>({
        {0, 0},
        {0, 1},
        {0, 2},
        {1, 0},
        {1, 1},
        {1, 2},
        {2, 0},
        {2, 1},
        {2, 2},
    });
    CHECK_RANGE_EQ(grid.cells(), expected_cells);
  }
  SUBCASE("all(n)") {
    const geom::BBox box{Vec{0.0, 0.0}, Vec{4.0, 4.0}};
    const geom::Grid grid{box, {4, 4}};
    constexpr auto expected_cells = std::to_array<Vec<size_t, 2>>({
        {1, 1},
        {1, 2},
        {2, 1},
        {2, 2},
    });
    CHECK_RANGE_EQ(grid.cells(1), expected_cells);
  }
  SUBCASE("range") {
    const geom::BBox box{Vec{0.0, 0.0}, Vec{8.0, 8.0}};
    const geom::Grid grid{box, {8, 8}};
    SUBCASE("exclusive") {
      constexpr auto expected_cells = std::to_array<Vec<size_t, 2>>({
          {1, 1},
          {1, 2},
          {2, 1},
          {2, 2},
      });
      CHECK_RANGE_EQ(grid.cells({1, 1}, {3, 3}), expected_cells);
    }
    SUBCASE("inclusive") {
      constexpr auto expected_cells = std::to_array<Vec<size_t, 2>>({
          {1, 1},
          {1, 2},
          {1, 3},
          {2, 1},
          {2, 2},
          {2, 3},
          {3, 1},
          {3, 2},
          {3, 3},
      });
      CHECK_RANGE_EQ(grid.cells_inclusive({1, 1}, {3, 3}), expected_cells);
    }
  }
  SUBCASE("intersecting") {
    const geom::BBox box{Vec{0.0, 0.0}, Vec{8.0, 8.0}};
    const geom::Grid grid{box, {8, 8}};
    SUBCASE("full intersection") {
      const geom::BBox search_box{Vec{3.0, 3.0}, Vec{5.0, 5.0}};
      constexpr auto expected_cells = std::to_array<Vec<size_t, 2>>({
          {3, 3},
          {3, 4},
          {3, 5},
          {4, 3},
          {4, 4},
          {4, 5},
          {5, 3},
          {5, 4},
          {5, 5},
      });
      CHECK_RANGE_EQ(grid.cells_intersecting(search_box), expected_cells);
    }
    SUBCASE("partial intersection") {
      const geom::BBox search_box{Vec{6.0, 6.0}, Vec{9.0, 9.0}};
      constexpr auto expected_cells = std::to_array<Vec<size_t, 2>>({
          {6, 6},
          {6, 7},
          {7, 6},
          {7, 7},
      });
      CHECK_RANGE_EQ(grid.cells_intersecting(search_box), expected_cells);
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
