/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

// clang-format off

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Disable warnings for nanobind.
/// Invoke this macro before including any nanobind header.
#define TIT_NANOBIND_INCLUDE_BEGIN                                             \
  _Pragma("GCC diagnostic push")                                               \
  _Pragma("GCC diagnostic ignored \"-Wcast-qual\"")                            \
  _Pragma("GCC diagnostic ignored \"-Wold-style-cast\"")                       \
  _Pragma("GCC diagnostic ignored \"-Wpedantic\"")                             \
  _Pragma("GCC diagnostic ignored \"-Wshadow\"")                               \
  _Pragma("GCC diagnostic ignored \"-Wc++98-compat-extra-semi\"")

/// Restore the warnings to default.
#define TIT_NANOBIND_INCLUDE_END _Pragma("GCC diagnostic pop")

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// clang-format on
