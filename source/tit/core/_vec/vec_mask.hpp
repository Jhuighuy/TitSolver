/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// IWYU pragma: private, include "tit/core/vec.hpp"
#pragma once

#include <array>
#include <concepts>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/simd.hpp"
#include "tit/core/utils.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Vector mask class.
//

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
  constexpr VecMask(Args&&... bs) // NOSONAR
      : col_{make_array<Dim, bool>(std::forward<Args>(bs)...)} {}

  /// Vector mask element at index.
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

  /// Check if all elements are true.
  constexpr operator bool() const noexcept {
    return all(*this);
  }

private:

  std::array<bool, Dim> col_;

}; // class VecMask

/// Column vector element-wise boolean mask with SIMD support.
template<simd::supported_type Num, size_t Dim>
class VecMask<Num, Dim> final {
public:

  /// Type of the underlying element mask that is used.
  using Mask = simd::Mask<Num>;

  /// Type of the underlying register that is used.
  using RegMask = simd::deduce_reg_mask_t<Num, Dim>;

  /// Size of the underlying register that is used.
  static constexpr auto RegSize = simd::deduce_size_v<Num, Dim>;

  /// Amount of the underlying registers stored.
  static constexpr auto RegCount = simd::deduce_count_v<Num, Dim>;

  // NOLINTBEGIN(*-type-union-access)

  /// Fill-initialize the vector mask with false values.
  constexpr VecMask() {
    if consteval {
      col_ = fill_array<Dim>(Mask{});
    } else {
      regs_ = fill_array<RegCount>(RegMask{});
    }
  }

  /// Fill-initialize the vector mask with the boolean @p b.
  constexpr explicit(Dim > 1) VecMask(bool b) {
    if consteval {
      col_ = fill_array<Dim>(Mask(b));
    } else {
      regs_ = fill_array<RegCount>(RegMask(b));
    }
  }

  /// Construct a vector mask with elements @p bi.
  template<class... Args>
    requires (Dim > 1) && (sizeof...(Args) == Dim) &&
             (std::constructible_from<bool, Args &&> && ...)
  constexpr VecMask(Args&&... bs) // NOSONAR
      : col_{make_array<Dim, Mask>(std::forward<Args>(bs)...)} {}

  /// Move-construct the vector mask.
  constexpr VecMask(VecMask&& other) noexcept {
    if consteval {
      col_ = std::move(other.col_);
    } else {
      regs_ = std::move(other.regs_);
    }
  }

  /// Move-assign the vector mask.
  constexpr auto operator=(VecMask&& other) noexcept -> VecMask& {
    if consteval {
      col_ = std::move(other.col_);
    } else {
      regs_ = std::move(other.regs_);
    }
    return *this;
  }

  /// Copy-construct the vector mask.
  constexpr VecMask(const VecMask& other) {
    if consteval {
      col_ = other.col_;
    } else {
      regs_ = other.regs_;
    }
  }

  /// Copy-assign the vector mask.
  // NOLINTNEXTLINE(cert-oop54-cpp)
  constexpr auto operator=(const VecMask& other) -> VecMask& {
    if consteval {
      col_ = other.col_;
    } else {
      regs_ = other.regs_;
    }
    return *this;
  }

  /// Destroy the vector mask.
  constexpr ~VecMask() = default;

  /// Vector mask element at index.
  /// @{
  constexpr auto operator[](size_t i) noexcept -> Mask& {
    TIT_ASSERT(i < Dim, "Row index is out of range.");
    return col_[i];
  }
  constexpr auto operator[](size_t i) const noexcept -> const Mask& {
    TIT_ASSERT(i < Dim, "Row index is out of range.");
    return col_[i];
  }
  /// @}

  /// Underlying register at index.
  /// @{
  auto reg(size_t i) noexcept -> RegMask& {
    TIT_ASSERT(i < RegCount, "Register index is out of range.");
    return regs_[i];
  }
  auto reg(size_t i) const noexcept -> const RegMask& {
    TIT_ASSERT(i < RegCount, "Register index is out of range.");
    return regs_[i];
  }
  /// @}

