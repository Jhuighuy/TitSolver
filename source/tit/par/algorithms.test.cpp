/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <algorithm>
#include <functional>
#include <random>
#include <ranges>
#include <stdexcept>
#include <vector>

#include "tit/par/algorithms.hpp"
#include "tit/par/control.hpp"
#include "tit/testing/test.hpp"
#include "tit/testing/utils.hpp"

namespace tit {
namespace {

// Disclaimer: Since this submodule is no more that a simple wrapper around the
// Intel TBB library, there is no need to test it in detail. The only thing we
// need to test is that our wrappers are working correctly.

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Thread error.
class ThreadError final : public std::runtime_error {
public:

  // Construct the thread error.
  ThreadError() : std::runtime_error{"thread error"} {}

}; // class ThreadError

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("par::for_each") {
  par::set_num_threads(4);
  std::vector data{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  SUBCASE("basic") {
    // Ensure the loop is executed.
    par::for_each(data, SleepFunc{[](int& i) { i += 1; }});
    CHECK(data == std::vector{1, 2, 3, 4, 5, 6, 7, 8, 9, 10});
  }
  SUBCASE("exceptions") {
    // Ensure the exceptions from the worker threads are caught.
    CHECK_THROWS_AS(par::for_each(data, SleepFunc{[](int i) {
                                    if (i == 7) throw ThreadError{};
                                  }}),
                    ThreadError);
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
    par::block_for_each(data, SleepFunc{[](int& i) { i += 1; }});
    CHECK(data == VectorOfVectors{{1, 2}, {3, 4}, {5, 6}, {7, 8}, {9, 10}});
  }
  SUBCASE("exceptions") {
    // Ensure the exceptions from the worker threads are caught.
    CHECK_THROWS_AS(par::block_for_each(data, SleepFunc{[](int i) {
                                          if (i == 7) throw ThreadError{};
                                        }}),
                    ThreadError);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("par::fold") {
  par::set_num_threads(4);
  const std::vector data{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  SUBCASE("basic") {
    // Ensure the loop is executed.
    const auto result = par::fold(data, 0, SleepFunc{std::plus{}}, std::plus{});
    CHECK(result == 45);
  }
  SUBCASE("exceptions") {
    // Ensure the exceptions from the worker threads are caught.
    CHECK_THROWS_AS(par::fold(data,
                              0,
                              SleepFunc{[](int partial, int i) {
                                if (i == 7) throw ThreadError{};
                                return partial + i;
                              }},
                              std::plus{}),
                    ThreadError);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("par::unstable_copy_if") {
  par::set_num_threads(4);
  const std::vector data{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  std::vector<int> out(data.size());
  SUBCASE("basic") {
    // Ensure the loop is executed.
    const auto iter = par::unstable_copy_if( //
        data,
        out.begin(),
        SleepFunc{[](int i) { return i % 2 == 0; }});
    CHECK(iter == out.begin() + 5);
    const auto out_range = std::ranges::subrange(out.begin(), iter);
    std::ranges::sort(out_range);
    CHECK_RANGE_EQ(out_range, {0, 2, 4, 6, 8});
  }
  SUBCASE("exceptions") {
    // Ensure the exceptions from the worker threads are caught.
    CHECK_THROWS_AS(
        par::unstable_copy_if(data, out.begin(), SleepFunc{[](int i) {
                                if (i == 7) throw ThreadError{};
                                return i % 2 == 0;
                              }}),
        ThreadError);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("par::transform") {
  par::set_num_threads(4);
  const std::vector data{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  std::vector<int> out(data.size());
  SUBCASE("basic") {
    // Ensure the loop is executed.
    const auto iter = par::transform( //
        data,
        out.begin(),
        SleepFunc{[](int i) { return 2 * i + 1; }});
    CHECK(iter == out.end());
    CHECK_RANGE_EQ(out, {1, 3, 5, 7, 9, 11, 13, 15, 17, 19});
  }
  SUBCASE("exceptions") {
    // Ensure the exceptions from the worker threads are caught.
    CHECK_THROWS_AS(par::transform(data, out.begin(), SleepFunc{[](int i) {
                                     if (i == 7) throw ThreadError{};
                                     return 2 * i + 1;
                                   }}),
                    ThreadError);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("par::sort") {
  par::set_num_threads(4);
  constexpr auto sorted = std::views::iota(0, 1000);
  auto data = sorted | std::ranges::to<std::vector>();
  std::ranges::shuffle(data, std::mt19937{123});
  SUBCASE("basic") {
    // Ensure the loop is executed.
    par::sort(data);
    CHECK_RANGE_EQ(data, sorted);
  }
  SUBCASE("exceptions") {
    // Ensure the exceptions from the worker threads are caught.
    CHECK_THROWS_AS(par::sort(data,
                              [](int a, int b) {
                                if (a == 123) throw ThreadError{};
                                return a < b;
                              }),
                    ThreadError);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
