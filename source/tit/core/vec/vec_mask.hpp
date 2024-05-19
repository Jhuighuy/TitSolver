/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// IWYU pragma: private, include "tit/core/vec.hpp"
#pragma once

#include <array>
#include <concepts>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/utils.hpp"

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
  constexpr VecMask(Args&&... bs) // NOSONAR
      : col_{make_array<Dim, bool>(std::forward<Args>(bs)...)} {}

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

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Logical operations
//

/// Vector mask element-wise logical negation operation.
template<class Num, size_t Dim>
constexpr auto operator!(const VecMask<Num, Dim>& m) -> VecMask<Num, Dim> {
  VecMask<Num, Dim> r;
  for (size_t i = 0; i < Dim; ++i) r[i] = !m[i];
  return r;
}

/// Vector mask element-wise conjunction operation.
template<class Num, size_t Dim>
constexpr auto operator&&(const VecMask<Num, Dim>& m,
                          const VecMask<Num, Dim>& n) -> VecMask<Num, Dim> {
  VecMask<Num, Dim> r;
  for (size_t i = 0; i < Dim; ++i) r[i] = m[i] && n[i];
  return r;
}

/// Vector mask element-wise disjunction operation.
template<class Num, size_t Dim>
constexpr auto operator||(const VecMask<Num, Dim>& m,
                          const VecMask<Num, Dim>& n) -> VecMask<Num, Dim> {
  VecMask<Num, Dim> r;
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
  for (size_t i = 0; i < Dim; ++i) r[i] = m[i] == n[i];
  return r;
}

/// Vector element-wise "not equal to" comparison operation.
template<class Num, size_t Dim>
constexpr auto operator!=(const VecMask<Num, Dim>& m,
                          const VecMask<Num, Dim>& n) -> VecMask<Num, Dim> {
  VecMask<Num, Dim> r;
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
  for (size_t i = 0; i < Dim; ++i) {
    if (m[i]) return true;
  }
  return false;
}

/// Check if all vector mask elements are set to true.
template<class Num, size_t Dim>
constexpr auto all(const VecMask<Num, Dim>& m) -> bool {
  for (size_t i = 0; i < Dim; ++i) {
    if (!m[i]) return false;
  }
  return true;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
