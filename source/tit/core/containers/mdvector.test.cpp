/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <array>

#include "tit/core/basic_types.hpp"
#include "tit/core/containers/mdvector.hpp"
#include "tit/testing/test.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("Mdspan") {
  SUBCASE("construction") {
    SUBCASE("from shape and values") {
      const std::array vals{1, 2, 3, 4, 5, 6, 7, 8, 9};
      const auto shape = std::to_array<size_t>({3, 3});
      const Mdspan mdspan{vals.begin(), shape};
      CHECK(mdspan.size() == 9);
      CHECK(mdspan.data() == vals.data());
      CHECK_RANGE_EQ(mdspan, vals);
      CHECK_RANGE_EQ(mdspan.shape(), shape);
    }
  }
  SUBCASE("operator[]") {
    const std::array vals{1, 2, 3, 4, 5, 6, 7, 8};
    const auto shape = std::to_array<size_t>({2, 2, 2});
    const Mdspan mdspan{vals.begin(), shape};
    CHECK(mdspan[{0, 1, 0}] == 3);
    CHECK(mdspan[{1, 0, 1}] == 6);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("Mdvector") {
  SUBCASE("construction") {
    SUBCASE("empty") {
      const Mdvector<int, 2> mdvector;
      CHECK(mdvector.size() == 0);
      CHECK_RANGE_EQ(mdvector.shape(), std::array<size_t, 2>{});
      CHECK(mdvector.data() == nullptr);
    }
    SUBCASE("from shape") {
      const Mdvector<int, 2> mdvector({4, 2});
      CHECK(mdvector.size() == 8);
      CHECK(mdvector.data() != nullptr);
      CHECK_RANGE_EQ(mdvector.shape(), {4, 2});
    }
  }
  SUBCASE("operator[]") {
    const std::array vals{1, 2, 3, 4, 5, 6, 7, 8};
    const Mdvector<int, 3> mdvector{vals.begin(), {2, 2, 2}};
    CHECK(mdvector[{0, 1, 0}] == 3);
    CHECK(mdvector[{1, 0, 1}] == 6);
  }
  SUBCASE("methods") {
    const std::array vals{1, 2, 3, 4, 5, 6, 7, 8};
    Mdvector<int, 3> mdvector{vals.begin(), {2, 2, 2}};
    SUBCASE("clear") {
      mdvector.clear();
      CHECK(mdvector.size() == 0);
      CHECK(mdvector.data() != nullptr); // no deallocation should happen.
      CHECK_RANGE_EQ(mdvector.shape(), std::array<size_t, 3>{});
    }
    SUBCASE("assign") {
      auto* const old_data = mdvector.data();
      SUBCASE("same size") {
        mdvector.assign({2, 1, 4});
        CHECK(mdvector.size() == 8);
        CHECK(mdvector.data() == old_data); // no reallocation should happen.
        CHECK_RANGE_EQ(mdvector, std::array<int, 8>{});
        CHECK_RANGE_EQ(mdvector.shape(), {2, 1, 4});
      }
      SUBCASE("smaller size") {
        mdvector.assign({2, 1, 2});
        CHECK(mdvector.size() == 4);
        CHECK(mdvector.data() == old_data); // no reallocation should happen.
        CHECK_RANGE_EQ(mdvector, std::array<int, 4>{});
        CHECK_RANGE_EQ(mdvector.shape(), {2, 1, 2});
      }
      SUBCASE("larger size") {
        mdvector.assign({2, 2, 4});
        CHECK(mdvector.size() == 16);
        CHECK(mdvector.data() != nullptr);
        CHECK(mdvector.data() != old_data);
        CHECK_RANGE_EQ(mdvector, std::array<int, 16>{});
        CHECK_RANGE_EQ(mdvector.shape(), {2, 2, 4});
      }
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
