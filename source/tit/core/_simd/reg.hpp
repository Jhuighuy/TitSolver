/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// IWYU pragma: private, include "tit/core/simd.hpp"
#pragma once

#include <span>

#include <hwy/highway.h>

#include "tit/core/_simd/reg_mask.hpp"
#include "tit/core/_simd/traits.hpp"
#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"

namespace tit::simd {

namespace hn = hwy::HWY_NAMESPACE;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// SIMD register.
template<class Num, size_t Size>
  requires supported<Num, Size>
class alignas(sizeof(Num) * Size) Reg final {
public:

  /// Highway tag type.
  using Tag = hn::FixedTag<impl::fixed_width_type_t<Num>, Size>;

  /// Highway register type.
  using Base = hn::Vec<Tag>;

  /// Associated mask register type.
  using RegMask = tit::simd::RegMask<Num, Size>;

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Base Highway register.
  Base base; // NOLINT(*-non-private-member-variables-in-classes)

  /// Construct SIMD register from the Highway register.
  [[gnu::always_inline]]
  Reg(const Base& b) noexcept
      : base{b} {}

  /// Fill-initialize the SIMD register with zeroes.
  [[gnu::always_inline]]
  Reg() noexcept
      : base{hn::Zero(Tag{})} {}

  /// Fill-initialize the SIMD register.
  [[gnu::always_inline]]
  explicit Reg(Num q) noexcept
      : base{hn::Set(Tag{}, q)} {}

  /// Load SIMD register from memory.
  [[gnu::always_inline]]
  explicit Reg(std::span<const Num> span) noexcept {
    TIT_ASSERT(span.size() >= Size, "Data size is too small!");
    base = hn::LoadU(Tag{}, span.data()); // NOLINT(*-prefer-member-initializer)
  }

