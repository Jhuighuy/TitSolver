/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <array>
#include <cstddef>
#include <optional>
#include <ranges>
#include <type_traits>
#include <utility>

#include "tit/core/assert.hpp"
#include "tit/core/mat.hpp"
#include "tit/core/math.hpp"
#include "tit/core/vec.hpp"

namespace tit::sph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Dirichlet boundary value.
template<class Value>
constexpr auto dirichlet_value(const Value& value) noexcept -> Value {
  return value;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Normalized wall extrapolation for a homogeneous Neumann condition.
template<std::ranges::input_range Samples,
         class WeightFunc,
         class ValueFunc,
         class Value>
constexpr auto homogeneous_neumann_extrapolate(Samples&& samples,
                                               WeightFunc&& weight_func,
                                               ValueFunc&& value_func,
                                               const Value& fallback) -> Value {
  using ValueT = std::remove_cvref_t<Value>;
  using Num = std::remove_cvref_t<decltype(std::declval<WeightFunc&>()(
      *std::ranges::begin(samples)))>;

  Num weight_sum{};
  ValueT value_sum{};
  for (auto&& sample : samples) {
    const auto weight = weight_func(sample);
    if (weight <= Num{} || is_tiny(weight)) continue;
    value_sum += weight * value_func(sample);
    weight_sum += weight;
  }
  if (is_tiny(weight_sum)) return fallback;
  return value_sum / weight_sum;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace impl {

template<std::size_t Order, class Num>
constexpr auto robin_basis(Num x, Num k, Num lambda) -> Vec<Num, Order> {
  static_assert(Order > 0);
  TIT_ASSERT(!is_tiny(k), "Robin extrapolation coefficient must be non-zero!");

  Vec<Num, Order> basis{};
  basis[0] = Num{1} - lambda / k * x;

  auto x_power = x;
  for (std::size_t i = 1; i < Order; ++i) {
    x_power *= x;
    basis[i] = x_power;
  }
  return basis;
}

template<std::size_t Order,
         class Value,
         std::ranges::input_range Samples,
         class XFunc,
         class WeightFunc,
         class ValueFunc,
         class Num,
         class Psi>
constexpr auto try_robin_extrapolate(Samples&& samples,
                                     XFunc&& x_func,
                                     WeightFunc&& weight_func,
                                     ValueFunc&& value_func,
                                     Num k,
                                     Num lambda,
                                     const Psi& psi) {
  using ValueT = std::remove_cvref_t<Value>;

  Mat<Num, Order> normal_matrix{};
  std::array<ValueT, Order> rhs{};
  std::size_t num_samples{};

  for (auto&& sample : samples) {
    const auto weight = weight_func(sample);
    if (weight <= Num{} || is_tiny(weight)) continue;

    const auto x = x_func(sample);
    const auto basis = robin_basis<Order>(x, k, lambda);
    const ValueT value = value_func(sample) - ValueT{psi / k * x};

    for (std::size_t i = 0; i < Order; ++i) {
      rhs[i] += weight * basis[i] * value;
      for (std::size_t j = 0; j < Order; ++j) {
        normal_matrix[i, j] += weight * basis[i] * basis[j];
      }
    }
    ++num_samples;
  }

  if (num_samples < Order) return std::optional<ValueT>{};
  const auto fact = lu(normal_matrix);
  if (!fact) return std::optional<ValueT>{};

  const auto inv_normal_matrix = fact->inverse();
  ValueT beta_0{};
  for (std::size_t j = 0; j < Order; ++j) {
    beta_0 += inv_normal_matrix[0, j] * rhs[j];
  }
  return std::optional<ValueT>{beta_0};
}

} // namespace impl

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Robin/Neumann boundary extrapolation using a local normal least-squares fit.
template<std::size_t Order = 2,
         std::ranges::forward_range Samples,
         class XFunc,
         class WeightFunc,
         class ValueFunc,
         class Num,
         class Psi,
         class Value>
constexpr auto robin_extrapolate(Samples&& samples,
                                 XFunc&& x_func,
                                 WeightFunc&& weight_func,
                                 ValueFunc&& value_func,
                                 Num k,
                                 Num lambda,
                                 const Psi& psi,
                                 const Value& fallback) -> Value {
  static_assert(Order > 0);
  if (const auto value = impl::try_robin_extrapolate<Order, Value>(samples,
                                                                   x_func,
                                                                   weight_func,
                                                                   value_func,
                                                                   k,
                                                                   lambda,
                                                                   psi)) {
    return *value;
  }
  if constexpr (Order > 1) {
    return robin_extrapolate<1>(samples,
                                x_func,
                                weight_func,
                                value_func,
                                k,
                                lambda,
                                psi,
                                fallback);
  } else {
    return fallback;
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
