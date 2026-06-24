/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// IWYU pragma: private, include "tit/core/simd.hpp"
#pragma once

#include <concepts>
#include <cstddef>

#include <hwy/highway.h>

#include "tit/core/type.hpp"

namespace tit::simd {

namespace hn = hwy::HWY_NAMESPACE;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Is SIMD supported for the specified numeric type?
template<class Num>
concept supported_type = std::integral<Num> || std::floating_point<Num>;

/// Evaluate the following command block if SIMD is supported for the given
/// type and if the code is not executed at compile-time.
#define TIT_IF_SIMD_AVALIABLE(Num)                                             \
  if constexpr (simd::supported_type<Num>)                                     \
    if !consteval

/// Is SIMD castable from one type to another?
template<class From, class To>
concept castable_to_type =
    supported_type<To> && supported_type<From> && (sizeof(To) == sizeof(From));

/// Evaluate the following command block if SIMD cast is supported for the given
/// type and if the code is not executed at compile-time.
#define TIT_IF_SIMD_CAST_AVALIABLE(From, To)                                   \
  if constexpr (simd::castable_to_type<From, To>)                              \
    if !consteval

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Minimal byte width of the SIMD register that is available on the current
/// hardware.
inline constexpr std::size_t min_reg_byte_width_v{
#ifdef __ARM_NEON
    8
#else
    16
#endif
};

/// Minimal SIMD register size for the specified type.
template<supported_type Num>
inline constexpr std::size_t min_reg_size_v =
    min_reg_byte_width_v / sizeof(Num);

/// Maximal SIMD register size for the specified type.
template<supported_type Num>
inline constexpr std::size_t max_reg_size_v =
    hn::ScalableTag<normalize_type_t<Num>>{}.MaxLanes();

/// Is SIMD supported for the specified numeric type and size?
template<class Num, std::size_t Size>
concept supported =
    supported_type<Num> &&
    (min_reg_size_v<Num> <= Size && Size <= max_reg_size_v<Num>) &&
    (Size % min_reg_size_v<Num> == 0);

// Is SIMD castable from one type to another?
template<class From, class To, std::size_t Size>
concept castable_to =
    supported<To, Size> && supported<From, Size> && castable_to_type<From, To>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::simd