  // NOLINTEND(*-type-union-access)

  /// Check if all elements are true.
  constexpr operator bool() const noexcept {
    return all(*this);
  }

private:

  union {
    std::array<Mask, Dim> col_{};
    std::array<RegMask, RegCount> regs_;
  };

}; // class VecMask<SIMD>

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Logical operations
//

/// Vector mask element-wise logical negation operation.
template<class Num, size_t Dim>
constexpr auto operator!(const VecMask<Num, Dim>& m) -> VecMask<Num, Dim> {
  VecMask<Num, Dim> r;
  TIT_IF_SIMD_AVALIABLE(Num) {
    for (size_t i = 0; i < VecMask<Num, Dim>::RegCount; ++i) {
      r.reg(i) = !m.reg(i);
    }
    return r;
  }
  for (size_t i = 0; i < Dim; ++i) r[i] = !m[i];
  return r;
}

/// Vector mask element-wise conjunction operation.
template<class Num, size_t Dim>
constexpr auto operator&&(const VecMask<Num, Dim>& m,
                          const VecMask<Num, Dim>& n) -> VecMask<Num, Dim> {
  VecMask<Num, Dim> r;
  TIT_IF_SIMD_AVALIABLE(Num) {
    for (size_t i = 0; i < VecMask<Num, Dim>::RegCount; ++i) {
      r.reg(i) = m.reg(i) && n.reg(i);
    }
    return r;
  }
  for (size_t i = 0; i < Dim; ++i) r[i] = m[i] && n[i];
  return r;
}

