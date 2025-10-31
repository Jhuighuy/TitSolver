/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// IWYU pragma: private, include "tit/core/simd.hpp"
#pragma once

/// @todo AVX-512 uses special 8/16 bit mask registers, which do not fit into
///       our approach of dealing with masks. Let's disable AVX-512 for now.
#ifdef __AVX512F__
#undef __AVX512F__
#endif

#include <bit>
#include <concepts>

#include "tit/core/basic_types.hpp"

namespace tit::simd {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Is SIMD supported for the specified numeric type?
template<class Num>
concept supported_type = std::integral<Num> || std::floating_point<Num>;

/// Evaluate the following command block if SIMD is supported for the given
/// type and if the code is not executed at compile-time.
#define TIT_IF_SIMD_AVALIABLE(Num)                                             \
  if constexpr (simd::supported_type<Num>)                                     \
    if !consteval

/// Is SIMD castable from one type to another?
template<class From, class To>
concept castable_to_type =
    supported_type<To> && supported_type<From> && (sizeof(To) == sizeof(From));

/// Evaluate the following command block if SIMD cast is supported for the given
/// type and if the code is not executed at compile-time.
#define TIT_IF_SIMD_CAST_AVALIABLE(From, To)                                   \
  if constexpr (simd::castable_to_type<From, To>)                              \
    if !consteval

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Minimal byte width of the SIMD register that is available on the current
/// hardware.
///
/// That is 16 bytes for all supported instruction sets.
inline constexpr size_t min_reg_byte_width_v{
#ifdef __SSE__
    16
#elifdef __ARM_NEON
    8
#else
#error Unknown SIMD instruction set!
#endif
};

/// Maximal byte width of the SIMD register that is available on the current
/// hardware.
///
/// That is 16 bytes for NEON and SSE instruction set, 32 and 64 bytes for
/// AVX and AVX-512 respectively.
inline constexpr size_t max_reg_byte_width_v{
#ifdef __AVX512F__
    64
#elifdef __AVX__
    32
#elif defined __SSE__ || defined __ARM_NEON
    16
#else
#error Unknown SIMD instruction set!
#endif
};

// Just in case.
static_assert(max_reg_byte_width_v >= min_reg_byte_width_v &&
              std::has_single_bit(max_reg_byte_width_v / min_reg_byte_width_v));

/// Minimal SIMD register size for the specified type.
template<supported_type Num>
inline constexpr size_t min_reg_size_v = min_reg_byte_width_v / sizeof(Num);

/// Maximal SIMD register size for the specified type.
template<supported_type Num>
inline constexpr size_t max_reg_size_v = max_reg_byte_width_v / sizeof(Num);

/// Is SIMD supported for the specified numeric type and size?
template<class Num, size_t Size>
concept supported =
    supported_type<Num> &&
    (min_reg_size_v<Num> <= Size && Size <= max_reg_size_v<Num>) &&
    (Size % min_reg_size_v<Num> == 0);

// Is SIMD castable from one type to another?
template<class From, class To, size_t Size>
concept castable_to =
    supported<To, Size> && supported<From, Size> && castable_to_type<From, To>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::simd
