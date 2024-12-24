/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <ranges>
#include <utility>
#include <vector>

#include "tit/core/par/algorithms.hpp"
#include "tit/core/par/control.hpp"

#include "tit/testing/test.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("par::impl::unview") {
  // We'll use `par::fold` to test the unviewing.
  par::set_num_threads(4);
  using VectorOfVectors = std::vector<std::vector<int>>;
  SUBCASE("basic") {
    // Nothing is unviewed.
    const std::vector data{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    CHECK(par::fold(data) == 45);
  }
  SUBCASE("join") {
    const VectorOfVectors data{{0, 1, 2}, {3, 4, 5}, {6, 7, 8}, {9}};
    CHECK(par::fold(data | std::views::join) == 45);
  }
  SUBCASE("filter") {
    const std::vector data{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    CHECK(par::fold(par::static_, data | std::views::filter([](int i) {
                                    return i % 2 == 0;
                                  })) == 20);
  }
  SUBCASE("join | transform") {
    const VectorOfVectors data{{0, 1, 2}, {3, 4, 5}, {6, 7, 8}, {9}};
    CHECK(par::fold(data | std::views::join |
                    std::views::transform([](int i) { return 2 * i; })) == 90);
  }
  SUBCASE("join | transform | elements") {
    const VectorOfVectors data{{0, 1, 2}, {3, 4, 5}, {6, 7, 8}, {9}};
    CHECK(par::fold(data | std::views::join | std::views::transform([](int i) {
                      return std::pair{i, 2 * i};
                    }) |
                    std::views::elements<1>) == 90);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
