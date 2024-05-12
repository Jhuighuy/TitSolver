/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <concepts>

#include "tit/core/basic_types.hpp"
#include "tit/core/type_traits.hpp"
#include "tit/core/uint_utils.hpp"

namespace tit::simd {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Minimal byte width of the SIMD register that is available on the current
/// hardware.
///
/// That is 16 bytes for all supported instruction sets.
inline constexpr size_t min_simd_byte_width_v{
#ifdef __ARM_NEON
    16
#elifdef __SSE__
    16
#else
    0
#endif
};

/// Maximal byte width of the SIMD register that is available on the current
/// hardware.
///
/// That is 16 bytes for NEON and SSE instruction set, 32 and 64 bytes for
/// AVX and AVX-512 respectively.
inline constexpr size_t max_simd_byte_width_v{
#ifdef __ARM_NEON
    16
#elifdef __AVX512F__
    64
#elifdef __AVX__
    32
#elifdef __SSE__
    16
#else
    0
#endif
};

// Just in case.
static_assert(max_simd_byte_width_v >= min_simd_byte_width_v &&
              is_power_of_two(max_simd_byte_width_v / min_simd_byte_width_v));

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Minimal SIMD register size for the specified type.
template<class Num>
  requires std::integral<Num> || std::floating_point<Num>
inline constexpr size_t min_simd_size_v = min_simd_byte_width_v / sizeof(Num);

/// Maximal SIMD register size for the specified type.
template<class Num>
  requires std::integral<Num> || std::floating_point<Num>
inline constexpr size_t max_simd_size_v = max_simd_byte_width_v / sizeof(Num);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Is SIMD supported for the specified numeric type and size?
template<class Num, size_t Size>
concept supported =
    (std::integral<Num> || std::floating_point<Num>) &&
    in_range_v<Size, min_simd_size_v<Num>, max_simd_size_v<Num>> &&
    (Size % min_simd_size_v<Num> == 0);

/// Is SIMD register splittable for the specified numeric type and size?
template<class Num, size_t Size>
concept splittable = supported<Num, Size> && (Size > min_simd_size_v<Num>);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Deduce SIMD register size to operate the given amount of scalars.
///
/// Deduced SIMD register is the smallest available SIMD register
/// that can operate the given amount of scalars with the least amount of
/// instructions. For example:
/// - For 1 or 2 `double`s on an SSE/NEON-capable machine it is the 128-bit
///   register, since there is no need to use wider registers even if they are
///   available.
/// - For 3 or 4 `double`s on an at least AVX-capable machine it is the 256-bit
///   register, since there is no need to use wider registers even if they are
///   available. If the machine is only SSE/NEON-capable, then it is the
///   128-bit register, since those are the only available on the hardware.
template<class Num, size_t Dim>
  requires std::integral<Num> || std::floating_point<Num>
inline constexpr size_t deduced_simd_size_v = std::clamp(
    align_up_to_power_of_two(Dim), min_simd_size_v<Num>, max_simd_size_v<Num>);

/// Amount of deduced SIMD registers required to store the given amount of
/// scalars.
///
/// @see simd::deduced_simd_size_v
template<class Num, size_t Dim>
inline constexpr size_t deduced_simd_count_for_v =
    divide_up(Dim, deduced_simd_size_v<Num, Dim>);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::simd
