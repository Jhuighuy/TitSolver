/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <cstddef>
#include <cstdint>

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Byte type.
using byte_t = std::byte;

/// 8-bit signed integer type.
using std::int8_t;

/// 8-bit unsigned integer type.
using std::uint8_t;

/// 16-bit signed integer type.
using std::int16_t;

/// 16-bit unsigned integer type.
using std::uint16_t;

/// 32-bit signed integer type.
using std::int32_t;

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

/// Invalid index.
inline constexpr size_t npos = SIZE_MAX;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

static_assert(sizeof(float) == 4, "'float' must be 32 bits");

/// 32-bit floating point type.
using float32_t = float;

static_assert(sizeof(double) == 8, "'double' must be 64 bits");

/// 64-bit floating point type.
using float64_t = double;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
