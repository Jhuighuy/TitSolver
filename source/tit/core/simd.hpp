/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>

#include "tit/core/basic_types.hpp"
#include "tit/core/math_utils.hpp"

namespace tit::simd {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** @brief SIMD register size (in bytes) available on the current hardware.
 ** What is 16 bytes for SSE and NEON instruction set, 32 bytes for the AVX
 ** instruction set and 64 bytes for the AVX-512 instruction set.
 ** When no known instruction set is detected, some default value is set to
 ** emphasize automatic vectorization for the compiler. */
inline constexpr size_t max_reg_size_bytes_v =
#if defined(__AVX512F__)
    64
#elif defined(__AVX__)
    32
#elif defined(__SSE__) || defined(__ARM_NEON)
    16
#else
    16 // No known SIMD instruction set is available. Use 16 bytes as default
       // and hope for automatic vectorization.
#endif
    ;

static_assert(is_power_of_two(max_reg_size_bytes_v));

/** Maximal SIMD register size (in elements) for the specified type. */
template<class Num>
  requires (is_power_of_two(sizeof(Num)))
inline constexpr auto max_reg_size_v =
    std::max<size_t>(1, max_reg_size_bytes_v / sizeof(Num));

// TODO: description for this concept is incorrect. This concept is used
// to determine if we need to make an explicit specialization for `Vec` for
// generic vectorized case.
/** @brief Does this amount of scalars form exactly a single SIMD register?
 ** Registers are used if:
 ** - either number of dimensions is greater than register
 **   size for the scalar type (e.g., 3 * `double` with NEON instruction set),
 ** - either it is less than register size for the scalar type
 **   (e.g. 3 * `double` on AVX instruction set) and number of dimensions is not
 **   power of two. In the latter case fractions of registers are used (e.g.,
 **   for 2 * `double` with AVX instruction set SSE registers are be used. */
template<class Num, size_t Dim>
concept use_regs = (Dim > max_reg_size_v<Num>) ||
                   (Dim < max_reg_size_v<Num> && !is_power_of_two(Dim));

/** SIMD register size for the specified amount of scalars. */
template<class Num, size_t Dim>
  requires use_regs<Num, Dim>
inline constexpr auto reg_size_v =
    std::min(max_reg_size_v<Num>, align_to_power_of_two(Dim));

/** Do SIMD registers match for the specified types? */
template<size_t Dim, class Num, class... RestNums>
concept regs_match =
    use_regs<Num, Dim> && (use_regs<RestNums, Dim> && ...) &&
    ((reg_size_v<Num, Dim> == reg_size_v<RestNums, Dim>) &&...);

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit::simd
