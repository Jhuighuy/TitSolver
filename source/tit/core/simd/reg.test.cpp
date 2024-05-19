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
  const auto r = simd::min(FloatReg{FloatArray{5.0F, 6.0F, 7.0F, 8.0F}},
                           FloatReg{FloatArray{1.0F, 7.0F, 4.0F, 9.0F}});
  FloatArray out{};
  r.store(out);
  CHECK(out == FloatArray{1.0F, 6.0F, 4.0F, 8.0F});
}

TEST_CASE("simd::Reg::max") {
  const auto r = simd::max(FloatReg{FloatArray{5.0F, 6.0F, 7.0F, 8.0F}},
                           FloatReg{FloatArray{1.0F, 7.0F, 4.0F, 9.0F}});
  FloatArray out{};
  r.store(out);
  CHECK(out == FloatArray{5.0F, 7.0F, 7.0F, 9.0F});
}

TEST_CASE("simd::Reg::filter") {
  const auto r =
      simd::filter(FloatRegMask{FloatMaskArray{true, false, true, false}},
                   FloatReg{FloatArray{5.0F, 6.0F, 7.0F, 8.0F}});
  FloatArray out{};
  r.store(out);
  CHECK(out == FloatArray{5.0F, 0.0F, 7.0F, 0.0F});
}

TEST_CASE("simd::Reg::select") {
  const auto r =
      simd::select(FloatRegMask{FloatMaskArray{true, false, true, false}},
                   FloatReg{FloatArray{5.0F, 6.0F, 7.0F, 8.0F}},
                   FloatReg{FloatArray{1.0F, 2.0F, 3.0F, 4.0F}});
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
    const FloatReg r = FloatReg{a} + FloatReg{b};
    FloatArray out{};
    r.store(out);
    CHECK(out == sum);
  }
  SUBCASE("with assignment") {
    FloatReg r{a};
    r += FloatReg{b};
    FloatArray out{};
    r.store(out);
    CHECK(out == sum);
  }
}

TEST_CASE("simd::Reg::operator-") {
  const FloatArray b{1.0F, 2.0F, 3.0F, 4.0F};
  SUBCASE("negation") {
    const FloatReg r = -FloatReg{b};
    FloatArray out{};
    r.store(out);
    CHECK(out == FloatArray{-1.0F, -2.0F, -3.0F, -4.0F});
  }
  const FloatArray a{5.0F, 6.0F, 7.0F, 8.0F};
  const FloatArray diff{4.0F, 4.0F, 4.0F, 4.0F};
  SUBCASE("normal") {
    const FloatReg r = FloatReg{a} - FloatReg{b};
    FloatArray out{};
    r.store(out);
    CHECK(out == diff);
  }
  SUBCASE("with assignment") {
    FloatReg r{a};
    r -= FloatReg{b};
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
    const FloatReg r = FloatReg{a} * FloatReg{b};
    FloatArray out{};
    r.store(out);
    CHECK(out == prod);
  }
  SUBCASE("with assignment") {
    FloatReg r{a};
    r *= FloatReg{b};
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
    const FloatReg r = FloatReg{a} / FloatReg{b};
    FloatArray out{};
    r.store(out);
    CHECK(out == quot);
  }
  SUBCASE("with assignment") {
    FloatReg r{a};
    r /= FloatReg{b};
    FloatArray out{};
    r.store(out);
    CHECK(out == quot);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("simd::Reg::floor") {
  const auto r = simd::floor(FloatReg{FloatArray{1.5F, 2.7F, 3.1F, 4.9F}});
  FloatArray out{};
  r.store(out);
  CHECK(out == FloatArray{1.0F, 2.0F, 3.0F, 4.0F});
}

TEST_CASE("simd::Reg::round") {
  const auto r = simd::round(FloatReg{FloatArray{1.5F, 2.7F, 3.1F, 4.9F}});
  FloatArray out{};
  r.store(out);
  CHECK(out == FloatArray{2.0F, 3.0F, 3.0F, 5.0F});
}

TEST_CASE("simd::Reg::ceil") {
  const auto r = simd::ceil(FloatReg{FloatArray{1.5F, 2.7F, 3.1F, 4.9F}});
  FloatArray out{};
  r.store(out);
  CHECK(out == FloatArray{2.0F, 3.0F, 4.0F, 5.0F});
}

TEST_CASE("simd::Reg::fma") {
  const auto r = simd::fma(FloatReg{FloatArray{1.0F, 2.0F, 3.0F, 4.0F}},
                           FloatReg{FloatArray{5.0F, 6.0F, 7.0F, 8.0F}},
                           FloatReg{FloatArray{9.0F, 10.0F, 11.0F, 12.0F}});
  FloatArray out{};
  r.store(out);
  CHECK(out == FloatArray{14.0F, 22.0F, 32.0F, 44.0F});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("simd::Reg::operator<=>") {
  const FloatArray a{1.0F, 2.0F, 4.0F, 4.0F};
  const FloatArray b{1.0F, 5.0F, 3.0F, 7.0F};
  SUBCASE("==") {
    const auto m = FloatReg{a} == FloatReg{b};
    FloatMaskArray out{};
    m.store(out);
    CHECK(out == FloatMaskArray{true, false, false, false});
  }
  SUBCASE("!=") {
    const auto m = FloatReg{a} != FloatReg{b};
    FloatMaskArray out{};
    m.store(out);
    CHECK(out == FloatMaskArray{false, true, true, true});
  }
  SUBCASE("<") {
    const auto m = FloatReg{a} < FloatReg{b};
    FloatMaskArray out{};
    m.store(out);
    CHECK(out == FloatMaskArray{false, true, false, true});
  }
  SUBCASE("<=") {
    const auto m = FloatReg{a} <= FloatReg{b};
    FloatMaskArray out{};
    m.store(out);
    CHECK(out == FloatMaskArray{true, true, false, true});
  }
  SUBCASE(">") {
    const auto m = FloatReg{a} > FloatReg{b};
    FloatMaskArray out{};
    m.store(out);
    CHECK(out == FloatMaskArray{false, false, true, false});
  }
  SUBCASE(">=") {
    const auto m = FloatReg{a} >= FloatReg{b};
    FloatMaskArray out{};
    m.store(out);
    CHECK(out == FloatMaskArray{true, false, true, false});
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("simd::Reg::sum") {
  CHECK(simd::sum(FloatReg{FloatArray{1.0F, 2.0F, 3.0F, 4.0F}}) == 10.0F);
}

TEST_CASE("simd::Reg::min_value") {
  CHECK(simd::min_value(FloatReg{FloatArray{3.0F, 2.0F, 4.0F, 1.0F}}) == 1.0F);
}

TEST_CASE("simd::Reg::max_value") {
  CHECK(simd::max_value(FloatReg{FloatArray{3.0F, 2.0F, 4.0F, 1.0F}}) == 4.0F);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
