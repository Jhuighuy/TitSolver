/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include "tit/core/io.hpp" // IWYU pragma: keep

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Print information message.
#define TIT_INFO(message, ...)                                                 \
  tit::println("INFO: " message __VA_OPT__(, __VA_ARGS__))

/// Print warning message.
#define TIT_WARN(message, ...)                                                 \
  tit::eprintln("WARN: " message __VA_OPT__(, __VA_ARGS__))

/// Print error message.
#define TIT_ERROR(message, ...)                                                \
  tit::eprintln("ERROR: " message __VA_OPT__(, __VA_ARGS__))

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
