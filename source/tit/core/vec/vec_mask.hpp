/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <array>
#include <concepts> // IWYU pragma: keep
#include <utility>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/math.hpp"
#include "tit/core/simd.hpp"
#include "tit/core/uint_utils.hpp"
#include "tit/core/utils.hpp"
#include "tit/core/vec/vec.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Column vector element-wise boolean mask.
template<class Num, size_t Dim>
class VecMask final {
public:

  /// Fill-initialize the vector mask with false values.
  constexpr VecMask() noexcept : VecMask(false) {}

  /// Fill-initialize the vector mask.
  constexpr explicit(Dim > 1) VecMask(bool b) noexcept
      : col_{fill_array<Dim>(b)} {}

  /// Construct a vector mask with elements @p bi.
  template<class... Args>
    requires (Dim > 1) && (sizeof...(Args) == Dim) &&
             (std::constructible_from<bool, Args &&> && ...)
  constexpr VecMask(Args&&... qi) // NOSONAR
      : col_{std::forward<Args>(qi)...} {}

  /// Element at index.
  /// @{
  constexpr auto operator[](size_t i) noexcept -> bool& {
    TIT_ASSERT(i < Dim, "Row index is out of range!");
    return col_[i];
  }
  constexpr auto operator[](size_t i) const noexcept -> bool {
    TIT_ASSERT(i < Dim, "Row index is out of range!");
    return col_[i];
  }
  /// @}

private:

  std::array<bool, Dim> col_;

}; // class VecMask

/// Column vector element-wise boolean mask (SIMD specialization).
template<class Num, size_t Dim>
  requires simd::available_for<Num, Dim>
class VecMask<Num, Dim> final {
public:

  /// Size of the underlying register that is used.
  static constexpr auto RegSize = simd::reg_size_for_v<Num, Dim>;

  /// Amount of the underlying registers stored.
  static constexpr auto RegCount = simd::reg_count_for_v<Num, Dim>;

  /// Type of the underlying element mask that is used.
  using Mask = make_bits_t<Num>;

  /// Type of the underlying register that is used.
  using RegMask = simd::RegMask<Num, RegSize>;

  /// Mask reference wrapper type.
  class BoolRef final {
  public:

    /// Construct a mask reference.
    constexpr BoolRef(Mask& m) noexcept : mask_(&m) {} // NOSONAR

    /// Convert to mask value to a boolean.
    constexpr operator bool() const noexcept { // NOSONAR
      TIT_ASSERT(mask_ != nullptr, "Invalid mask pointer!");
      return *mask_ != Mask{0};
    }

    /// Assign mask value.
    constexpr auto operator=(bool b) noexcept -> BoolRef& {
      TIT_ASSERT(mask_ != nullptr, "Invalid mask pointer!");
      *mask_ = to_mask_(b);
      return *this;
    }

  private:

    Mask* mask_;

  }; // class BoolRef

  // NOLINTBEGIN(*-type-union-access)

  /// Fill-initialize the vector mask with false values.
  constexpr VecMask() noexcept {
    if consteval {
      col_ = fill_array<Dim>(to_mask_(false));
    } else {
      regs_ = fill_array<RegCount>(RegMask(zeroinit));
    }
  }

  /// Fill-initialize the vector mask with the boolean @p b.
  constexpr explicit(Dim > 1) VecMask(bool b) noexcept {
    auto const q = to_mask_(b);
    if consteval {
      col_ = fill_array<Dim>(q);
    } else {
      regs_ = fill_array<RegCount>(RegMask(q));
    }
  }

  /// Construct a vector mask with booleans @p bi.
  template<class... Args>
    requires (Dim > 1) && (sizeof...(Args) == Dim) &&
             (std::constructible_from<bool, Args> && ...)
  constexpr VecMask(Args... bi) noexcept { // NOSONAR
    if consteval {
      col_ = {to_mask_(bi)...};
    } else {
      auto const qs = make_array<RegSize * RegCount, Mask>(to_mask_(bi)...);
      auto const pack = [&]<size_t RegIndex>(std::index_sequence<RegIndex>) {
        return [&]<size_t... MaskIndices>(std::index_sequence<MaskIndices...>) {
          return RegMask{qs[RegIndex * RegSize + MaskIndices]...};
        }(std::make_index_sequence<RegSize>{});
      };
      [&]<size_t... RegIndices>(std::index_sequence<RegIndices...>) {
        regs_ = {pack(std::index_sequence<RegIndices>{})...};
      }(std::make_index_sequence<RegCount>{});
    }
  }

