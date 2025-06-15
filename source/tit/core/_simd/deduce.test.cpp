/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <array>
#include <type_traits>

#include "tit/core/basic_types.hpp"
#include "tit/core/simd.hpp"

namespace tit {
namespace {

template<size_t Size, size_t Count>
using RegArray = std::array<simd::Reg<float, Size>, Count>;

template<size_t Dim>
using DeducedRegArray = std::array<simd::deduce_reg_t<float, Dim>,
                                   simd::deduce_count_v<float, Dim>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#if defined __AVX512F__

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

#elif defined __AVX__

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

#elif defined __SSE__

// Can be fitted into a single 128 bit register.
static_assert(std::is_same_v<DeducedRegArray<1>, RegArray<4, 1>>);
static_assert(std::is_same_v<DeducedRegArray<2>, RegArray<4, 1>>);
static_assert(std::is_same_v<DeducedRegArray<3>, RegArray<4, 1>>);

// Should occupy a few 128 bit registers.
static_assert(std::is_same_v<DeducedRegArray<7>, RegArray<4, 2>>);
static_assert(std::is_same_v<DeducedRegArray<8>, RegArray<4, 2>>);
static_assert(std::is_same_v<DeducedRegArray<9>, RegArray<4, 3>>);
static_assert(std::is_same_v<DeducedRegArray<12>, RegArray<4, 3>>);

#elif defined __ARM_NEON

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
