/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// IWYU pragma: private, include "tit/core/simd.hpp"
#pragma once

#include "tit/core/_simd/reg.hpp"
#include "tit/core/_simd/reg_mask.hpp"
#include "tit/core/_simd/traits.hpp"
#include "tit/core/basic_types.hpp"
#include "tit/core/math.hpp"

namespace tit::simd {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Deduce SIMD register to operate the given amount of scalars.
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
/// @{

/// Deduce SIMD register to operate the given amount of scalars.
template<supported_type Num, size_t Dim>
inline constexpr size_t deduce_size_v = [](this auto self, size_t reg_size) {
  if (Dim <= reg_size) return reg_size;
  if (reg_size >= max_reg_size_v<Num>) return max_reg_size_v<Num>;
  return self(reg_size * 2);
}(min_reg_size_v<Num>);

/// Amount of deduced SIMD registers required to store the given amount of
/// scalars.
template<supported_type Num, size_t Dim>
inline constexpr size_t deduce_count_v =
    divide_up(Dim, deduce_size_v<Num, Dim>);

/// Deduced SIMD register type to operate the given amount of scalars.
template<supported_type Num, size_t Dim>
using deduce_reg_t = Reg<Num, deduce_size_v<Num, Dim>>;

/// Deduced SIMD register mask type to operate the given amount of scalars.
template<supported_type Num, size_t Dim>
using deduce_reg_mask_t = RegMask<Num, deduce_size_v<Num, Dim>>;

/// @}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::simd
