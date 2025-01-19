/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// IWYU pragma: always_keep
// IWYU pragma: private, include "tit/py/core.hpp"
#pragma once

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#define PY_SSIZE_T_CLEAN          // Recommended Python configuration.
#ifndef TIT_PYTHON_INTERPRETER    // When not embedding, use limited API.
#define Py_LIMITED_API 0x030B0000 // Python 3.11.
#endif
#include <Python.h> // IWYU pragma: export

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include <concepts>

namespace tit::py {

// The following functions are defined in `errors.hpp`.

/// Check if the Python error is already set.
auto is_error_set() -> bool;

/// Throw a C++ exception caused by a Python error that is already set.
[[noreturn]] void raise();

/// Ensure that the status code represents a successful operation.
template<class Result = bool, std::integral Int>
auto ensure(Int status) -> Result {
  if constexpr (std::signed_integral<Int>) {
    if (status < 0) raise();
  }
  return static_cast<Result>(status);
}

} // namespace tit::py

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
