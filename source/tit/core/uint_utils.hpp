/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <limits>
#include <type_traits>

#include "tit/core/basic_types.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Divide two positive integers and round up the result.
template<std::integral Int>
constexpr auto divide_up(Int n, std::type_identity_t<Int> d) noexcept -> Int {
  return (n + d - Int{1}) / d;
}

/// Align up to a positive integer.
template<std::integral Int>
constexpr auto align_up(Int n,
                        std::type_identity_t<Int> alignment) noexcept -> Int {
  return divide_up(n, alignment) * alignment;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Check if the first integer is larger than the second one.
/// If the integers are equal, check if the tie breaker is true.
template<std::integral Int>
constexpr auto better(Int a, Int b, bool tie_breaker) noexcept -> bool {
  return (a > b) || ((a == b) && tie_breaker);
}

template<std::integral Int>
constexpr auto worse(Int a, Int b, bool tie_breaker) noexcept -> bool {
  return (a < b) || ((a == b) && tie_breaker);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Fast pseudo-random number generator based on the SplitMix64 algorithm.
class SplitMix64 final {
public:

  /// The result type of the generator.
  using result_type = uint64_t;

  /// Construct the generator with a seed.
  constexpr explicit SplitMix64(uint64_t seed) noexcept : seed_{seed} {}

  /// Get the minimum value that the generator can produce.
  static constexpr auto min() noexcept -> uint64_t {
    return 0;
  }

  /// Get the maximum value that the generator can produce.
  static constexpr auto max() noexcept -> uint64_t {
    return std::numeric_limits<uint64_t>::max();
  }

  /// Generate a random number.
  constexpr auto operator()() noexcept -> uint64_t {
    seed_ += 0x9E3779B97F4A7C15;
    auto z = seed_;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EB;
    return z ^ (z >> 31);
  }

private:

  uint64_t seed_;

}; // class SplitMix64

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
