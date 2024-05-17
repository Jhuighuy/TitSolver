/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <array>

#include "tit/core/simd.hpp"

#include "tit/testing/test.hpp"

namespace tit {
namespace {

// 128-bit floating point SIMD appears to be supported on all platforms.
using FloatMask = simd::Mask<float>;
using FloatMaskArray = std::array<FloatMask, 4>;
using FloatRegMask = simd::RegMask<float, 4>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("simd::RegMask") {
  SUBCASE("load and store") {
    FloatMaskArray const in{false, true, true, false};
    FloatRegMask const r(in);
    FloatMaskArray out{};
    r.store(out);
    CHECK(in == out);
  }
  SUBCASE("zero initialization") {
    FloatRegMask const r{};
    FloatMaskArray out{true};
    r.store(out);
    for (auto const& x : out) CHECK_FALSE(x);
  }
  SUBCASE("value initialization") {
    FloatRegMask const r(true);
    FloatMaskArray out{};
    r.store(out);
    for (auto const& x : out) CHECK(x);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("simd::RegMask::any_and_all") {
  SUBCASE("all") {
    FloatMaskArray const a{true, true, true, true};
    FloatRegMask const ra(a);
    CHECK(any(ra));
    CHECK(all(ra));
  }
  SUBCASE("some") {
    FloatMaskArray const a{true, false, true, false};
    FloatRegMask const ra(a);
    CHECK(any(ra));
    CHECK_FALSE(all(ra));
  }
  SUBCASE("none") {
    FloatMaskArray const a{false, false, false, false};
    FloatRegMask const ra(a);
    CHECK_FALSE(any(ra));
    CHECK_FALSE(all(ra));
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
