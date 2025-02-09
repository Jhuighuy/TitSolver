/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <array>
#include <unordered_set>

#include "tit/core/basic_types.hpp"
#include "tit/core/rand_utils.hpp"

#include "tit/testing/test.hpp"

namespace tit {
namespace {

#define INT_TYPES int, unsigned int, long, unsigned long

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("SplitMix64") {
  // Disclaimer: Since SplitMix64 algorithm was not designed by us,
  // we only test the basic properties of the generator.
  SplitMix64 rng{/*seed=*/123};
  std::unordered_set<uint64_t> random_values{};
  for (size_t sample = 0; sample < 100; ++sample) {
    const auto random_value = rng();
    REQUIRE(random_value >= SplitMix64::min());
    REQUIRE(random_value <= SplitMix64::max());
    CHECK_FALSE(random_values.contains(random_value));
    random_values.insert(random_value);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE_TEMPLATE("less_or", Int, INT_TYPES) {
  SUBCASE("deterministic") {
    for (const auto b : {true, false}) {
      // Non-equal operands.
      CHECK(less_or(Int{3}, Int{5}, b));
      CHECK_FALSE(less_or(Int{5}, Int{3}, b));
      // Equal operands. Result is determined by the third argument.
      CHECK(less_or(Int{5}, Int{5}, b) == b);
    }
  }
  SUBCASE("random") {
    SplitMix64 rng{/*seed=*/123};
    // Non-equal operands. Result is known.
    CHECK(less_or(Int{3}, Int{5}, rng));
    CHECK_FALSE(less_or(Int{5}, Int{3}, rng));
    // Equal operands. Result is random.
    std::array<size_t, 2> results{};
    for (size_t sample = 0; sample < 10; ++sample) {
      results[less_or(Int{5}, Int{5}, rng)] += 1;
    }
    CHECK(results[false] > 0);
    CHECK(results[true] > 0);
  }
}

TEST_CASE_TEMPLATE("greater_or", Int, INT_TYPES) {
  SUBCASE("deterministic") {
    for (const auto b : {true, false}) {
      // Non-equal operands.
      CHECK(greater_or(Int{5}, Int{3}, b));
      CHECK_FALSE(greater_or(Int{3}, Int{5}, b));
      // Equal operands. Result is determined by the third argument.
      CHECK(greater_or(Int{5}, Int{5}, b) == b);
    }
  }
  SUBCASE("random") {
    SplitMix64 rng{/*seed=*/123};
    // Non-equal operands. Result is known.
    CHECK(greater_or(Int{5}, Int{3}, rng));
    CHECK_FALSE(greater_or(Int{3}, Int{5}, rng));
    // Equal operands. Result is random.
    std::array<size_t, 2> results{};
    for (size_t sample = 0; sample < 10; ++sample) {
      results[greater_or(Int{5}, Int{5}, rng)] += 1;
    }
    CHECK(results[false] > 0);
    CHECK(results[true] > 0);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("randomized_hash") {
  // Ensure randomized hash is deterministic.
  CHECK(randomized_hash(1) == randomized_hash(1));
  // Ensure randomized hash is not affected by the order of the arguments.
  CHECK(randomized_hash(1, 2, 3) == randomized_hash(3, 2, 1));
  CHECK(randomized_hash(1, 2, 3) == randomized_hash(2, 1, 3));
  // Ensure randomized hash is different for different arguments.
  CHECK(randomized_hash(1, 2, 3) != randomized_hash(5, 6, 7));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
