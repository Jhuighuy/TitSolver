/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <algorithm>
#include <ranges>
#include <utility>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/containers/multivector.hpp"

#include "tit/testing/test.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("Multivector") {
  SUBCASE("empty") {
    const Multivector<int> multivector{};
    CHECK(multivector.size() == 0);
    CHECK(multivector.empty());
    CHECK_RANGE_EMPTY(multivector.bucket_sizes());
    CHECK_RANGE_EMPTY(multivector.buckets());
  }
  SUBCASE("from initial values") {
    const Multivector multivector{{1, 2, 3, 4}, {5, 6, 7}, {8, 9}};
    CHECK(multivector.size() == 3);
    CHECK_FALSE(multivector.empty());
    CHECK_RANGE_EQ(multivector.bucket_sizes(), std::vector{4, 3, 2});
    CHECK_RANGE_EQ(multivector.buckets() | std::views::join,
                   std::vector{1, 2, 3, 4, 5, 6, 7, 8, 9});
    CHECK_RANGE_EQ(multivector[0], std::vector{1, 2, 3, 4});
    CHECK_RANGE_EQ(multivector[1], std::vector{5, 6, 7});
    CHECK_RANGE_EQ(multivector[2], std::vector{8, 9});
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("Multivector::clear") {
  Multivector multivector{{1, 2, 3, 4}, {5, 6, 7}, {8, 9}};
  REQUIRE(multivector.size() == 3);
  multivector.clear();
  CHECK(multivector.empty());
}

TEST_CASE("Multivector::append_bucket") {
  // Populate a multivector with buckets.
  const std::vector<std::vector<int>> buckets{
      {1, 2, 3, 4},
      {5, 6, 7},
      {8, 9},
  };
  Multivector<int> multivector{};
  for (const auto& bucket : buckets) multivector.append_bucket(bucket);

  // Ensure the multivector is correct.
  REQUIRE(multivector.size() == buckets.size());
  for (const auto& [bucket, expected] :
       std::views::zip(multivector.buckets(), buckets)) {
    CHECK_RANGE_EQ(bucket, expected);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("Multivector::assign_pairs_seq") {
  // Build a multivector from sequence of pairs.
  const std::vector<std::pair<size_t, int>> pairs{
      {0, 1},
      {0, 2},
      {0, 3},
      {0, 4},
      {1, 5},
      {1, 6},
      {1, 7},
      {2, 8},
      {2, 9},
  };
  Multivector<int> multivector{};
  multivector.assign_pairs_seq(3, pairs);

  // Ensure the multivector is correct.
  REQUIRE(multivector.size() == 3);
  CHECK_RANGE_EQ(multivector[0], std::vector{1, 2, 3, 4});
  CHECK_RANGE_EQ(multivector[1], std::vector{5, 6, 7});
  CHECK_RANGE_EQ(multivector[2], std::vector{8, 9});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("Multivector::assign_buckets_par") {
  // Build a multivector from a sequence of buckets.
  const std::vector<std::vector<int>> buckets{
      {1, 2},
      {3, 4},
      {5, 6},
      {7, 8},
      {9},
  };
  Multivector<int> multivector{};
  multivector.assign_buckets_par(buckets);

  // Ensure the multivector is correct.
  REQUIRE(multivector.size() == buckets.size());
  for (const auto& [bucket, expected] :
       std::views::zip(multivector.buckets(), buckets)) {
    CHECK_RANGE_EQ(bucket, expected);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("Multivector::assign_pairs_par_tall") {
  // Build a multivector from a sequence of pairs.
  const std::vector<std::pair<size_t, int>> pairs{
      {0, 1},
      {2, 8},
      {0, 2},
      {0, 4},
      {1, 5},
      {1, 6},
      {0, 3},
      {1, 7},
      {2, 9},
  };
  Multivector<int> multivector{};
  multivector.assign_pairs_par_tall(3, pairs);

  // Sort the buckets, since parallel algorithms does not guarantee order.
  std::ranges::for_each(multivector.buckets(), std::ranges::sort);

  // Ensure the multivector is correct.
  REQUIRE(multivector.size() == 3);
  CHECK_RANGE_EQ(multivector[0], std::vector{1, 2, 3, 4});
  CHECK_RANGE_EQ(multivector[1], std::vector{5, 6, 7});
  CHECK_RANGE_EQ(multivector[2], std::vector{8, 9});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("Multivector::assign_pairs_par_wide") {
  // Build a multivector from a sequence of pairs.
  const std::vector<std::pair<size_t, int>> pairs{
      {0, 1},
      {2, 8},
      {0, 2},
      {0, 4},
      {1, 5},
      {1, 6},
      {0, 3},
      {1, 7},
      {2, 9},
  };
  Multivector<int> multivector{};
  multivector.assign_pairs_par_wide(3, pairs);

  // Sort the buckets, since parallel algorithms does not guarantee order.
  std::ranges::for_each(multivector.buckets(), std::ranges::sort);

  // Ensure the multivector is correct.
  REQUIRE(multivector.size() == 3);
  CHECK_RANGE_EQ(multivector[0], std::vector{1, 2, 3, 4});
  CHECK_RANGE_EQ(multivector[1], std::vector{5, 6, 7});
  CHECK_RANGE_EQ(multivector[2], std::vector{8, 9});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
