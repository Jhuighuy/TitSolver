/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <array>
#include <concepts>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/simd.hpp"
#include "tit/core/uint_utils.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Column vector element-wise boolean mask.
template<class Num, size_t Dim>
class VecMask final {
public:

  /// Size of the underlying register that is used.
  static constexpr auto RegSize = simd::reg_size_for_v<Num, Dim>;

  /// Amount of the underlying registers stored.
  static constexpr auto RegCount = simd::reg_count_for_v<Num, Dim>;

  /// Type of the underlying register that is used.
  using RegMask = simd::RegMask<Num, RegSize>;

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Type of the underlying element mask that is used.
  using Mask = make_bits_t<Num>;

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

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  // NOLINTBEGIN(*-type-union-access)

  /// Fill-initialize the vector mask with false values.
  constexpr VecMask() noexcept {
    TIT_IF_SIMD_AVALIABLE (Num, Dim) {
      regs_ = fill_array<RegCount>(RegMask{});
      return;
    }
    col_ = fill_array<Dim>(to_mask_(false));
  }

  /// Fill-initialize the vector mask with the boolean @p b.
  constexpr explicit(Dim > 1) VecMask(bool b) noexcept {
    auto const q = to_mask_(b);
    TIT_IF_SIMD_AVALIABLE (Num, Dim) {
      regs_ = fill_array<RegCount>(RegMask(q));
      return;
    }
    col_ = fill_array<Dim>(q);
  }

  /// Construct a vector mask with booleans @p bi.
  template<class... Args>
    requires (Dim > 1) && (sizeof...(Args) == Dim) &&
             (std::constructible_from<bool, Args &&> && ...)
  constexpr VecMask(Args... bi) noexcept : col_{to_mask_(bi)...} {} // NOSONAR

  /// Move-construct the vector mask.
  constexpr VecMask(VecMask&&) = default;

  /// Copy-construct the vector mask.
  constexpr VecMask(VecMask const&) = default;

  /// Move-assign the vector mask.
  constexpr auto operator=(VecMask&&) -> VecMask& = default;

  /// Copy-assign the vector mask.
  constexpr auto operator=(VecMask const&) -> VecMask& = default;

  /// Destroy the vector mask.
  constexpr ~VecMask() = default;

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

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

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
//
// Vector mask reduction
//

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
