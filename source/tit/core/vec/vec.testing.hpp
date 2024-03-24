/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <array>
#include <tuple>

#include "tit/core/basic_types.hpp"
#include "tit/core/math.hpp"
#include "tit/core/vec/vec.hpp"
#include "tit/core/vec/vec_mask.hpp"

#include "tit/testing/random.hpp"
#include "tit/testing/strict.hpp"

namespace tit {

// clang-format off

// Vector types that use SIMD.
#define VEC_TYPES                                                              \
  Vec< float, 4>, Vec< float, 8>, Vec< float, 16>,                             \
  Vec<double, 2>, Vec<double, 4>, Vec<double,  8>

// All sensible vector types.
#define ALL_VEC_TYPES                                                          \
  Vec< float,  1>, Vec< float,  2>, Vec< float,  3>, Vec< float,  4>,          \
  Vec< float,  5>, Vec< float,  6>, Vec< float,  7>, Vec< float,  8>,          \
  Vec< float,  9>, Vec< float, 10>, Vec< float, 11>, Vec< float, 12>,          \
  Vec< float, 13>, Vec< float, 14>, Vec< float, 15>, Vec< float, 16>,          \
  Vec<double,  1>, Vec<double,  2>, Vec<double,  3>, Vec<double,  4>,          \
  Vec<double,  5>, Vec<double,  6>, Vec<double,  7>, Vec<double,  8>

// clang-format on

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Construct a vector from the given array.
template<class Num, size_t Dim>
constexpr auto to_vec(std::array<Num, Dim> const& array) noexcept
    -> Vec<Num, Dim> {
  return std::apply([](auto const&... qi) { return Vec{qi...}; }, array);
}

// Construct a vector mask from the given array.
template<class Num, size_t Dim>
constexpr auto to_vec_mask(std::array<bool, Dim> const& array) noexcept
    -> VecMask<Num, Dim> {
  return std::apply([](auto const&... qi) { return VecMask<Num, Dim>{qi...}; },
                    array);
}

// Random vector generator.
template<class Num, size_t Dim>
struct RandomGenerator<Vec<Num, Dim>> {
  static auto operator()() -> Vec<Num, Dim> {
    return to_vec(gen_array<Num, Dim>());
  }
}; // struct RandomGenerator<Vec<Num, Dim>>

// Random vector mask generator.
template<class Num, size_t Dim>
struct RandomGenerator<VecMask<Num, Dim>> {
  static auto operator()() -> VecMask<Num, Dim> {
    return to_vec_mask<Num>(gen_array<bool, Dim>());
  }
}; // struct RandomGenerator<VecMask<Num, Dim>>

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Use strict wrapper to prevent SIMD specialization.
template<class Num>
using Ref = Strict<Num>;

// Use strict wrapper to prevent SIMD specialization.
template<class Num, size_t Dim>
using RefVec = Vec<Ref<Num>, Dim>;

// Use strict wrapper to prevent SIMD specialization.
template<class Num, size_t Dim>
using RefVecMask = VecMask<Ref<Num>, Dim>;

// Cast a value to a reference value.
template<class Num>
auto to_ref_val(Num const& c) -> Ref<Num> {
  return Strict{c};
}

// Cast a vector to a reference vector.
template<class Num, size_t Dim>
auto to_ref_vec(Vec<Num, Dim> const& v) -> RefVec<Num, Dim> {
  return static_vec_cast<Ref<Num>>(v);
}

// Check if a value is equal to a reference value.
template<class Num>
auto equal_to_ref(Num const& c, Ref<Num> const& c_ref) -> bool {
  return to_ref_val(c) == c_ref;
}

// Check if a vector is equal to a reference vector.
template<class Num, size_t Dim>
auto equal_to_ref(Vec<Num, Dim> const& v, //
                  RefVec<Num, Dim> const& v_ref) -> bool {
  return all(to_ref_vec(v) == v_ref);
}

// Check if a vector mask is equal to a reference vector.
template<class Num, size_t Dim>
auto equal_to_ref(VecMask<Num, Dim> const& v, //
                  RefVecMask<Num, Dim> const& v_ref) -> bool {
  // Since there are no comparisons defined, we just check the values by hand.
  for (size_t i = 0; i < Dim; ++i) {
    if (v[i] != v_ref[i]) return false;
  }
  return true;
}

// Check if a value is approximately equal to a reference value.
template<class Num>
auto approx_equal_to_ref(Num const& c, Ref<Num> const& c_ref) -> bool {
  return approx_equal_to(to_ref_val(c), c_ref);
}

// Check if a vector is approximately equal to a reference vector.
template<class Num, size_t Dim>
auto approx_equal_to_ref(Vec<Num, Dim> const& v, //
                         RefVec<Num, Dim> const& v_ref) -> bool {
  return all(approx_equal_to(to_ref_vec(v), v_ref));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
