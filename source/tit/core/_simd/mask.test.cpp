/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "tit/core/simd.hpp"

#include "tit/testing/test.hpp"

namespace tit {
namespace {

using FloatMask = simd::Mask<float>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("simd::Mask") {
  static_assert(sizeof(FloatMask) == sizeof(float));
  SUBCASE("zero initialization") {
    const FloatMask m{};
    CHECK_FALSE(m);
  }
  SUBCASE("value initialization") {
    const FloatMask m{true};
    CHECK(m);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("simd::Mask::operator!") {
  CHECK(!FloatMask{false});
  CHECK_FALSE(!FloatMask{true});
}

TEST_CASE("simd::Mask::operator&&") {
  CHECK_FALSE((FloatMask{false} && FloatMask{false}));
  CHECK_FALSE((FloatMask{false} && FloatMask{true}));
  CHECK_FALSE((FloatMask{true} && FloatMask{false}));
  CHECK((FloatMask{true} && FloatMask{true}));
}

TEST_CASE("simd::Mask::operator||") {
  CHECK_FALSE((FloatMask{false} || FloatMask{false}));
  CHECK((FloatMask{false} || FloatMask{true}));
  CHECK((FloatMask{true} || FloatMask{false}));
  CHECK((FloatMask{true} || FloatMask{true}));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