  /// Element at index.
  /// @{
  constexpr auto operator[](size_t i) noexcept -> BoolRef {
    TIT_ASSERT(i < Dim, "Row index is out of range.");
    return col_[i];
  }
  constexpr auto operator[](size_t i) const noexcept -> bool {
    TIT_ASSERT(i < Dim, "Row index is out of range.");
    return col_[i] != Mask{0};
  }
  /// @}

  /// Underlying register at index.
  /// @{
  auto reg(size_t i) noexcept -> RegMask& {
    TIT_ASSERT(i < RegCount, "Register index is out of range.");
    return regs_[i];
  }
  auto reg(size_t i) const noexcept -> RegMask const& {
    TIT_ASSERT(i < RegCount, "Register index is out of range.");
    return regs_[i];
  }
  /// @}

  // NOLINTEND(*-type-union-access)

private:

  static constexpr auto to_mask_(bool b) noexcept -> Mask {
    return b ? ~Mask{0} : Mask{0};
  }

  union {
    std::array<Mask, Dim> col_;
    std::array<RegMask, RegCount> regs_;
  };

}; // class VecMask

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Vector element-wise "equal to" comparison boolean mask.
template<class Num, size_t Dim>
constexpr auto operator==(Vec<Num, Dim> const& a, Vec<Num, Dim> const& b)
    -> VecMask<Num, Dim> {
  VecMask<Num, Dim> m;
  TIT_IF_SIMD_AVALIABLE (Num, Dim) {
    constexpr auto RegCount = Vec<Num, Dim>::RegCount;
    for (size_t i = 0; i < RegCount; ++i) m.reg(i) = a.reg(i) == b.reg(i);
    return m;
  }
  for (size_t i = 0; i < Dim; ++i) m[i] = a[i] == b[i];
  return m;
}

/// Vector element-wise "not equal to" comparison boolean mask.
template<class Num, size_t Dim>
constexpr auto operator!=(Vec<Num, Dim> const& a, Vec<Num, Dim> const& b)
    -> VecMask<Num, Dim> {
  VecMask<Num, Dim> m;
  TIT_IF_SIMD_AVALIABLE (Num, Dim) {
    constexpr auto RegCount = Vec<Num, Dim>::RegCount;
    for (size_t i = 0; i < RegCount; ++i) m.reg(i) = a.reg(i) != b.reg(i);
    return m;
  }
  for (size_t i = 0; i < Dim; ++i) m[i] = a[i] != b[i];
  return m;
}

/// Vector element-wise "less than" comparison boolean mask.
template<class Num, size_t Dim>
constexpr auto operator<(Vec<Num, Dim> const& a, Vec<Num, Dim> const& b)
    -> VecMask<Num, Dim> {
  VecMask<Num, Dim> m;
  TIT_IF_SIMD_AVALIABLE (Num, Dim) {
    constexpr auto RegCount = Vec<Num, Dim>::RegCount;
    for (size_t i = 0; i < RegCount; ++i) m.reg(i) = a.reg(i) < b.reg(i);
    return m;
  }
  for (size_t i = 0; i < Dim; ++i) m[i] = a[i] < b[i];
  return m;
}

/// Vector element-wise "less than or equal to" comparison boolean mask.
template<class Num, size_t Dim>
constexpr auto operator<=(Vec<Num, Dim> const& a, Vec<Num, Dim> const& b)
    -> VecMask<Num, Dim> {
  VecMask<Num, Dim> m;
  TIT_IF_SIMD_AVALIABLE (Num, Dim) {
    constexpr auto RegCount = Vec<Num, Dim>::RegCount;
    for (size_t i = 0; i < RegCount; ++i) m.reg(i) = a.reg(i) <= b.reg(i);
    return m;
  }
  for (size_t i = 0; i < Dim; ++i) m[i] = a[i] <= b[i];
  return m;
}

