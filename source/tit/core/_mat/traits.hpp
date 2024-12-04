/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// IWYU pragma: private, include "tit/core/mat.hpp"
#pragma once

#include <concepts>
#include <type_traits>

#include "tit/core/_mat/mat.hpp"
#include "tit/core/basic_types.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace impl {
template<class>
inline constexpr bool is_mat_v = false;
template<class Num, size_t Dim>
inline constexpr bool is_mat_v<Mat<Num, Dim>> = true;
} // namespace impl

/// Is the type a specialization of a matrix type?
template<class T>
inline constexpr bool is_mat_v = impl::is_mat_v<T>;

/// Matrix row type.
template<class Mat>
  requires is_mat_v<Mat>
using mat_row_t = typename Mat::Row;

/// Number type of the matrix.
template<class Mat>
  requires is_mat_v<Mat>
using mat_num_t = std::remove_cvref_t<decltype(Mat{}[0, 0])>;

/// Matrix multiplier type: matrix or vector of the same size.
template<class Mult, class Mat>
concept mat_multiplier = is_mat_v<Mat> && (std::same_as<Mult, Mat> ||
                                           std::same_as<Mult, mat_row_t<Mat>>);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
