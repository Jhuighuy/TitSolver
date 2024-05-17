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
    FloatArray const in{1.0F, 2.0F, 3.0F, 4.0F};
    FloatReg const r(in);
    FloatArray out{};
    r.store(out);
    CHECK(in == out);
  }
  SUBCASE("zero initialization") {
    FloatReg const r{};
    FloatArray out{1.0F};
    r.store(out);
    for (auto const& x : out) CHECK(x == 0.0F);
  }
  SUBCASE("value initialization") {
    auto const val = 1.3F;
    FloatReg const r(val);
    FloatArray out{};
    r.store(out);
    for (auto const& x : out) CHECK(x == val);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("simd::Reg::min") {
  FloatArray const a{5.0F, 6.0F, 7.0F, 8.0F};
  FloatArray const b{1.0F, 7.0F, 4.0F, 9.0F};
  FloatReg const r = simd::min(FloatReg(a), FloatReg(b));
  FloatArray out{};
  r.store(out);
  CHECK(out == FloatArray{1.0F, 6.0F, 4.0F, 8.0F});
}

TEST_CASE("simd::Reg::max") {
  FloatArray const a{5.0F, 6.0F, 7.0F, 8.0F};
  FloatArray const b{1.0F, 7.0F, 4.0F, 9.0F};
  FloatReg const r = simd::max(FloatReg(a), FloatReg(b));
  FloatArray out{};
  r.store(out);
  CHECK(out == FloatArray{5.0F, 7.0F, 7.0F, 9.0F});
}

TEST_CASE("simd::Reg::filter") {
  FloatArray const a{5.0F, 6.0F, 7.0F, 8.0F};
  FloatMaskArray const mask{true, false, true, false};
  FloatReg const r = simd::filter(FloatRegMask(mask), FloatReg(a));
  FloatArray out{};
  r.store(out);
  CHECK(out == FloatArray{5.0F, 0.0F, 7.0F, 0.0F});
}

TEST_CASE("simd::Reg::select") {
  FloatArray const a{5.0F, 6.0F, 7.0F, 8.0F};
  FloatArray const b{1.0F, 2.0F, 3.0F, 4.0F};
  FloatMaskArray const mask{true, false, true, false};
  FloatReg const r = simd::select(FloatRegMask(mask), FloatReg(a), FloatReg(b));
  FloatArray out{};
  r.store(out);
  CHECK(out == FloatArray{5.0F, 2.0F, 7.0F, 4.0F});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("simd::Reg::operator+") {
  FloatArray const a{1.0F, 2.0F, 3.0F, 4.0F};
  FloatArray const b{5.0F, 6.0F, 7.0F, 8.0F};
  FloatArray const sum{6.0F, 8.0F, 10.0F, 12.0F};
  SUBCASE("normal") {
    FloatReg const r = FloatReg(a) + FloatReg(b);
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
  FloatArray const b{1.0F, 2.0F, 3.0F, 4.0F};
  SUBCASE("negation") {
    FloatReg const r = -FloatReg(b);
    FloatArray out{};
    r.store(out);
    CHECK(out == FloatArray{-1.0F, -2.0F, -3.0F, -4.0F});
  }
  FloatArray const a{5.0F, 6.0F, 7.0F, 8.0F};
  FloatArray const diff{4.0F, 4.0F, 4.0F, 4.0F};
  SUBCASE("normal") {
    FloatReg const r = FloatReg(a) - FloatReg(b);
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
  FloatArray const a{2.0F, 3.0F, 4.0F, 5.0F};
  FloatArray const b{6.0F, 7.0F, 8.0F, 9.0F};
  FloatArray const prod{12.0F, 21.0F, 32.0F, 45.0F};
  SUBCASE("normal") {
    FloatReg const r = FloatReg(a) * FloatReg(b);
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
  FloatArray const a{12.0F, 21.0F, 32.0F, 45.0F};
  FloatArray const b{6.0F, 7.0F, 8.0F, 9.0F};
  FloatArray const quot{2.0F, 3.0F, 4.0F, 5.0F};
  SUBCASE("normal") {
    FloatReg const r = FloatReg(a) / FloatReg(b);
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
  FloatArray const a{1.5F, 2.7F, 3.1F, 4.9F};
  FloatReg const r = simd::floor(FloatReg(a));
  FloatArray out{};
  r.store(out);
  CHECK(out == FloatArray{1.0F, 2.0F, 3.0F, 4.0F});
}

TEST_CASE("simd::Reg::round") {
  FloatArray const a{1.5F, 2.7F, 3.1F, 4.9F};
  FloatReg const r = simd::round(FloatReg(a));
  FloatArray out{};
  r.store(out);
  CHECK(out == FloatArray{2.0F, 3.0F, 3.0F, 5.0F});
}

TEST_CASE("simd::Reg::ceil") {
  FloatArray const a{1.5F, 2.7F, 3.1F, 4.9F};
  FloatReg const r = simd::ceil(FloatReg(a));
  FloatArray out{};
  r.store(out);
  CHECK(out == FloatArray{2.0F, 3.0F, 4.0F, 5.0F});
}

TEST_CASE("simd::Reg::fma") {
  FloatArray const a{1.0F, 2.0F, 3.0F, 4.0F};
  FloatArray const b{5.0F, 6.0F, 7.0F, 8.0F};
  FloatArray const c{9.0F, 10.0F, 11.0F, 12.0F};
  FloatReg const r = simd::fma(FloatReg(a), FloatReg(b), FloatReg(c));
  FloatArray out{};
  r.store(out);
  CHECK(out == FloatArray{14.0F, 22.0F, 32.0F, 44.0F});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("simd::Reg::operator<=>") {
  FloatArray const a{1.0F, 2.0F, 4.0F, 4.0F};
  FloatArray const b{1.0F, 5.0F, 3.0F, 7.0F};
  SUBCASE("==") {
    auto const m = FloatReg(a) == FloatReg(b);
    FloatMaskArray out{};
    m.store(out);
    CHECK(out == FloatMaskArray{true, false, false, false});
  }
  SUBCASE("!=") {
    auto const m = FloatReg(a) != FloatReg(b);
    FloatMaskArray out{};
    m.store(out);
    CHECK(out == FloatMaskArray{false, true, true, true});
  }
  SUBCASE("<") {
    auto const m = FloatReg(a) < FloatReg(b);
    FloatMaskArray out{};
    m.store(out);
    CHECK(out == FloatMaskArray{false, true, false, true});
  }
  SUBCASE("<=") {
    auto const m = FloatReg(a) <= FloatReg(b);
    FloatMaskArray out{};
    m.store(out);
    CHECK(out == FloatMaskArray{true, true, false, true});
  }
  SUBCASE(">") {
    auto const m = FloatReg(a) > FloatReg(b);
    FloatMaskArray out{};
    m.store(out);
    CHECK(out == FloatMaskArray{false, false, true, false});
  }
  SUBCASE(">=") {
    auto const m = FloatReg(a) >= FloatReg(b);
    FloatMaskArray out{};
    m.store(out);
    CHECK(out == FloatMaskArray{true, false, true, false});
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("simd::Reg::sum") {
  FloatArray const a{1.0F, 2.0F, 3.0F, 4.0F};
  CHECK(simd::sum(FloatReg(a)) == 10.0F);
}

TEST_CASE("simd::Reg::min_value") {
  FloatArray const a{3.0F, 2.0F, 4.0F, 1.0F};
  CHECK(simd::min_value(FloatReg(a)) == 1.0F);
}

TEST_CASE("simd::Reg::max_value") {
  FloatArray const a{3.0F, 2.0F, 4.0F, 1.0F};
  CHECK(simd::max_value(FloatReg(a)) == 4.0F);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
