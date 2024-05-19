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
  const FloatMaskArray in{false, true, false, true};
  const FloatRegMask r = !FloatRegMask(in);
  FloatMaskArray out{};
  r.store(out);
  CHECK(out == FloatMaskArray{true, false, true, false});
}

TEST_CASE("simd::RegMask::operator&&") {
  const FloatMaskArray a{true, false, true, false};
  const FloatMaskArray b{true, true, false, false};
  const FloatRegMask ra(a);
  const FloatRegMask rb(b);
  const FloatRegMask r = ra && rb;
  FloatMaskArray out{};
  r.store(out);
  CHECK(out == FloatMaskArray{true, false, false, false});
}

TEST_CASE("simd::RegMask::operator||") {
  const FloatMaskArray a{true, false, true, false};
  const FloatMaskArray b{true, true, false, false};
  const FloatRegMask ra(a);
  const FloatRegMask rb(b);
  const FloatRegMask r = ra || rb;
  FloatMaskArray out{};
  r.store(out);
  CHECK(out == FloatMaskArray{true, true, true, false});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("simd::RegMask::operator==") {
  const FloatMaskArray a{true, false, true, false};
  const FloatMaskArray b{true, true, false, false};
  const FloatRegMask ra(a);
  const FloatRegMask rb(b);
  const FloatRegMask r = ra == rb;
  FloatMaskArray out{};
  r.store(out);
  CHECK(out == FloatMaskArray{true, false, false, true});
}

TEST_CASE("simd::RegMask::operator!=") {
  const FloatMaskArray a{true, false, true, false};
  const FloatMaskArray b{true, true, false, false};
  const FloatRegMask ra(a);
  const FloatRegMask rb(b);
  const FloatRegMask r = ra != rb;
  FloatMaskArray out{};
  r.store(out);
  CHECK(out == FloatMaskArray{false, true, true, false});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("simd::RegMask::any_and_all") {
  SUBCASE("all") {
    const FloatMaskArray a{true, true, true, true};
    const FloatRegMask ra(a);
    CHECK(any(ra));
    CHECK(all(ra));
  }
  SUBCASE("some") {
    const FloatMaskArray a{true, false, true, false};
    const FloatRegMask ra(a);
    CHECK(any(ra));
    CHECK_FALSE(all(ra));
  }
  SUBCASE("none") {
    const FloatMaskArray a{false, false, false, false};
    const FloatRegMask ra(a);
    CHECK_FALSE(any(ra));
    CHECK_FALSE(all(ra));
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
