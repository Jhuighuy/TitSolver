/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <bit>
#include <concepts> // IWYU pragma: keep

namespace tit {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Divide two unsigned integers and round up the result. */
template<std::unsigned_integral UInt>
constexpr auto divide_up(UInt n, UInt d) noexcept -> UInt {
  return (n + d - UInt{1}) / d;
}

/** Align up to an unsigned integer. */
template<std::unsigned_integral UInt>
constexpr auto align_up(UInt n, UInt alignment) noexcept -> UInt {
  return divide_up(n, alignment) * alignment;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Check if integer n is power of two. */
template<std::unsigned_integral UInt>
constexpr auto is_power_of_two(UInt n) noexcept -> bool {
  return std::has_single_bit(n);
}

/** Align up an unsigned integer to the nearest power of two. */
template<std::unsigned_integral UInt>
constexpr auto align_up_to_power_of_two(UInt n) noexcept -> UInt {
  return std::bit_ceil(n);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit
