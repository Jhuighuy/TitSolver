/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// IWYU pragma: private, include "tit/core/simd.hpp"
#pragma once

#include <bit>
#include <span>

#include <hwy/highway.h>

#include "tit/core/_simd/mask.hpp"
#include "tit/core/_simd/traits.hpp"
#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/type.hpp"

namespace tit::simd {

namespace hn = hwy::HWY_NAMESPACE;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// SIMD register mask.
template<class Num, size_t Size>
  requires supported<Num, Size>
class alignas(sizeof(Num) * Size) RegMask final {
public:

  /// Highway tag type.
  using Tag = hn::FixedTag<normalize_type_t<Num>, Size>;

  /// Highway mask register type.
  using Base = hn::Mask<Tag>;

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Base Highway register.
  Base base; // NOLINT(*-non-private-member-variables-in-classes)

  /// Construct SIMD register mask from the Highway register.
  [[gnu::always_inline]]
  explicit(false) RegMask(const Base& b) noexcept
      : base{b} {}

  /// Fill-initialize the SIMD register mask with zeroes.
  [[gnu::always_inline]]
  RegMask() noexcept
      : base{hn::MaskFromVec(hn::Zero(Tag{}))} {}

  /// Fill-initialize the SIMD register mask.
  [[gnu::always_inline]]
  explicit RegMask(Mask<Num> q) noexcept
      : base{hn::MaskFromVec(hn::Set(Tag{}, std::bit_cast<Num>(q)))} {}

  /// Load SIMD register mask from memory.
  [[gnu::always_inline]]
  explicit RegMask(std::span<const Mask<Num>> span) noexcept {
    TIT_ASSERT(span.size() >= Size, "Span size is too small!");
    base = hn::MaskFromVec( // NOLINT(*-prefer-member-initializer)
        hn::LoadU(Tag{}, safe_bit_ptr_cast<const Num*>(span.data())));
  }

  /// Store SIMD register mask into memory.
  [[gnu::always_inline]]
  void store(std::span<Mask<Num>> span) const noexcept {
    TIT_ASSERT(span.size() >= Size, "Span size is too small!");
    hn::StoreU(hn::VecFromMask(Tag{}, base),
               Tag{},
               safe_bit_ptr_cast<Num*>(span.data()));
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// SIMD mask negation operation.
  [[gnu::always_inline]]
  friend auto operator!(const RegMask& m) noexcept -> RegMask {
    return hn::Not(m.base);
  }

  /// SIMD mask conjunction operation.
  [[gnu::always_inline]]
  friend auto operator&&(const RegMask& m, const RegMask& n) noexcept
      -> RegMask {
    return hn::And(m.base, n.base);
  }

  /// SIMD mask disjunction operation.
  [[gnu::always_inline]]
  friend auto operator||(const RegMask& m, const RegMask& n) noexcept
      -> RegMask {
    return hn::Or(m.base, n.base);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// SIMD mask "equal to" comparison operation.
  [[gnu::always_inline]]
  friend auto operator==(const RegMask& m, const RegMask& n) noexcept
      -> RegMask {
    return hn::Not(hn::Xor(m.base, n.base));
  }

  /// SIMD mask "not equal to" comparison operation.
  [[gnu::always_inline]]
  friend auto operator!=(const RegMask& m, const RegMask& n) noexcept
      -> RegMask {
    return hn::Xor(m.base, n.base);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

}; // class RegMask

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Check if any SIMD register mask element is set to true.
template<class Num, size_t Size>
  requires supported<Num, Size>
[[gnu::always_inline]]
inline auto any(const RegMask<Num, Size>& m) noexcept -> bool {
  return !hn::AllFalse(typename RegMask<Num, Size>::Tag{}, m.base);
}

/// Check if all SIMD register mask elements are set to true.
template<class Num, size_t Size>
  requires supported<Num, Size>
[[gnu::always_inline]]
inline auto all(const RegMask<Num, Size>& m) noexcept -> bool {
  return hn::AllTrue(typename RegMask<Num, Size>::Tag{}, m.base);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Count the number of true values in the SIMD register mask.
template<class Num, size_t Size>
  requires supported<Num, Size>
[[gnu::always_inline]]
inline auto count_true(const RegMask<Num, Size>& m) noexcept -> size_t {
  return hn::CountTrue(typename RegMask<Num, Size>::Tag{}, m.base);
}

/// Try to find the first true value in the SIMD register mask.
/// Return `-1` if all values are false.
template<class Num, size_t Size>
  requires supported<Num, Size>
[[gnu::always_inline]]
inline auto find_true(const RegMask<Num, Size>& m) noexcept -> ssize_t {
  return hn::FindFirstTrue(typename RegMask<Num, Size>::Tag{}, m.base);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::simd
