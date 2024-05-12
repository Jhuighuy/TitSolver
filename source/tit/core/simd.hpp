/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>

#include "tit/core/basic_types.hpp"
#include "tit/core/uint_utils.hpp"

// IWYU pragma: begin_exports
#include "tit/core/simd/reg.hpp"
#ifdef __ARM_NEON
#include "tit/core/simd/reg.arm.hpp"
#elifdef __SSE__
#include "tit/core/simd/reg.x86.hpp"
#endif
// IWYU pragma: end_exports

namespace tit::simd {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Minimal size of the SIMD register (in bytes) that is available on the
/// current hardware. That is 16 bytes for all supported instruction sets.
inline constexpr size_t min_reg_size_bytes_v{
#ifdef __ARM_NEON
    16
#elifdef __SSE__
    16
#else
    1
#endif
};

static_assert(is_power_of_two(min_reg_size_bytes_v));

/// Maximal size of the SIMD register (in bytes) that is available on the
/// current hardware. That is 16 bytes for NEON and SSE instruction set,
/// 32 and 64 bytes for AVX and AVX-512 respectively.
inline constexpr size_t max_reg_size_bytes_v{
#ifdef __ARM_NEON
    16
#elifdef __AVX512F__
    64
#elifdef __AVX__
    32
#elifdef __SSE__
    16
#else
    1
#endif
};

static_assert(is_power_of_two(max_reg_size_bytes_v) &&
              max_reg_size_bytes_v >= min_reg_size_bytes_v);

/// Minimal SIMD register size (in elements) for the specified type.
template<class Num>
inline constexpr auto min_reg_size_v = min_reg_size_bytes_v / sizeof(Num);

/// Maximal SIMD register size (in elements) for the specified type.
template<class Num>
inline constexpr auto max_reg_size_v = max_reg_size_bytes_v / sizeof(Num);

/// Size of "the most suitable register" to operate the given amount of scalars.
/// @see simd::available_for
template<class Num, size_t Dim>
inline constexpr auto reg_size_for_v = std::clamp(
    align_up_to_power_of_two(Dim), min_reg_size_v<Num>, max_reg_size_v<Num>);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// @brief Is "the most suitable SIMD register" type to operate the given amount
///        of scalars specialized?
///
/// "The most suitable SIMD register" is the smallest available SIMD register
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
concept available_for = available<Num, reg_size_for_v<Num, Dim>>;

/// Syntax sugar for conditional compilation based on the availability of the
/// most suitable SIMD register for the given amount of scalars.
#define TIT_IF_SIMD_AVALIABLE(Num, Dim)                                        \
  if constexpr (simd::available_for<Num, Dim>)                                 \
    if !consteval

/// Amount of "the most suitable SIMD registers" required to store the given
/// amount of scalars.
/// @see simd::available_for */
template<class Num, size_t Dim>
inline constexpr auto reg_count_for_v =
    divide_up(Dim, reg_size_for_v<Num, Dim>);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::simd
