/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include "tit/python/python_h.hpp" // must be first.

#include <concepts>

#include "tit/core/exception.hpp"
#include "tit/core/str_utils.hpp"

namespace tit::py {

// NOLINTBEGIN(*-include-cleaner)

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Throw an exception caused by the Python error that is already set.
[[noreturn]] inline void raise() {
  PyErr_Print();
  TIT_THROW("Python exception.");
}

/// Throw a type error.
[[noreturn]] inline void raise_type_error(CStrView /*message*/) {
  raise();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Ensure that the status code is positive.
template<std::integral Int>
inline auto ensure(Int status) -> Int {
  if (status < 0) raise();
  return status;
}

template<class Result, std::integral Int>
  requires (!std::same_as<Result, Int>)
inline auto ensure(Int status) -> Result {
  return static_cast<Result>(ensure(status));
}

template<std::integral Int>
inline auto ensure_bool(Int status) -> bool {
  return ensure(status) == 1;
}

/// Ensure that the object is not null.
[[nodiscard]] inline auto ensure(PyObject* object) -> PyObject* {
  if (object == nullptr) raise();
  return object;
}
inline void ensure_decref(PyObject* object) {
  Py_DECREF(ensure(object));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// NOLINTEND(*-include-cleaner)

} // namespace tit::py
