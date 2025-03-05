/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <chrono>
#include <ranges>
#include <thread>

#include "tit/core/par/algorithms.hpp"
#include "tit/core/par/atomic.hpp"
#include "tit/core/par/control.hpp"

#include "tit/testing/test.hpp"

namespace tit {
namespace {

// Disclaimer: Since this submodule is no more that a simple wrapper around the
// GCC atomics intrinsics, there is no need to test it in detail. The only thing
// we need to test is that our wrappers are working correctly.

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("par::load") {
  const auto val = 10;
  CHECK(par::load(val) == val);
}

TEST_CASE("par::store") {
  auto val = 10;
  constexpr auto desired = 20;
  par::store(val, desired);
  CHECK(val == desired);
}

TEST_CASE("par::wait") {
  static constexpr auto init = 10;
  static constexpr auto updated = 200;
  auto val = init;
  par::set_num_threads(4);
  par::for_each(std::views::iota(0, 4), [&val](int i) {
    if (i == 2) {
      std::this_thread::sleep_for(std::chrono::milliseconds{10});
      par::store(val, updated);
    } else {
      CHECK(par::wait(val, init) == updated);
    }
  });
  CHECK(val == updated);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("par::compare_exchange") {
  constexpr auto expected = 10;
  constexpr auto desired = 20;
  SUBCASE("success") {
    auto val = expected;
    CHECK(par::compare_exchange(val, expected, desired));
    CHECK(val == desired);
  }
  SUBCASE("failure") {
    constexpr auto unexpected = 30;
    auto val = unexpected;
    CHECK_FALSE(par::compare_exchange(val, expected, desired));
    CHECK(val == unexpected);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("par::fetch_and_add") {
  constexpr auto init = 10;
  constexpr auto delta = 20;
  auto val = init;
  // Ensure we are getting back the original value.
  CHECK(par::fetch_and_add(val, delta) == init);
  // Ensure that the value was updated correctly.
  CHECK(val == init + delta);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
