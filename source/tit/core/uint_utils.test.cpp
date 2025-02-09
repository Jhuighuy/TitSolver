/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "tit/core/uint_utils.hpp"

#include "tit/testing/test.hpp"

namespace tit {
namespace {

#define UINT_TYPES unsigned int, unsigned long

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE_TEMPLATE("divide_up", UInt, UINT_TYPES) {
  CHECK(divide_up(UInt{0}, UInt{10}) == UInt{0});
  CHECK(divide_up(UInt{3}, UInt{10}) == UInt{1});
  CHECK(divide_up(UInt{7}, UInt{10}) == UInt{1});
  CHECK(divide_up(UInt{10}, UInt{10}) == UInt{1});
  CHECK(divide_up(UInt{11}, UInt{10}) == UInt{2});
  CHECK(divide_up(UInt{20}, UInt{10}) == UInt{2});
}

TEST_CASE_TEMPLATE("align_up", UInt, UINT_TYPES) {
  CHECK(align_up(UInt{0}, UInt{10}) == UInt{0});
  CHECK(align_up(UInt{3}, UInt{10}) == UInt{10});
  CHECK(align_up(UInt{7}, UInt{10}) == UInt{10});
  CHECK(align_up(UInt{10}, UInt{10}) == UInt{10});
  CHECK(align_up(UInt{11}, UInt{10}) == UInt{20});
  CHECK(align_up(UInt{20}, UInt{10}) == UInt{20});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
