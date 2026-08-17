/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <array>
#include <cmath>
#include <numbers>
#include <span>

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
  FloatArray out{};
  SUBCASE("load and store") {
    const FloatArray in{1.0F, 2.0F, 3.0F, 4.0F};
    const FloatReg r(in);
    r.store(out);
    CHECK(in == out);
  }
  SUBCASE("zero initialization") {
    const FloatReg r{};
    out.fill(1.0F);
    r.store(out);
    for (const auto& x : out) CHECK(x == 0.0F);
  }
  SUBCASE("value initialization") {
    const auto val = 1.3F;
    const FloatReg r(val);
    r.store(out);
    for (const auto& x : out) CHECK(x == val);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("simd::Reg::operator+") {
  const FloatArray a{1.0F, 2.0F, 3.0F, 4.0F};
  const FloatArray b{5.0F, 6.0F, 7.0F, 8.0F};
  const FloatArray sum{6.0F, 8.0F, 10.0F, 12.0F};
  FloatArray out{};
  SUBCASE("normal") {
    (FloatReg{a} + FloatReg{b}).store(out);
    CHECK(out == sum);
  }
  SUBCASE("with assignment") {
    FloatReg r{a};
    r += FloatReg{b};
    r.store(out);
    CHECK(out == sum);
  }
}

TEST_CASE("simd::Reg::operator-") {
  const FloatArray a{5.0F, 6.0F, 7.0F, 8.0F};
  const FloatArray b{1.0F, 2.0F, 3.0F, 4.0F};
  const FloatArray diff{4.0F, 4.0F, 4.0F, 4.0F};
  FloatArray out{};
  SUBCASE("negation") {
    (-FloatReg{b}).store(out);
    CHECK(out == FloatArray{-1.0F, -2.0F, -3.0F, -4.0F});
  }
  SUBCASE("normal") {
    (FloatReg{a} - FloatReg{b}).store(out);
    CHECK(out == diff);
  }
  SUBCASE("with assignment") {
    FloatReg r{a};
    r -= FloatReg{b};
    r.store(out);
    CHECK(out == diff);
  }
}

TEST_CASE("simd::Reg::operator*") {
  const FloatArray a{2.0F, 3.0F, 4.0F, 5.0F};
  const FloatArray b{6.0F, 7.0F, 8.0F, 9.0F};
  const FloatArray prod{12.0F, 21.0F, 32.0F, 45.0F};
  FloatArray out{};
  SUBCASE("normal") {
    (FloatReg{a} * FloatReg{b}).store(out);
    CHECK(out == prod);
  }
  SUBCASE("with assignment") {
    FloatReg r{a};
    r *= FloatReg{b};
    r.store(out);
    CHECK(out == prod);
  }
}

TEST_CASE("simd::Reg::operator/") {
  const FloatArray a{12.0F, 21.0F, 32.0F, 45.0F};
  const FloatArray b{6.0F, 7.0F, 8.0F, 9.0F};
  const FloatArray quot{2.0F, 3.0F, 4.0F, 5.0F};
  FloatArray out{};
  SUBCASE("normal") {
    (FloatReg{a} / FloatReg{b}).store(out);
    CHECK(out == quot);
  }
  SUBCASE("with assignment") {
    FloatReg r{a};
    r /= FloatReg{b};
    r.store(out);
    CHECK(out == quot);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("simd::Reg::operator<=>") {
  const FloatArray a{1.0F, 2.0F, 4.0F, 4.0F};
  const FloatArray b{1.0F, 5.0F, 3.0F, 7.0F};
  FloatMaskArray out{};
  SUBCASE("==") {
    (FloatReg{a} == FloatReg{b}).store(out);
    CHECK(out == FloatMaskArray{true, false, false, false});
  }
  SUBCASE("!=") {
    (FloatReg{a} != FloatReg{b}).store(out);
    CHECK(out == FloatMaskArray{false, true, true, true});
  }
  SUBCASE("<") {
    (FloatReg{a} < FloatReg{b}).store(out);
    CHECK(out == FloatMaskArray{false, true, false, true});
  }
  SUBCASE("<=") {
    (FloatReg{a} <= FloatReg{b}).store(out);
    CHECK(out == FloatMaskArray{true, true, false, true});
  }
  SUBCASE(">") {
    (FloatReg{a} > FloatReg{b}).store(out);
    CHECK(out == FloatMaskArray{false, false, true, false});
  }
  SUBCASE(">=") {
    (FloatReg{a} >= FloatReg{b}).store(out);
    CHECK(out == FloatMaskArray{true, false, true, false});
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("simd::Reg::broadcast") {
  FloatArray out{};
  simd::broadcast(FloatReg{FloatArray{1.0F, 2.0F, 3.0F, 4.0F}}).store(out);
  CHECK(out == FloatArray{1.0F, 1.0F, 1.0F, 1.0F});
}

TEST_CASE("simd::Reg::make_pair") {
  FloatArray out{};
  simd::make_pair(3.0F, 5.0F).store(out);
  CHECK(out[0] == 3.0F);
  CHECK(out[1] == 5.0F);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("simd::Reg::reg_cast") {
  SUBCASE("same type") {
    const FloatArray in{1.0F, -2.0F, 3.5F, -4.9F};
    FloatArray out{};
    simd::reg_cast<float>(FloatReg{in}).store(out);
    CHECK(out == in);
  }
  SUBCASE("different type") {
    const FloatArray in{1.0F, -2.0F, 3.5F, -4.9F};
    std::array<int, 4> out{};
    simd::reg_cast<int>(FloatReg{in}).store(out);
    CHECK(out == std::array{1, -2, 3, -4});
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("simd::Reg::abs") {
  FloatArray out{};
  simd::abs(FloatReg{FloatArray{-5.0F, 6.0F, 7.0F, 8.0F}}).store(out);
  CHECK(out == FloatArray{5.0F, 6.0F, 7.0F, 8.0F});
}

TEST_CASE("simd::Reg::copysign") {
  FloatArray out{};
  simd::copysign(FloatReg{FloatArray{1.0F, 2.0F, 3.0F, 4.0F}},
                 FloatReg{FloatArray{-1.0F, 1.0F, -1.0F, 1.0F}})
      .store(out);
  CHECK(out == FloatArray{-1.0F, 2.0F, -3.0F, 4.0F});
}

TEST_CASE("simd::Reg::min") {
  FloatArray out{};
  simd::min(FloatReg{FloatArray{5.0F, 6.0F, 7.0F, 8.0F}},
            FloatReg{FloatArray{1.0F, 7.0F, 4.0F, 9.0F}})
      .store(out);
  CHECK(out == FloatArray{1.0F, 6.0F, 4.0F, 8.0F});
}

TEST_CASE("simd::Reg::max") {
  FloatArray out{};
  simd::max(FloatReg{FloatArray{5.0F, 6.0F, 7.0F, 8.0F}},
            FloatReg{FloatArray{1.0F, 7.0F, 4.0F, 9.0F}})
      .store(out);
  CHECK(out == FloatArray{5.0F, 7.0F, 7.0F, 9.0F});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("simd::Reg::filter") {
  FloatArray out{};
  simd::filter(FloatRegMask{FloatMaskArray{true, false, true, false}},
               FloatReg{FloatArray{5.0F, 6.0F, 7.0F, 8.0F}})
      .store(out);
  CHECK(out == FloatArray{5.0F, 0.0F, 7.0F, 0.0F});
}

TEST_CASE("simd::Reg::select") {
  FloatArray out{};
  simd::select(FloatRegMask{FloatMaskArray{true, false, true, false}},
               FloatReg{FloatArray{5.0F, 6.0F, 7.0F, 8.0F}},
               FloatReg{FloatArray{1.0F, 2.0F, 3.0F, 4.0F}})
      .store(out);
  CHECK(out == FloatArray{5.0F, 2.0F, 7.0F, 4.0F});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("simd::Reg::take_n") {
  const FloatArray a{1.0F, 2.0F, 3.0F, 4.0F};
  FloatArray out{};
  SUBCASE("n = 0") {
    simd::take_n(0, FloatReg{a}).store(out);
    CHECK(out == FloatArray{0.0F, 0.0F, 0.0F, 0.0F});
  }
  SUBCASE("n = 1") {
    simd::take_n(1, FloatReg{a}).store(out);
    CHECK(out == FloatArray{1.0F, 0.0F, 0.0F, 0.0F});
  }
  SUBCASE("n = 2") {
    simd::take_n(2, FloatReg{a}).store(out);
    CHECK(out == FloatArray{1.0F, 2.0F, 0.0F, 0.0F});
  }
  SUBCASE("n = 3") {
    simd::take_n(3, FloatReg{a}).store(out);
    CHECK(out == FloatArray{1.0F, 2.0F, 3.0F, 0.0F});
  }
  SUBCASE("n = 4") {
    simd::take_n(4, FloatReg{a}).store(out);
    CHECK(out == FloatArray{1.0F, 2.0F, 3.0F, 4.0F});
  }
}

TEST_CASE("simd::Reg::merge_n") {
  const FloatArray a{1.0F, 2.0F, 3.0F, 4.0F};
  const FloatArray b{5.0F, 6.0F, 7.0F, 8.0F};
  FloatArray out{};
  SUBCASE("n = 0") {
    simd::merge_n(0, FloatReg{a}, FloatReg{b}).store(out);
    CHECK(out == FloatArray{5.0F, 6.0F, 7.0F, 8.0F});
  }
  SUBCASE("n = 1") {
    simd::merge_n(1, FloatReg{a}, FloatReg{b}).store(out);
    CHECK(out == FloatArray{1.0F, 6.0F, 7.0F, 8.0F});
  }
  SUBCASE("n = 2") {
    simd::merge_n(2, FloatReg{a}, FloatReg{b}).store(out);
    CHECK(out == FloatArray{1.0F, 2.0F, 7.0F, 8.0F});
  }
  SUBCASE("n = 3") {
    simd::merge_n(3, FloatReg{a}, FloatReg{b}).store(out);
    CHECK(out == FloatArray{1.0F, 2.0F, 3.0F, 8.0F});
  }
  SUBCASE("n = 4") {
    simd::merge_n(4, FloatReg{a}, FloatReg{b}).store(out);
    CHECK(out == FloatArray{1.0F, 2.0F, 3.0F, 4.0F});
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("simd::Reg::floor") {
  FloatArray out{};
  simd::floor(FloatReg{FloatArray{1.5F, 2.7F, 3.1F, 4.9F}}).store(out);
  CHECK(out == FloatArray{1.0F, 2.0F, 3.0F, 4.0F});
}

TEST_CASE("simd::Reg::round") {
  FloatArray out{};
  simd::round(FloatReg{FloatArray{1.5F, 2.7F, 3.1F, 4.9F}}).store(out);
  CHECK(out == FloatArray{2.0F, 3.0F, 3.0F, 5.0F});
}

TEST_CASE("simd::Reg::ceil") {
  FloatArray out{};
  simd::ceil(FloatReg{FloatArray{1.5F, 2.7F, 3.1F, 4.9F}}).store(out);
  CHECK(out == FloatArray{2.0F, 3.0F, 4.0F, 5.0F});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("simd::Reg::fma") {
  FloatArray out{};
  simd::fma(FloatReg{FloatArray{1.0F, 2.0F, 3.0F, 4.0F}},
            FloatReg{FloatArray{5.0F, 6.0F, 7.0F, 8.0F}},
            FloatReg{FloatArray{9.0F, 10.0F, 11.0F, 12.0F}})
      .store(out);
  CHECK(out == FloatArray{14.0F, 22.0F, 32.0F, 44.0F});
}

TEST_CASE("simd::Reg::sqrt") {
  FloatArray out{};
  simd::sqrt(FloatReg{FloatArray{1.0F, 4.0F, 9.0F, 16.0F}}).store(out);
  CHECK(out == FloatArray{1.0F, 2.0F, 3.0F, 4.0F});
}

TEST_CASE("simd::Reg::fast_atan2") {
  FloatArray out{};
  simd::fast_atan2(FloatReg{FloatArray{0.0F, 1.0F, 0.0F, -1.0F}},
                   FloatReg{FloatArray{1.0F, 0.0F, -1.0F, 0.0F}})
      .store(out);
  const auto pi = std::numbers::pi_v<float>;
  CHECK_APPROX_EQ(out[0], 0.0F);
  CHECK_APPROX_EQ(out[1], pi / 2);
  CHECK_APPROX_EQ(out[2], pi);
  CHECK_APPROX_EQ(out[3], -pi / 2);
}

TEST_CASE("simd::Reg::fast_log1p") {
  FloatArray out{};
  simd::fast_log1p(FloatReg{FloatArray{0.0F, 1.0F, 2.0F, -0.5F}}).store(out);
  CHECK_APPROX_EQ(out[0], 0.0F);
  CHECK_APPROX_EQ(out[1], std::numbers::ln2_v<float>);
  CHECK_APPROX_EQ(out[2], std::log(3.0F));
  CHECK_APPROX_EQ(out[3], std::log(0.5F));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("simd::Reg::extract") {
  const FloatReg r{FloatArray{1.0F, 2.0F, 3.0F, 4.0F}};
  CHECK(simd::extract<0>(r) == 1.0F);
  CHECK(simd::extract<1>(r) == 2.0F);
  CHECK(simd::extract<2>(r) == 3.0F);
  CHECK(simd::extract<3>(r) == 4.0F);
}

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
