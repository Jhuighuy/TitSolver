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
using FloatArray = std::array<float, 4>;
using FloatReg = simd::Reg<float, 4>;
using FloatMask = simd::Mask<float>;
using FloatMaskArray = std::array<FloatMask, 4>;
using FloatRegMask = simd::RegMask<float, 4>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("simd::Reg") {
  SUBCASE("load and store") {
    const FloatArray in{1.0F, 2.0F, 3.0F, 4.0F};
    const FloatReg r(in);
    FloatArray out{};
    r.store(out);
    CHECK(in == out);
  }
  SUBCASE("zero initialization") {
    const FloatReg r{};
    FloatArray out{1.0F};
    r.store(out);
    for (const auto& x : out) CHECK(x == 0.0F);
  }
  SUBCASE("value initialization") {
    const auto val = 1.3F;
    const FloatReg r(val);
    FloatArray out{};
    r.store(out);
    for (const auto& x : out) CHECK(x == val);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("simd::Reg::min") {
  const FloatArray a{5.0F, 6.0F, 7.0F, 8.0F};
  const FloatArray b{1.0F, 7.0F, 4.0F, 9.0F};
  const FloatReg r = simd::min(FloatReg(a), FloatReg(b));
  FloatArray out{};
  r.store(out);
  CHECK(out == FloatArray{1.0F, 6.0F, 4.0F, 8.0F});
}

TEST_CASE("simd::Reg::max") {
  const FloatArray a{5.0F, 6.0F, 7.0F, 8.0F};
  const FloatArray b{1.0F, 7.0F, 4.0F, 9.0F};
  const FloatReg r = simd::max(FloatReg(a), FloatReg(b));
  FloatArray out{};
  r.store(out);
  CHECK(out == FloatArray{5.0F, 7.0F, 7.0F, 9.0F});
}

TEST_CASE("simd::Reg::filter") {
  const FloatArray a{5.0F, 6.0F, 7.0F, 8.0F};
  const FloatMaskArray mask{true, false, true, false};
  const FloatReg r = simd::filter(FloatRegMask(mask), FloatReg(a));
  FloatArray out{};
  r.store(out);
  CHECK(out == FloatArray{5.0F, 0.0F, 7.0F, 0.0F});
}

TEST_CASE("simd::Reg::select") {
  const FloatArray a{5.0F, 6.0F, 7.0F, 8.0F};
  const FloatArray b{1.0F, 2.0F, 3.0F, 4.0F};
  const FloatMaskArray mask{true, false, true, false};
  const FloatReg r = simd::select(FloatRegMask(mask), FloatReg(a), FloatReg(b));
  FloatArray out{};
  r.store(out);
  CHECK(out == FloatArray{5.0F, 2.0F, 7.0F, 4.0F});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("simd::Reg::operator+") {
  const FloatArray a{1.0F, 2.0F, 3.0F, 4.0F};
  const FloatArray b{5.0F, 6.0F, 7.0F, 8.0F};
  const FloatArray sum{6.0F, 8.0F, 10.0F, 12.0F};
  SUBCASE("normal") {
    const FloatReg r = FloatReg(a) + FloatReg(b);
    FloatArray out{};
    r.store(out);
    CHECK(out == sum);
  }
  SUBCASE("with assignment") {
    FloatReg r(a);
    r += FloatReg(b);
    FloatArray out{};
    r.store(out);
    CHECK(out == sum);
  }
}

TEST_CASE("simd::Reg::operator-") {
  const FloatArray b{1.0F, 2.0F, 3.0F, 4.0F};
  SUBCASE("negation") {
    const FloatReg r = -FloatReg(b);
    FloatArray out{};
    r.store(out);
    CHECK(out == FloatArray{-1.0F, -2.0F, -3.0F, -4.0F});
  }
  const FloatArray a{5.0F, 6.0F, 7.0F, 8.0F};
  const FloatArray diff{4.0F, 4.0F, 4.0F, 4.0F};
  SUBCASE("normal") {
    const FloatReg r = FloatReg(a) - FloatReg(b);
    FloatArray out{};
    r.store(out);
    CHECK(out == diff);
  }
  SUBCASE("with assignment") {
    FloatReg r(a);
    r -= FloatReg(b);
    FloatArray out{};
    r.store(out);
    CHECK(out == diff);
  }
}

TEST_CASE("simd::Reg::operator*") {
  const FloatArray a{2.0F, 3.0F, 4.0F, 5.0F};
  const FloatArray b{6.0F, 7.0F, 8.0F, 9.0F};
  const FloatArray prod{12.0F, 21.0F, 32.0F, 45.0F};
  SUBCASE("normal") {
    const FloatReg r = FloatReg(a) * FloatReg(b);
    FloatArray out{};
    r.store(out);
    CHECK(out == prod);
  }
  SUBCASE("with assignment") {
    FloatReg r(a);
    r *= FloatReg(b);
    FloatArray out{};
    r.store(out);
    CHECK(out == prod);
  }
}

TEST_CASE("simd::Reg::operator/") {
  const FloatArray a{12.0F, 21.0F, 32.0F, 45.0F};
  const FloatArray b{6.0F, 7.0F, 8.0F, 9.0F};
  const FloatArray quot{2.0F, 3.0F, 4.0F, 5.0F};
  SUBCASE("normal") {
    const FloatReg r = FloatReg(a) / FloatReg(b);
    FloatArray out{};
    r.store(out);
    CHECK(out == quot);
  }
  SUBCASE("with assignment") {
    FloatReg r(a);
    r /= FloatReg(b);
    FloatArray out{};
    r.store(out);
    CHECK(out == quot);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("simd::Reg::floor") {
  const FloatArray a{1.5F, 2.7F, 3.1F, 4.9F};
  const FloatReg r = simd::floor(FloatReg(a));
  FloatArray out{};
  r.store(out);
  CHECK(out == FloatArray{1.0F, 2.0F, 3.0F, 4.0F});
}

TEST_CASE("simd::Reg::round") {
  const FloatArray a{1.5F, 2.7F, 3.1F, 4.9F};
  const FloatReg r = simd::round(FloatReg(a));
  FloatArray out{};
  r.store(out);
  CHECK(out == FloatArray{2.0F, 3.0F, 3.0F, 5.0F});
}

TEST_CASE("simd::Reg::ceil") {
  const FloatArray a{1.5F, 2.7F, 3.1F, 4.9F};
  const FloatReg r = simd::ceil(FloatReg(a));
  FloatArray out{};
  r.store(out);
  CHECK(out == FloatArray{2.0F, 3.0F, 4.0F, 5.0F});
}

TEST_CASE("simd::Reg::fma") {
  const FloatArray a{1.0F, 2.0F, 3.0F, 4.0F};
  const FloatArray b{5.0F, 6.0F, 7.0F, 8.0F};
  const FloatArray c{9.0F, 10.0F, 11.0F, 12.0F};
  const FloatReg r = simd::fma(FloatReg(a), FloatReg(b), FloatReg(c));
  FloatArray out{};
  r.store(out);
  CHECK(out == FloatArray{14.0F, 22.0F, 32.0F, 44.0F});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("simd::Reg::operator<=>") {
  const FloatArray a{1.0F, 2.0F, 4.0F, 4.0F};
  const FloatArray b{1.0F, 5.0F, 3.0F, 7.0F};
  SUBCASE("==") {
    const auto m = FloatReg(a) == FloatReg(b);
    FloatMaskArray out{};
    m.store(out);
    CHECK(out == FloatMaskArray{true, false, false, false});
  }
  SUBCASE("!=") {
    const auto m = FloatReg(a) != FloatReg(b);
    FloatMaskArray out{};
    m.store(out);
    CHECK(out == FloatMaskArray{false, true, true, true});
  }
  SUBCASE("<") {
    const auto m = FloatReg(a) < FloatReg(b);
    FloatMaskArray out{};
    m.store(out);
    CHECK(out == FloatMaskArray{false, true, false, true});
  }
  SUBCASE("<=") {
    const auto m = FloatReg(a) <= FloatReg(b);
    FloatMaskArray out{};
    m.store(out);
    CHECK(out == FloatMaskArray{true, true, false, true});
  }
  SUBCASE(">") {
    const auto m = FloatReg(a) > FloatReg(b);
    FloatMaskArray out{};
    m.store(out);
    CHECK(out == FloatMaskArray{false, false, true, false});
  }
  SUBCASE(">=") {
    const auto m = FloatReg(a) >= FloatReg(b);
    FloatMaskArray out{};
    m.store(out);
    CHECK(out == FloatMaskArray{true, false, true, false});
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("simd::Reg::sum") {
  const FloatArray a{1.0F, 2.0F, 3.0F, 4.0F};
  CHECK(simd::sum(FloatReg(a)) == 10.0F);
}

TEST_CASE("simd::Reg::min_value") {
  const FloatArray a{3.0F, 2.0F, 4.0F, 1.0F};
  CHECK(simd::min_value(FloatReg(a)) == 1.0F);
}

TEST_CASE("simd::Reg::max_value") {
  const FloatArray a{3.0F, 2.0F, 4.0F, 1.0F};
  CHECK(simd::max_value(FloatReg(a)) == 4.0F);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
