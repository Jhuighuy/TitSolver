/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <array>
#include <cstddef>
#include <type_traits>

#include <hwy/highway.h>

#include "tit/core/simd.hpp"

namespace tit {
namespace {

template<std::size_t Size, std::size_t Count>
using RegArray = std::array<simd::Reg<float, Size>, Count>;

template<std::size_t Dim>
using DeducedRegArray = std::array<simd::deduce_reg_t<float, Dim>,
                                   simd::deduce_count_v<float, Dim>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#if (HWY_TARGET & (HWY_AVX3 | HWY_AVX3_DL | HWY_AVX3_ZEN4 | HWY_AVX3_SPR |     \
                   HWY_AVX10_2)) != 0

// Can be fitted into a single 128 bit register.
static_assert(std::is_same_v<DeducedRegArray<1>, RegArray<4, 1>>);
static_assert(std::is_same_v<DeducedRegArray<2>, RegArray<4, 1>>);
static_assert(std::is_same_v<DeducedRegArray<3>, RegArray<4, 1>>);
static_assert(std::is_same_v<DeducedRegArray<4>, RegArray<4, 1>>);

// Can be fitted into a single 256 bit register.
static_assert(std::is_same_v<DeducedRegArray<5>, RegArray<8, 1>>);
static_assert(std::is_same_v<DeducedRegArray<6>, RegArray<8, 1>>);
static_assert(std::is_same_v<DeducedRegArray<7>, RegArray<8, 1>>);
static_assert(std::is_same_v<DeducedRegArray<8>, RegArray<8, 1>>);

// Can be fitted into a single 512 bit register.
static_assert(std::is_same_v<DeducedRegArray<9>, RegArray<16, 1>>);
static_assert(std::is_same_v<DeducedRegArray<10>, RegArray<16, 1>>);
static_assert(std::is_same_v<DeducedRegArray<16>, RegArray<16, 1>>);

// Should occupy a few 512 bit registers.
static_assert(std::is_same_v<DeducedRegArray<17>, RegArray<16, 2>>);
static_assert(std::is_same_v<DeducedRegArray<20>, RegArray<16, 2>>);
static_assert(std::is_same_v<DeducedRegArray<32>, RegArray<16, 2>>);
static_assert(std::is_same_v<DeducedRegArray<36>, RegArray<16, 3>>);

#elif HWY_TARGET == HWY_AVX2

// Can be fitted into a single 128 bit register.
static_assert(std::is_same_v<DeducedRegArray<1>, RegArray<4, 1>>);
static_assert(std::is_same_v<DeducedRegArray<2>, RegArray<4, 1>>);
static_assert(std::is_same_v<DeducedRegArray<3>, RegArray<4, 1>>);
static_assert(std::is_same_v<DeducedRegArray<4>, RegArray<4, 1>>);

// Can be fitted into a single 256 bit register.
static_assert(std::is_same_v<DeducedRegArray<5>, RegArray<8, 1>>);
static_assert(std::is_same_v<DeducedRegArray<6>, RegArray<8, 1>>);
static_assert(std::is_same_v<DeducedRegArray<7>, RegArray<8, 1>>);
static_assert(std::is_same_v<DeducedRegArray<8>, RegArray<8, 1>>);

// Should occupy a few 512 bit registers.
static_assert(std::is_same_v<DeducedRegArray<9>, RegArray<8, 2>>);
static_assert(std::is_same_v<DeducedRegArray<16>, RegArray<8, 2>>);
static_assert(std::is_same_v<DeducedRegArray<17>, RegArray<8, 3>>);
static_assert(std::is_same_v<DeducedRegArray<24>, RegArray<8, 3>>);

#elif (HWY_TARGET & (HWY_SSE2 | HWY_SSSE3 | HWY_SSE4)) != 0

// Can be fitted into a single 128 bit register.
static_assert(std::is_same_v<DeducedRegArray<1>, RegArray<4, 1>>);
static_assert(std::is_same_v<DeducedRegArray<2>, RegArray<4, 1>>);
static_assert(std::is_same_v<DeducedRegArray<3>, RegArray<4, 1>>);

// Should occupy a few 128 bit registers.
static_assert(std::is_same_v<DeducedRegArray<7>, RegArray<4, 2>>);
static_assert(std::is_same_v<DeducedRegArray<8>, RegArray<4, 2>>);
static_assert(std::is_same_v<DeducedRegArray<9>, RegArray<4, 3>>);
static_assert(std::is_same_v<DeducedRegArray<12>, RegArray<4, 3>>);

#elif (HWY_TARGET & HWY_ALL_NEON) != 0

// Can be fitted into a single 64 bit register.
static_assert(std::is_same_v<DeducedRegArray<1>, RegArray<2, 1>>);
static_assert(std::is_same_v<DeducedRegArray<2>, RegArray<2, 1>>);

// Can be fitted into a single 128 bit register.
static_assert(std::is_same_v<DeducedRegArray<3>, RegArray<4, 1>>);
static_assert(std::is_same_v<DeducedRegArray<4>, RegArray<4, 1>>);

// Should occupy a few 128 bit registers.
static_assert(std::is_same_v<DeducedRegArray<5>, RegArray<4, 2>>);
static_assert(std::is_same_v<DeducedRegArray<8>, RegArray<4, 2>>);
static_assert(std::is_same_v<DeducedRegArray<9>, RegArray<4, 3>>);
static_assert(std::is_same_v<DeducedRegArray<12>, RegArray<4, 3>>);

#else

#error Unknown SIMD instruction set!

#endif

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
