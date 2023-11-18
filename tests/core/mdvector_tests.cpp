/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License
 * See /LICENSE.md for license information.
 * SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <algorithm>
#include <array>
#include <functional> // for what reason, IWYU?
#include <ranges>

#include <doctest/doctest.h>

#include "tit/core/mdvector.hpp"

namespace {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

TEST_CASE("tit::core::Mdspan") {
  // Construct `Mdspan`.
  const auto values = std::array{1, 2, 3, //
                                 4, 5, 6, //
                                 7, 8, 9};
  const auto shape = std::array{3UZ, 3UZ};
  const auto mdspan = tit::Mdspan<const int, 2>{values.data(), shape.data()};
  // Check data access.
  CHECK(mdspan.size() == 9);
  CHECK(mdspan.front() == 1);
  CHECK(mdspan.back() == 9);
  CHECK(mdspan[0, 0] == 1);
  CHECK(mdspan[0, 1] == 2);
  CHECK(mdspan[1, 0] == 4);
  CHECK(mdspan[2, 1] == 8);
  // Check data access using subspans.
  CHECK(mdspan[1].size() == 3);
  CHECK(mdspan[1].front() == 4);
  CHECK(mdspan[1].back() == 6);
  CHECK(mdspan[0][0] == 1);
  CHECK(mdspan[0][1] == 2);
  CHECK(mdspan[1][0] == 4);
  CHECK(mdspan[2][1] == 8);
  // Check data access using iterators.
  const auto iter = std::ranges::find(mdspan, 7);
  CHECK(iter - mdspan.begin() == 6);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

TEST_CASE("tit::core::Mdvector") {
  SUBCASE("shape") {
    // Construct `Mdvector` and check it's size.
    auto mdvector = tit::Mdvector<int, 2>(3, 3);
    CHECK(mdvector.size() == 9);
    // Assign different shape to it and check it's size.
    mdvector.assign(2, 4);
    CHECK(mdvector.size() == 8);
    // Clear the vector and check if it is size again.
    mdvector.clear();
    CHECK(mdvector.size() == 0);
  }

  /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

  SUBCASE("access") {
    const auto make_mdvector = []() {
      // Construct Mdvector.
      auto mdvector = tit::Mdvector<int, 2>(3UZ, 3UZ);
      // Populate it with values using the different accessors.
      mdvector.front() = 1, mdvector[0, 1] = 2, mdvector[0][2] = 2;
      mdvector[1, 0] = 4, mdvector[1][1] = 5, mdvector[1, 2] = 6;
      mdvector[2][0] = 9, mdvector[2, 1] = 8, mdvector.back() = 9;
      return mdvector;
    };
    // Retrieve const copy of our vector to play with.
    const auto mdvector = make_mdvector();
    // Check basic accessors.
    CHECK(mdvector.front() == 1);
    CHECK(mdvector.back() == 9);
    // Check data access.
    CHECK(mdvector[0, 0] == 1);
    CHECK(mdvector[0, 1] == 2);
    CHECK(mdvector[1, 0] == 4);
    CHECK(mdvector[2, 1] == 8);
    // Check data access via subspans.
    CHECK(mdvector[1].size() == 3);
    CHECK(mdvector[1].front() == 4);
    CHECK(mdvector[1].back() == 6);
    CHECK(mdvector[0][0] == 1);
    CHECK(mdvector[0][1] == 2);
    CHECK(mdvector[1][0] == 4);
    CHECK(mdvector[2][1] == 8);
  }

  /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

  SUBCASE("iterators") {
    const auto make_mdvector = []() {
      // Construct and empty `Mdvector`.
      auto mdvector = tit::Mdvector<int, 3>{};
      // Assign shape to it.
      mdvector.assign(4, 4, 4);
      // Populate it using iterators.
      std::ranges::copy(std::views::iota(1, 65), mdvector.begin());
      return mdvector;
    };
    // Retrieve const copy of our vector to play with.
    const auto mdvector = make_mdvector();
    // Find `32` in vector.
    const auto iter = std::ranges::find(mdvector, 32);
    CHECK(iter - mdvector.begin() == 31);
  }
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace
