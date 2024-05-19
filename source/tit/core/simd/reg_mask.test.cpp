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
    const FloatMaskArray in{false, true, true, false};
    const FloatRegMask r(in);
    FloatMaskArray out{};
    r.store(out);
    CHECK(in == out);
  }
  SUBCASE("zero initialization") {
    const FloatRegMask r{};
    FloatMaskArray out{true};
    r.store(out);
    for (const auto& x : out) CHECK_FALSE(x);
  }
  SUBCASE("value initialization") {
    const FloatRegMask r(true);
    FloatMaskArray out{};
    r.store(out);
    for (const auto& x : out) CHECK(x);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("simd::RegMask::operator!") {
  const auto r = !FloatRegMask(FloatMaskArray{false, true, false, true});
  FloatMaskArray out{};
  r.store(out);
  CHECK(out == FloatMaskArray{true, false, true, false});
}

TEST_CASE("simd::RegMask::operator&&") {
  const auto r = FloatRegMask{FloatMaskArray{true, false, true, false}} &&
                 FloatRegMask{FloatMaskArray{true, true, false, false}};
  FloatMaskArray out{};
  r.store(out);
  CHECK(out == FloatMaskArray{true, false, false, false});
}

TEST_CASE("simd::RegMask::operator||") {
  const auto r = FloatRegMask{FloatMaskArray{true, false, true, false}} ||
                 FloatRegMask{FloatMaskArray{true, true, false, false}};
  FloatMaskArray out{};
  r.store(out);
  CHECK(out == FloatMaskArray{true, true, true, false});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("simd::RegMask::operator==") {
  const FloatMaskArray m{true, false, true, false};
  const FloatMaskArray n{true, true, false, false};
  SUBCASE("==") {
    const auto r = FloatRegMask{m} == FloatRegMask{n};
    FloatMaskArray out{};
    r.store(out);
    CHECK(out == FloatMaskArray{true, false, false, true});
  }
  SUBCASE("!=") {
    const auto r = FloatRegMask{m} != FloatRegMask{n};
    FloatMaskArray out{};
    r.store(out);
    CHECK(out == FloatMaskArray{false, true, true, false});
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("simd::RegMask::any_and_all") {
  SUBCASE("all") {
    const FloatRegMask m{FloatMaskArray{true, true, true, true}};
    CHECK(any(m));
    CHECK(all(m));
  }
  SUBCASE("some") {
    const FloatRegMask m{FloatMaskArray{true, false, true, false}};
    CHECK(any(m));
    CHECK_FALSE(all(m));
  }
  SUBCASE("none") {
    const FloatRegMask m{FloatMaskArray{false, false, false, false}};
    CHECK_FALSE(any(m));
    CHECK_FALSE(all(m));
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