/// Vector mask element-wise disjunction operation.
template<class Num, size_t Dim>
constexpr auto operator||(const VecMask<Num, Dim>& m,
                          const VecMask<Num, Dim>& n) -> VecMask<Num, Dim> {
  VecMask<Num, Dim> r;
  TIT_IF_SIMD_AVALIABLE(Num) {
    for (size_t i = 0; i < VecMask<Num, Dim>::RegCount; ++i) {
      r.reg(i) = m.reg(i) || n.reg(i);
    }
    return r;
  }
  for (size_t i = 0; i < Dim; ++i) r[i] = m[i] || n[i];
  return r;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Comparison operations
//

/// Vector mask element-wise "equal to" comparison operation.
template<class Num, size_t Dim>
constexpr auto operator==(const VecMask<Num, Dim>& m,
                          const VecMask<Num, Dim>& n) -> VecMask<Num, Dim> {
  VecMask<Num, Dim> r;
  TIT_IF_SIMD_AVALIABLE(Num) {
    for (size_t i = 0; i < VecMask<Num, Dim>::RegCount; ++i) {
      r.reg(i) = m.reg(i) == n.reg(i);
    }
    return r;
  }
  for (size_t i = 0; i < Dim; ++i) r[i] = m[i] == n[i];
  return r;
}

/// Vector element-wise "not equal to" comparison operation.
template<class Num, size_t Dim>
constexpr auto operator!=(const VecMask<Num, Dim>& m,
                          const VecMask<Num, Dim>& n) -> VecMask<Num, Dim> {
  VecMask<Num, Dim> r;
  TIT_IF_SIMD_AVALIABLE(Num) {
    for (size_t i = 0; i < VecMask<Num, Dim>::RegCount; ++i) {
      r.reg(i) = m.reg(i) != n.reg(i);
    }
    return r;
  }
  for (size_t i = 0; i < Dim; ++i) r[i] = m[i] != n[i];
  return r;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Reduction
//

/// Check if any vector mask element is set to true.
template<class Num, size_t Dim>
constexpr auto any(const VecMask<Num, Dim>& m) -> bool {
  TIT_IF_SIMD_AVALIABLE(Num) {
    constexpr auto RegSize = VecMask<Num, Dim>::RegSize;
    constexpr auto FullRegCount = Dim / RegSize;
    if constexpr (FullRegCount > 0) {
      auto r_reg = m.reg(0);
      for (size_t i = 1; i < FullRegCount; ++i) r_reg = r_reg || m.reg(i);
      if (simd::any(r_reg)) return true;
      for (size_t i = FullRegCount * RegSize; i < Dim; ++i) {
        if (m[i]) return true;
      }
      return false;
    }
  }
  for (size_t i = 0; i < Dim; ++i) {
    if (m[i]) return true;
  }
  return false;
}

/// Check if all vector mask elements are set to true.
template<class Num, size_t Dim>
constexpr auto all(const VecMask<Num, Dim>& m) -> bool {
  TIT_IF_SIMD_AVALIABLE(Num) {
    constexpr auto RegSize = VecMask<Num, Dim>::RegSize;
    constexpr auto FullRegCount = Dim / RegSize;
    if constexpr (FullRegCount > 0) {
      auto r_reg = m.reg(0);
      for (size_t i = 1; i < FullRegCount; ++i) r_reg = r_reg && m.reg(i);
      if (!simd::all(r_reg)) return false;
      for (size_t i = FullRegCount * RegSize; i < Dim; ++i) {
        if (!m[i]) return false;
      }
      return true;
    }
  }
  for (size_t i = 0; i < Dim; ++i) {
    if (!m[i]) return false;
  }
  return true;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Search
//

/// Count the number of true values in the vector mask.
template<class Num, size_t Dim>
constexpr auto count_true(const VecMask<Num, Dim>& m) -> size_t {
  TIT_IF_SIMD_AVALIABLE(Num) {
    constexpr auto RegSize = VecMask<Num, Dim>::RegSize;
    constexpr auto FullRegCount = Dim / RegSize;
    if constexpr (FullRegCount > 0) {
      auto count = simd::count_true(m.reg(0));
      for (size_t i = 1; i < FullRegCount; ++i) {
        count += simd::count_true(m.reg(i));
      }
      for (size_t i = FullRegCount * RegSize; i < Dim; ++i) {
        if (m[i]) count += 1;
      }
      return count;
    }
  }
  size_t count = 0;
  for (size_t i = 0; i < Dim; ++i) {
    if (m[i]) count += 1;
  }
  return count;
}

/// Try to find the first true value in the vector mask.
/// Return `-1` if all values are false.
template<class Num, size_t Dim>
constexpr auto try_find_true(const VecMask<Num, Dim>& m) -> ssize_t {
  TIT_IF_SIMD_AVALIABLE(Num) {
    constexpr auto RegSize = VecMask<Num, Dim>::RegSize;
    constexpr auto FullRegCount = Dim / RegSize;
    if constexpr (FullRegCount > 0) {
      for (size_t i = 0; i < FullRegCount; ++i) {
        const auto true_index = simd::try_find_true(m.reg(i));
        if (true_index != -1) return true_index + i * RegSize;
      }
      for (size_t i = FullRegCount * RegSize; i < Dim; ++i) {
        if (m[i]) return static_cast<ssize_t>(i);
      }
      return -1;
    }
  }
  for (size_t i = 0; i < Dim; ++i) {
    if (m[i]) return static_cast<ssize_t>(i);
  }
  return -1;
}

/// Find the first true value in the vector mask.
template<class Num, size_t Dim>
constexpr auto find_true(const VecMask<Num, Dim>& m) -> size_t {
  TIT_ASSERT(count_true(m) > 0, "No true value in the vector mask!");
  TIT_IF_SIMD_AVALIABLE(Num) {
    constexpr auto RegSize = VecMask<Num, Dim>::RegSize;
    constexpr auto FullRegCount = Dim / RegSize;
    constexpr auto RemainderSize = Dim - RegSize * FullRegCount;
    if constexpr (FullRegCount > 0 && RemainderSize == 0) {
      for (size_t i = 0; i < FullRegCount - 1; ++i) {
        const auto true_index = simd::try_find_true(m.reg(i));
        if (true_index != -1) return true_index + i * RegSize;
      }
      constexpr auto LastReg = FullRegCount - 1;
      return RegSize * LastReg + simd::find_true(m.reg(LastReg));
    }
  }
  return try_find_true(m);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
