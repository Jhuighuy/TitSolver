/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// IWYU pragma: private, include "tit/core/simd.hpp"
#pragma once

#include <concepts>
#include <type_traits>

#include "tit/core/_simd/traits.hpp"
#include "tit/core/basic_types.hpp"

namespace tit::simd {

namespace impl {

template<supported_type Num>
struct mask_bits;

template<std::integral Int>
struct mask_bits<Int> : std::make_unsigned<Int> {};

template<>
struct mask_bits<float32_t> : std::type_identity<uint32_t> {};

template<>
struct mask_bits<float64_t> : std::type_identity<uint64_t> {};

template<supported_type Num>
using mask_bits_t = typename impl::mask_bits<Num>::type;

} // namespace impl

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Scalar mask.
template<supported_type Num>
class Mask final {
public:

  /// Construct a mask equivalent to boolean @p b.
  constexpr explicit(false) Mask(bool b = false) noexcept
      : bits_{b ? static_cast<Bits>(~Bits{0}) : Bits{0}} {}

  /// Cast back to the boolean.
  constexpr explicit(false) operator bool() const noexcept {
    return bits_ != 0;
  }

  /// Mask negation operation.
  friend constexpr auto operator!(Mask m) noexcept -> Mask {
    return Mask{~m.bits_};
  }

  /// Mask conjunction operation.
  friend constexpr auto operator&&(Mask m, Mask n) noexcept -> Mask {
    return Mask{m.bits_ & n.bits_};
  }

  /// Mask disjunction operation.
  friend constexpr auto operator||(Mask m, Mask n) noexcept -> Mask {
    return Mask{m.bits_ | n.bits_};
  }

  /// Mask equality operation.
  constexpr auto operator==(const Mask&) const noexcept -> bool = default;

private:

  using Bits = impl::mask_bits_t<Num>;

  constexpr explicit Mask(Bits bits) noexcept : bits_{bits} {}

  Bits bits_;

}; // class Mask

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::simd
