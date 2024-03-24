/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <array>
#include <concepts> // IWYU pragma: keep
#include <random>

#include "tit/core/basic_types.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Random engine.
auto get_random_engine() -> std::mt19937&; // NOSONAR

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Random value generator.
template<class T>
struct RandomGenerator;

/// Random boolean generator.
template<>
struct RandomGenerator<bool> {
  static auto operator()() -> bool {
    std::uniform_int_distribution distribution{0, 1};
    return distribution(get_random_engine()) == 1;
  }
}; // struct RandomGenerator<Int>

/// Random integer generator.
template<std::integral Int>
struct RandomGenerator<Int> {
  static auto operator()() -> Int {
    std::uniform_int_distribution<Int> distribution{};
    return distribution(get_random_engine());
  }
}; // struct RandomGenerator<Int>

/// Random float generator.
template<std::floating_point Float>
struct RandomGenerator<Float> {
  static auto operator()() -> Float {
    std::uniform_real_distribution<Float> distribution{};
    return distribution(get_random_engine());
  }
}; // struct RandomGenerator<Float>

/// Random array generator.
template<class T, size_t Size>
struct RandomGenerator<std::array<T, Size>> {
  static auto operator()() -> std::array<T, Size> {
    std::array<T, Size> array{};
    for (auto& elem : array) elem = RandomGenerator<T>{}();
    return array;
  }
}; // struct RandomGenerator<std::array<T, Size>>

/// Generate a random value.
template<class T>
auto gen_val() -> T {
  return RandomGenerator<T>{}();
}

/// Generate a random array.
template<class T, size_t Size>
auto gen_array() -> std::array<T, Size> {
  return RandomGenerator<std::array<T, Size>>{}();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
