/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <algorithm>
#include <array>
#include <ranges>

#include "tit/core/containers/mdvector.hpp"

#include "tit/testing/test.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("Mdspan") {
  const std::array shape{3ZU, 3ZU};
  const std::array vals{1, 2, 3, 4, 5, 6, 7, 8, 9};
  const Mdspan mdspan{shape, vals};
  SUBCASE("access") {
    CHECK(mdspan.size() == 9);
    CHECK(mdspan.front() == 1);
    CHECK(mdspan.back() == 9);
    CHECK(mdspan[0, 0] == 1);
    CHECK(mdspan[0, 1] == 2);
    CHECK(mdspan[std::array{1, 0}] == 4);
    CHECK(mdspan[2, 1] == 8);
  }
  SUBCASE("subspans") {
    CHECK(mdspan[1].size() == 3);
    CHECK(mdspan[1].front() == 4);
    CHECK(mdspan[1].back() == 6);
    CHECK(mdspan[0][0] == 1);
    CHECK(mdspan[0][1] == 2);
    CHECK(mdspan[1][0] == 4);
    CHECK(mdspan[2][1] == 8);
  }
  SUBCASE("iterators") {
    const auto iter = std::ranges::find(mdspan, 7);
    CHECK(iter - mdspan.begin() == 6);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("Mdvector") {
  SUBCASE("shape") {
    // Construct `Mdvector` and check it's size.
    Mdvector<int, 2> mdvector(3, 3);
    CHECK(mdvector.size() == 9);

    // Assign different shape to it and check it's size.
    mdvector.assign(2, 4);
    CHECK(mdvector.size() == 8);

    // Clear the vector and check if it is size again.
    mdvector.clear();
    CHECK(mdvector.size() == 0);
  }
  SUBCASE("access") {
    const auto make_mdvector = []() {
      // Construct Mdvector.
      Mdvector<int, 2> mdvector(3, 3);

      // Populate it with vals using the different accessors.
      mdvector.front() = 1, mdvector[0, 1] = 2, mdvector[0][2] = 2;
      mdvector[1, 0] = 4, mdvector[1][1] = 5, mdvector[1, 2] = 6;
      mdvector[2][0] = 9, mdvector[std::array{2, 1}] = 8, mdvector.back() = 9;
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
  SUBCASE("iterators") {
    const auto make_mdvector = []() {
      // Construct and populate `Mdvector` using iterators.
      Mdvector<int, 3> mdvector{};
      mdvector.assign(4, 4, 4);
      std::ranges::copy(std::views::iota(1, 65), mdvector.begin());
      std::ranges::reverse(mdvector);
      return mdvector;
    };

    // Retrieve const copy of our vector to play with.
    const auto mdvector = make_mdvector();

    // Find `17` in vector.
    const auto iter = std::ranges::find(mdvector, 17);
    CHECK(iter - mdvector.begin() == (65 - 17 - 1));
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
