/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// IWYU pragma: private, include "tit/core/vec.hpp"
#pragma once

#include <type_traits>

#include "tit/core/_vec/vec.hpp"
#include "tit/core/basic_types.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace impl {
template<class>
inline constexpr bool is_vec_v = false;
template<class Num, size_t Dim>
inline constexpr bool is_vec_v<Vec<Num, Dim>> = true;
} // namespace impl

/// Is the type a specialization of a vector type?
template<class T>
inline constexpr bool is_vec_v = impl::is_vec_v<T>;

/// Number type of the vector.
template<class Vec>
  requires is_vec_v<Vec>
using vec_num_t = std::remove_cvref_t<decltype(Vec{}[0])>;

/// Dimensionality of the vector.
template<class Vec>
  requires is_vec_v<Vec>
inline constexpr size_t vec_dim_v = Vec{}.dim();

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
