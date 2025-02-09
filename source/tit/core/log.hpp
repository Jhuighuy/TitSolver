/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
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