/// Vector element-wise "greater than" comparison boolean mask.
template<class Num, size_t Dim>
constexpr auto operator>(Vec<Num, Dim> const& a, Vec<Num, Dim> const& b)
    -> VecMask<Num, Dim> {
  VecMask<Num, Dim> m;
  TIT_IF_SIMD_AVALIABLE (Num, Dim) {
    constexpr auto RegCount = Vec<Num, Dim>::RegCount;
    for (size_t i = 0; i < RegCount; ++i) m.reg(i) = a.reg(i) > b.reg(i);
    return m;
  }
  for (size_t i = 0; i < Dim; ++i) m[i] = a[i] > b[i];
  return m;
}

/// Vector element-wise "greater than or equal to" comparison boolean mask.
template<class Num, size_t Dim>
constexpr auto operator>=(Vec<Num, Dim> const& a, Vec<Num, Dim> const& b)
    -> VecMask<Num, Dim> {
  VecMask<Num, Dim> m;
  TIT_IF_SIMD_AVALIABLE (Num, Dim) {
    constexpr auto RegCount = Vec<Num, Dim>::RegCount;
    for (size_t i = 0; i < RegCount; ++i) m.reg(i) = a.reg(i) >= b.reg(i);
    return m;
  }
  for (size_t i = 0; i < Dim; ++i) m[i] = a[i] >= b[i];
  return m;
}

/// Vector element-wise "approximately equal to" comparison boolean mask.
template<class Num, size_t Dim>
constexpr auto approx_equal_to(Vec<Num, Dim> const& a, Vec<Num, Dim> const& b)
    -> VecMask<Num, Dim> {
  VecMask<Num, Dim> m;
  TIT_IF_SIMD_AVALIABLE (Num, Dim) {
    typename Vec<Num, Dim>::Reg const eps_reg(small_number_v<Num>);
    constexpr auto RegCount = Vec<Num, Dim>::RegCount;
    for (size_t i = 0; i < RegCount; ++i) {
      m.reg(i) = abs_delta(a.reg(i), b.reg(i)) <= eps_reg;
    }
    return m;
  }
  for (size_t i = 0; i < Dim; ++i) m[i] = approx_equal_to(a[i], b[i]);
  return m;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Blend vector and a zero vector based on a boolean mask.
template<class Num, size_t Dim>
constexpr auto blend_zero(VecMask<Num, Dim> const& m, Vec<Num, Dim> const& a)
    -> Vec<Num, Dim> {
  Vec<Num, Dim> r;
  TIT_IF_SIMD_AVALIABLE (Num, Dim) {
    constexpr auto RegCount = Vec<Num, Dim>::RegCount;
    for (size_t i = 0; i < RegCount; ++i) {
      r.reg(i) = blend_zero(m.reg(i), a.reg(i));
    }
    return r;
  }
  for (size_t i = 0; i < Dim; ++i) r[i] = m[i] ? a[i] : Num{0};
  return r;
}

/// Blend two vectors based on a boolean mask.
template<class Num, size_t Dim>
constexpr auto blend(VecMask<Num, Dim> const& m, //
                     Vec<Num, Dim> const& a, Vec<Num, Dim> const& b)
    -> Vec<Num, Dim> {
  Vec<Num, Dim> r;
  TIT_IF_SIMD_AVALIABLE (Num, Dim) {
    constexpr auto RegCount = Vec<Num, Dim>::RegCount;
    for (size_t i = 0; i < RegCount; ++i) {
      r.reg(i) = blend(m.reg(i), a.reg(i), b.reg(i));
    }
    return r;
  }
  for (size_t i = 0; i < Dim; ++i) r[i] = m[i] ? a[i] : b[i];
  return r;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Check if any vector mask element is set to true.
template<class Num, size_t Dim>
constexpr auto any(VecMask<Num, Dim> const& m) noexcept -> bool {
  for (size_t i = 0; i < Dim; ++i) {
    if (m[i]) return true;
  }
  return false;
}

/// Check if all vector mask elements are set to true.
template<class Num, size_t Dim>
constexpr auto all(VecMask<Num, Dim> const& m) noexcept -> bool {
  for (size_t i = 0; i < Dim; ++i) {
    if (!m[i]) return false;
  }
  return true;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
