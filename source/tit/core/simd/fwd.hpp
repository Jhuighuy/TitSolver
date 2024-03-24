/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <type_traits>

#include "tit/core/basic_types.hpp"

namespace tit::simd {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// SIMD register type.
template<class Num, size_t Size>
  requires std::is_arithmetic_v<Num>
class Reg {}; // class Reg

/// SIMD register mask type.
template<class Num, size_t Size>
  requires std::is_arithmetic_v<Num>
class RegMask {}; // class RegMask

/// Is the SIMD register of the given type and size specialized?
template<class Num, size_t Size>
concept available = !std::is_empty_v<Reg<Num, Size>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::simd
