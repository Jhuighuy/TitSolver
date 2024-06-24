/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <cstddef>
#include <cstdint>

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// 8-bit signed integer type.
using std::int8_t;

/// 8-bit unsigned integer type.
using std::uint8_t;

/// 16-bit signed integer type.
using std::int16_t;

/// 16-bit unsigned integer type.
using std::uint16_t;

/// 32-bit signed integer type.
using std::uint32_t;

/// 32-bit unsigned integer type.
using std::uint32_t;

/// 64-bit signed integer type.
using std::int64_t;

/// 64-bit unsigned integer type.
using std::uint64_t;

/// Unsigned sized type.
using std::size_t;

/// Signed sized type.
using ssize_t = std::ptrdiff_t;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// 32-bit floating point type.
/// @todo Use definitions from <stdfloat> when it becomes available.
using float32_t = float;

/// 64-bit floating point type.
/// @todo Use definitions from <stdfloat> when it becomes available.
using float64_t = double;

/// Default floating-point type type.
using real_t = float64_t;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// C-style array reference.
template<class T, size_t Size>
using carr_ref_t = T (&)[Size]; // NOLINT(*-c-arrays)

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
