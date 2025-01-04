/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <type_traits>

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Cast to signed integer of the same size.
template<std::unsigned_integral UInt>
constexpr auto to_signed(UInt value) -> std::make_signed_t<UInt> {
  return static_cast<std::make_signed_t<UInt>>(value);
}

/// Cast to unsigned integer of the same size.
template<std::signed_integral SInt>
constexpr auto to_unsigned(SInt value) -> std::make_unsigned_t<SInt> {
  return static_cast<std::make_unsigned_t<SInt>>(value);
}

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

} // namespace tit
