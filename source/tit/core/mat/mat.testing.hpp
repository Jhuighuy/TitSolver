/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include "tit/core/basic_types.hpp"
#include "tit/core/mat/mat.hpp"
#include "tit/core/math.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// For consistency with vector comparison.
constexpr auto all(bool b) noexcept -> bool {
  return b;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Matrix exact equality operator.
template<class Num, size_t Dim>
constexpr auto operator==(Mat<Num, Dim> const& a,
                          Mat<Num, Dim> const& b) noexcept -> bool {
  for (size_t i = 0; i < Dim; ++i) {
    for (size_t j = 0; j < Dim; ++j) {
      if (a[i, j] != b[i, j]) return false;
    }
  }
  return true;
}

// Matrix approximate equality operator.
template<class Num, size_t Dim>
constexpr auto approx_equal_to(Mat<Num, Dim> const& a,
                               Mat<Num, Dim> const& b) noexcept -> bool {
  for (size_t i = 0; i < Dim; ++i) {
    for (size_t j = 0; j < Dim; ++j) {
      if (!approx_equal_to(a[i, j], b[i, j])) return false;
    }
  }
  return true;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