  /// Store SIMD register into memory.
  [[gnu::always_inline]]
  void store(std::span<Num> span) const noexcept {
    TIT_ASSERT(span.size() >= Size, "Data size is too small!");
    hn::StoreU(base, Tag{}, span.data());
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// SIMD unary plus operation.
  [[gnu::always_inline]]
  friend auto operator+(const Reg& a) noexcept -> Reg {
    return a;
  }

  /// SIMD addition operation.
  [[gnu::always_inline]]
  friend auto operator+(const Reg& a, const Reg& b) noexcept -> Reg {
    return a.base + b.base;
  }

  /// SIMD addition with assignment operation.
  [[gnu::always_inline]]
  friend auto operator+=(Reg& a, const Reg& b) noexcept -> Reg& {
    return a = a + b;
  }

  /// SIMD negation operation.
  [[gnu::always_inline]]
  friend auto operator-(const Reg& a) noexcept -> Reg {
    return hn::Neg(a.base);
  }

  /// SIMD subtraction operation.
  [[gnu::always_inline]]
  friend auto operator-(const Reg& a, const Reg& b) noexcept -> Reg {
    return a.base - b.base;
  }

  /// SIMD subtraction with assignment operation.
  [[gnu::always_inline]]
  friend auto operator-=(Reg& a, const Reg& b) noexcept -> Reg& {
    return a = a - b;
  }

  /// SIMD multiplication operation.
  [[gnu::always_inline]]
  friend auto operator*(const Reg& a, const Reg& b) noexcept -> Reg {
    return a.base * b.base;
  }

  /// SIMD multiplication with assignment operation.
  [[gnu::always_inline]]
  friend auto operator*=(Reg& a, const Reg& b) noexcept -> Reg& {
    return a = a * b;
  }

  /// SIMD division operation.
  [[gnu::always_inline]]
  friend auto operator/(const Reg& a, const Reg& b) noexcept -> Reg {
    return a.base / b.base;
  }

  /// SIMD division with assignment operation.
  [[gnu::always_inline]]
  friend auto operator/=(Reg& a, const Reg& b) noexcept -> Reg& {
    return a = a / b;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// SIMD "equal to" comparison operation.
  [[gnu::always_inline]]
  friend auto operator==(const Reg& a, const Reg& b) noexcept -> RegMask {
    return a.base == b.base;
  }

  /// SIMD "not equal to" comparison operation.
  [[gnu::always_inline]]
  friend auto operator!=(const Reg& a, const Reg& b) noexcept -> RegMask {
    return a.base != b.base;
  }

  /// SIMD "less than" comparison operation.
  [[gnu::always_inline]]
  friend auto operator<(const Reg& a, const Reg& b) noexcept -> RegMask {
    return a.base < b.base;
  }

  /// SIMD "less than or equal to" comparison operation.
  [[gnu::always_inline]]
  friend auto operator<=(const Reg& a, const Reg& b) noexcept -> RegMask {
    return a.base <= b.base;
  }

  /// SIMD "greater than" comparison operation.
  [[gnu::always_inline]]
  friend auto operator>(const Reg& a, const Reg& b) noexcept -> RegMask {
    return a.base > b.base;
  }

  /// SIMD "greater than or equal to" comparison operation.
  [[gnu::always_inline]]
  friend auto operator>=(const Reg& a, const Reg& b) noexcept -> RegMask {
    return a.base >= b.base;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

}; // class Reg

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// SIMD element-wise cast.
template<class To, class From, size_t Size>
  requires castable_to<From, To, Size>
[[gnu::always_inline]]
inline auto reg_cast(const Reg<From, Size>& a) noexcept -> Reg<To, Size> {
  return hn::ConvertTo(typename Reg<To, Size>::Tag{}, a.base);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// SIMD element-wise minimum algorithm.
template<class Num, size_t Size>
  requires supported<Num, Size>
[[gnu::always_inline]]
inline auto min(const Reg<Num, Size>& a, const Reg<Num, Size>& b) noexcept
    -> Reg<Num, Size> {
  return hn::Min(a.base, b.base);
}

/// SIMD element-wise maximum algorithm.
template<class Num, size_t Size>
  requires supported<Num, Size>
[[gnu::always_inline]]
inline auto max(const Reg<Num, Size>& a, const Reg<Num, Size>& b) noexcept
    -> Reg<Num, Size> {
  return hn::Max(a.base, b.base);
}

/// SIMD element-wise filter algorithm.
template<class Num, size_t Size>
  requires supported<Num, Size>
[[gnu::always_inline]]
inline auto filter(const RegMask<Num, Size>& m,
                   const Reg<Num, Size>& a) noexcept -> Reg<Num, Size> {
  return hn::IfThenElseZero(m.base, a.base);
}

/// SIMD element-wise select algorithm.
template<class Num, size_t Size>
  requires supported<Num, Size>
[[gnu::always_inline]]
inline auto select(const RegMask<Num, Size>& m,
                   const Reg<Num, Size>& a,
                   const Reg<Num, Size>& b) noexcept -> Reg<Num, Size> {
  return hn::IfThenElse(m.base, a.base, b.base);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// SIMD `floor` function overload.
template<class Num, size_t Size>
  requires supported<Num, Size>
[[gnu::always_inline]]
inline auto floor(const Reg<Num, Size>& a) noexcept -> Reg<Num, Size> {
  return hn::Floor(a.base);
}

/// SIMD `round` function overload.
template<class Num, size_t Size>
  requires supported<Num, Size>
[[gnu::always_inline]]
inline auto round(const Reg<Num, Size>& a) noexcept -> Reg<Num, Size> {
  return hn::Round(a.base);
}

/// SIMD `ceil` function overload.
template<class Num, size_t Size>
  requires supported<Num, Size>
[[gnu::always_inline]]
inline auto ceil(const Reg<Num, Size>& a) noexcept -> Reg<Num, Size> {
  return hn::Ceil(a.base);
}

/// SIMD fused multiply-add operation.
template<class Num, size_t Size>
  requires supported<Num, Size>
[[gnu::always_inline]]
inline auto fma(const Reg<Num, Size>& a,
                const Reg<Num, Size>& b,
                const Reg<Num, Size>& c) noexcept -> Reg<Num, Size> {
  return hn::MulAdd(a.base, b.base, c.base);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// SIMD horizontal sum reduction.
template<class Num, size_t Size>
  requires supported<Num, Size>
[[gnu::always_inline]]
inline auto sum(const Reg<Num, Size>& a) noexcept -> Num {
  return hn::GetLane(hn::SumOfLanes(typename Reg<Num, Size>::Tag{}, a.base));
}

/// SIMD horizontal minimum reduction.
template<class Num, size_t Size>
  requires supported<Num, Size>
[[gnu::always_inline]]
inline auto min_value(const Reg<Num, Size>& a) noexcept -> Num {
  return hn::GetLane(hn::MinOfLanes(typename Reg<Num, Size>::Tag{}, a.base));
}

/// SIMD horizontal maximum reduction.
template<class Num, size_t Size>
  requires supported<Num, Size>
[[gnu::always_inline]]
inline auto max_value(const Reg<Num, Size>& a) noexcept -> Num {
  return hn::GetLane(hn::MaxOfLanes(typename Reg<Num, Size>::Tag{}, a.base));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::simd
