/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

// Some versions of doctest have broken platform detection. The easiest way to
// fix it is to include this header before doctest.
#include <cstdlib> // IWYU pragma: keep

#include <doctest/doctest.h> // IWYU pragma: exports

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#define STATIC_CHECK(...)                                                      \
  static_assert(__VA_ARGS__);                                                  \
  CHECK(__VA_ARGS__)

#define STATIC_CHECK_FALSE(...)                                                \
  static_assert(!(__VA_ARGS__));                                               \
  CHECK_FALSE(__VA_ARGS__)

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
