/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <limits>
#include <random>
#include <type_traits>

#include "tit/core/basic_types.hpp"
#include "tit/core/utils.hpp"

namespace tit {

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

/// Check if the first argument is smaller than the second one.
/// If the arguments are equal, return the third argument.
template<std::totally_ordered Arg>
constexpr auto less_or(Arg a, Arg b, bool otherwise) noexcept -> bool {
  return (a < b) || ((a == b) && otherwise);
}

/// Check if the first argument is smaller than the second one.
/// If the arguments are equal, return random boolean value.
template<std::totally_ordered Arg, class RNG>
  requires std::uniform_random_bit_generator<std::remove_reference_t<RNG>>
constexpr auto less_or(Arg a, Arg b, RNG&& rng) noexcept -> bool {
  TIT_ASSUME_UNIVERSAL(RNG, rng);
  return less_or(a, b, rng() % 2 == 1);
}

/// Check if the first argument is larger than the second one.
/// If the arguments are equal, return the third argument.
template<std::totally_ordered Arg>
constexpr auto greater_or(Arg a, Arg b, bool otherwise) noexcept -> bool {
  return (a > b) || ((a == b) && otherwise);
}

/// Check if the first argument is larger than the second one.
/// If the arguments are equal, return random boolean value.
template<std::totally_ordered Arg, class RNG>
  requires std::uniform_random_bit_generator<std::remove_reference_t<RNG>>
constexpr auto greater_or(Arg a, Arg b, RNG&& rng) noexcept -> bool {
  TIT_ASSUME_UNIVERSAL(RNG, rng);
  return greater_or(a, b, rng() % 2 == 1);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Get a randomized hash for the given integral arguments.
/// Randomized hash does not depend on the order of the arguments.
template<std::uniform_random_bit_generator RNG = SplitMix64,
         std::integral... Vals>
constexpr auto randomized_hash(Vals... vals) noexcept -> RNG::result_type {
  return (RNG{static_cast<RNG::result_type>(vals)}() ^ ...);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
