/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <cstddef>
#include <cstdint>

namespace tit {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** 8-bit unsigned integer type. */
using uint8_t = std::uint8_t;

/** 16-bit unsigned integer type. */
using uint16_t = std::uint16_t;

/** 32-bit unsigned integer type. */
using uint32_t = std::uint32_t;

/** 64-bit unsigned integer type. */
using uint64_t = std::uint64_t;

/** Unsigned sized type. */
using size_t = std::size_t;

/** Signed sized type. */
using ssize_t = std::ptrdiff_t;

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** 32-bit floating point type. */
using float32_t = float;

/** 64-bit floating point type. */
using float64_t = double;

/** Default floating-point type type. */
using real_t = float64_t;

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit

// I do not know where to put these, so I'll leave them here.
#if defined(__clang_major__) && (__clang_major__ > 17)
#error "It is time for a huge clean-up!"
#endif
#if defined(__GNUC__) && (__GNUC__ > 13)
#error "It is time for a huge clean-up!"
#endif
