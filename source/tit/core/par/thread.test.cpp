/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <ranges>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/par.hpp"

#include "tit/testing/test.hpp"

namespace tit {
namespace {

// Disclaimer: Since this submodule is no more that a simple wrapper around the
// Intel TBB library, there is no need to test it in detail. The only thing we
// need to test is that our wrappers are working correctly.

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("par::for_each") {
  par::set_num_threads(4);
  std::vector<int> data{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  SUBCASE("basic") {
    // Ensure the loop is executed.
    par::for_each(data, [](int& i) { i += 1; });
    CHECK(data == std::vector{1, 2, 3, 4, 5, 6, 7, 8, 9, 10});
  }
  SUBCASE("exceptions") {
    // Ensure the exceptions from the worker threads are caught.
    CHECK_THROWS_WITH_AS(
        [&data] {
          par::for_each(data, [](int /*i*/) {
            throw std::runtime_error{"Loop failed!"};
          });
          FAIL("Loop should have thrown an exception!");
        }(),
        "Loop failed!",
        std::runtime_error);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("par::static_for_each") {
  par::set_num_threads(4);
  std::vector<int> data{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  std::vector<size_t> indices(data.size());
  SUBCASE("basic") {
    // Ensure the loop is executed and the thread distribution is correct.
    par::static_for_each( //
        std::views::zip(data, indices),
        [](size_t thread_index, auto pair) {
          auto& [i, ti] = pair;
          ti = thread_index, i += 1;
        });
    CHECK(data == std::vector{1, 2, 3, 4, 5, 6, 7, 8, 9, 10});
    CHECK(indices == std::vector<size_t>{0, 0, 0, 1, 1, 1, 2, 2, 3, 3});
  }
  SUBCASE("exceptions") {
    // Ensure the exceptions from worker threads are caught.
    CHECK_THROWS_WITH_AS(
        [&data] {
          par::static_for_each(data, [](size_t /*thread_index*/, int /*i*/) {
            throw std::runtime_error{"Loop failed!"};
          });
          FAIL("Loop should have thrown an exception!");
        }(),
        "Loop failed!",
        std::runtime_error);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("par::block_for_each") {
  par::set_num_threads(4);
  using VectorOfVectors = std::vector<std::vector<int>>;
  VectorOfVectors data{{0, 1}, {2, 3}, {4, 5}, {6, 7}, {8, 9}};
  SUBCASE("basic") {
    // Ensure the loop is executed.
    /// @todo This test does not really test that the iterations are done in
    /// in chunks, it is hard to test that without exposing the internals.
    par::block_for_each(data, [](int& i) { i += 1; });
    CHECK(data == VectorOfVectors{{1, 2}, {3, 4}, {5, 6}, {7, 8}, {9, 10}});
  }
  SUBCASE("exceptions") {
    // Ensure the exceptions from the worker threads are caught.
    CHECK_THROWS_WITH_AS(
        [&data] {
          par::block_for_each(data, [](int /*i*/) {
            throw std::runtime_error{"Loop failed!"};
          });
          FAIL("Loop should have thrown an exception!");
        }(),
        "Loop failed!",
        std::runtime_error);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
