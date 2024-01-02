/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// TODO: This is needed for doctest to properly detect the platform.
// Remove once fixed.
#include <cstdlib> // IWYU pragma: keep

#include <doctest/doctest.h>

#include "tit/core/uint_utils.hpp"

namespace tit {
namespace {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

// Unsigned integer types to test against.
#define UINT_TYPES unsigned int, unsigned long

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

TEST_CASE_TEMPLATE("tit::divide_up", UInt, UINT_TYPES) {
  CHECK(divide_up(UInt{0}, UInt{10}) == UInt{0});
  CHECK(divide_up(UInt{3}, UInt{10}) == UInt{1});
  CHECK(divide_up(UInt{7}, UInt{10}) == UInt{1});
  CHECK(divide_up(UInt{10}, UInt{10}) == UInt{1});
  CHECK(divide_up(UInt{11}, UInt{10}) == UInt{2});
  CHECK(divide_up(UInt{20}, UInt{10}) == UInt{2});
}

TEST_CASE_TEMPLATE("tit::align_up", UInt, UINT_TYPES) {
  CHECK(align_up(UInt{0}, UInt{10}) == UInt{0});
  CHECK(align_up(UInt{3}, UInt{10}) == UInt{10});
  CHECK(align_up(UInt{7}, UInt{10}) == UInt{10});
  CHECK(align_up(UInt{10}, UInt{10}) == UInt{10});
  CHECK(align_up(UInt{11}, UInt{10}) == UInt{20});
  CHECK(align_up(UInt{20}, UInt{10}) == UInt{20});
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

TEST_CASE_TEMPLATE("tit::is_power_of_two", UInt, UINT_TYPES) {
  CHECK(is_power_of_two(UInt{1}));
  CHECK(is_power_of_two(UInt{512}));
  CHECK(!is_power_of_two(UInt{255}));
  CHECK(!is_power_of_two(UInt{513}));
}

TEST_CASE_TEMPLATE("tit::align_up_to_power_of_two", UInt, UINT_TYPES) {
  CHECK(align_up_to_power_of_two(UInt{1}) == UInt{1});
  CHECK(align_up_to_power_of_two(UInt{2}) == UInt{2});
  CHECK(align_up_to_power_of_two(UInt{3}) == UInt{4});
  CHECK(align_up_to_power_of_two(UInt{5}) == UInt{8});
  CHECK(align_up_to_power_of_two(UInt{127}) == UInt{128});
  CHECK(align_up_to_power_of_two(UInt{128}) == UInt{128});
  CHECK(align_up_to_power_of_two(UInt{129}) == UInt{256});
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace
} // namespace tit
