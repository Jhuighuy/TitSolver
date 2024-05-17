/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <array>
#include <concepts>
#include <utility>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/math.hpp"
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

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Vector element-wise "equal to" comparison boolean mask.
template<class Num, size_t Dim>
constexpr auto operator==(Vec<Num, Dim> const& a,
                          Vec<Num, Dim> const& b) -> VecMask<Num, Dim> {
  VecMask<Num, Dim> m;
  for (size_t i = 0; i < Dim; ++i) m[i] = a[i] == b[i];
  return m;
}

/// Vector element-wise "not equal to" comparison boolean mask.
template<class Num, size_t Dim>
constexpr auto operator!=(Vec<Num, Dim> const& a,
                          Vec<Num, Dim> const& b) -> VecMask<Num, Dim> {
  VecMask<Num, Dim> m;
  for (size_t i = 0; i < Dim; ++i) m[i] = a[i] != b[i];
  return m;
}

/// Vector element-wise "less than" comparison boolean mask.
template<class Num, size_t Dim>
constexpr auto operator<(Vec<Num, Dim> const& a,
                         Vec<Num, Dim> const& b) -> VecMask<Num, Dim> {
  VecMask<Num, Dim> m;
  for (size_t i = 0; i < Dim; ++i) m[i] = a[i] < b[i];
  return m;
}

/// Vector element-wise "less than or equal to" comparison boolean mask.
template<class Num, size_t Dim>
constexpr auto operator<=(Vec<Num, Dim> const& a,
                          Vec<Num, Dim> const& b) -> VecMask<Num, Dim> {
  VecMask<Num, Dim> m;
  for (size_t i = 0; i < Dim; ++i) m[i] = a[i] <= b[i];
  return m;
}

/// Vector element-wise "greater than" comparison boolean mask.
template<class Num, size_t Dim>
constexpr auto operator>(Vec<Num, Dim> const& a,
                         Vec<Num, Dim> const& b) -> VecMask<Num, Dim> {
  VecMask<Num, Dim> m;
  for (size_t i = 0; i < Dim; ++i) m[i] = a[i] > b[i];
  return m;
}

/// Vector element-wise "greater than or equal to" comparison boolean mask.
template<class Num, size_t Dim>
constexpr auto operator>=(Vec<Num, Dim> const& a,
                          Vec<Num, Dim> const& b) -> VecMask<Num, Dim> {
  VecMask<Num, Dim> m;
  for (size_t i = 0; i < Dim; ++i) m[i] = a[i] >= b[i];
  return m;
}

/// Vector element-wise "approximately equal to" comparison boolean mask.
template<class Num, size_t Dim>
constexpr auto approx_equal_to(Vec<Num, Dim> const& a,
                               Vec<Num, Dim> const& b) -> VecMask<Num, Dim> {
  VecMask<Num, Dim> m;
  for (size_t i = 0; i < Dim; ++i) m[i] = approx_equal_to(a[i], b[i]);
  return m;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Blend vector and a zero vector based on a boolean mask.
template<class Num, size_t Dim>
constexpr auto filter(VecMask<Num, Dim> const& m,
                      Vec<Num, Dim> const& a) -> Vec<Num, Dim> {
  Vec<Num, Dim> r;
  for (size_t i = 0; i < Dim; ++i) r[i] = m[i] ? a[i] : Num{0};
  return r;
}

/// Blend two vectors based on a boolean mask.
template<class Num, size_t Dim>
constexpr auto select(VecMask<Num, Dim> const& m,
                      Vec<Num, Dim> const& a,
                      Vec<Num, Dim> const& b) -> Vec<Num, Dim> {
  Vec<Num, Dim> r;
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
