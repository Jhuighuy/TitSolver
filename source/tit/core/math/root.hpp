/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// IWYU pragma: private, include "tit/core/math.hpp"
#pragma once

#include <concepts>
#include <expected>
#include <type_traits>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/math/funcs.hpp"
#include "tit/core/utils.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Newton-Raphson solver error type.
enum class NewtonError : uint8_t {
  no_error,      ///< No error.
  not_converged, ///< Number of iterations exceeded.
  zero_deriv,    ///< Computation reached a zero derivative.
};

/// Newton-Raphson solver result type.
template<class Num>
using NewtonResult = std::expected<Num, NewtonError>;

/// Find function root using the Newton-Raphson method.
template<std::floating_point Num, class Func>
  requires std::invocable<Func&&, Num>
constexpr auto newton(Num x,
                      Func&& f,
                      std::type_identity_t<Num> eps = tiny_number_v<Num>,
                      size_t max_iter = 10) -> NewtonResult<Num> {
  TIT_ASSUME_UNIVERSAL(Func, f);
  for (size_t iter = 0; iter < max_iter; ++iter) {
    const auto [y, df_dx] = std::invoke(f, x);
    if (abs(y) <= eps) return x;
    if (is_tiny(df_dx)) return std::unexpected{NewtonError::zero_deriv};
    x -= y / df_dx;
  }
  return std::unexpected{NewtonError::not_converged};
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Bisection solver error type.
enum class BisectionError : uint8_t {
  no_error,      ///< No error.
  not_converged, ///< Number of iterations exceeded.
  sign,          ///< Function has same signs on the ends of the search range.
};

/// Bisection solver result type.
template<class Num>
using BisectionResult = std::expected<Num, BisectionError>;

/// Find function root using the Bisection method.
template<class Num, class Func>
  requires std::invocable<Func&&, Num>
constexpr auto bisection(Num min_x,
                         Num max_x,
                         Func&& f,
                         std::type_identity_t<Num> eps = tiny_number_v<Num>,
                         size_t max_iter = 10) -> BisectionResult<Num> {
  TIT_ASSUME_UNIVERSAL(Func, f);

  // Check the search bounds first.
  TIT_ASSERT(min_x <= max_x, "Inverted search range!");
  auto min_f = std::invoke(f, min_x);
  if (abs(min_f) <= eps) return min_x;
  auto max_f = std::invoke(f, max_x);
  if (abs(max_f) <= eps) return max_x;

  for (size_t iter = 0; iter < max_iter; ++iter) {
    if (sign(max_f) == sign(min_f)) {
      return std::unexpected{BisectionError::sign};
    }

    // Approximate f(x) with line equation:
    // f(x) = min_f + (max_f - min_f)/(max_x - min_x) * (x - min_x),
    // therefore, approximate root of f(x) == 0 is:
    const auto x = min_x - min_f * (max_x - min_x) / (max_f - min_f);
    const auto y = std::invoke(f, x);
    if (abs(y) <= eps) return x;

    // Update the search bounds.
    const auto sign_y = sign(y);
    if (sign_y != sign(min_f)) max_x = x, max_f = y;
    else if (sign_y != sign(max_f)) min_x = x, min_f = y;
  }

  return std::unexpected{BisectionError::not_converged};
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
