/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <bit>
#include <concepts> // IWYU pragma: keep
#include <type_traits>

#include "tit/core/basic_types.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Divide two unsigned integers and round up the result.
template<std::unsigned_integral UInt>
constexpr auto divide_up(UInt n, UInt d) noexcept -> UInt {
  return (n + d - UInt{1}) / d;
}

/// Align up to an unsigned integer.
template<std::unsigned_integral UInt>
constexpr auto align_up(UInt n, UInt alignment) noexcept -> UInt {
  return divide_up(n, alignment) * alignment;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Check if integer n is power of two.
template<std::unsigned_integral UInt>
constexpr auto is_power_of_two(UInt n) noexcept -> bool {
  return std::has_single_bit(n);
}

/// Align up an unsigned integer to the nearest power of two.
template<std::unsigned_integral UInt>
constexpr auto align_up_to_power_of_two(UInt n) noexcept -> UInt {
  return std::bit_ceil(n);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Signed integer representation of the argument.
template<class Int>
  requires std::integral<Int>
constexpr auto to_signed(Int a) noexcept {
  return static_cast<std::make_signed_t<Int>>(a);
}

/// Unsigned integer representation of the argument.
template<class Int>
  requires std::integral<Int>
constexpr auto to_unsigned(Int a) noexcept {
  return static_cast<std::make_unsigned_t<Int>>(a);
}

/// Unsigned integer representation of the argument.
template<class T>
  requires std::integral<T> || std::floating_point<T>
constexpr auto to_bits(T a) noexcept {
  constexpr auto S = sizeof(T);
  if constexpr (S == sizeof(uint8_t)) return std::bit_cast<uint8_t>(a);
  else if constexpr (S == sizeof(uint16_t)) return std::bit_cast<uint16_t>(a);
  else if constexpr (S == sizeof(uint32_t)) return std::bit_cast<uint32_t>(a);
  else if constexpr (S == sizeof(uint64_t)) return std::bit_cast<uint64_t>(a);
  else static_assert(false);
}

/// Unsigned integral type of the same size.
template<class T>
  requires std::integral<T> || std::floating_point<T>
using make_bits_t = decltype(to_bits(std::declval<T>()));

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
