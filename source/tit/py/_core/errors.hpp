/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// IWYU pragma: private, include "tit/py/core.hpp"
#pragma once

#include <exception>
#include <format>
#include <utility>

#include "tit/core/checks.hpp"
#include "tit/core/str_utils.hpp"
#include "tit/core/utils.hpp"

#include "tit/py/_core/_python.hpp"
#include "tit/py/_core/objects.hpp"

namespace tit::py {

// NOLINTBEGIN(*-include-cleaner)

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Declared in `_python.hpp` to avoid circular dependencies.
inline auto is_error_set() -> bool {
  return PyErr_Occurred() != nullptr;
}

/// Clear the Python error.
inline void clear_error() {
  TIT_ASSERT(is_error_set(), "No Python error was set!");
  PyErr_Clear();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Save and restore the Python error.
class ErrorScope final : public NonCopyableBase {
public:

  /// Construct the error scope.
  ErrorScope() noexcept {
    TIT_ASSERT(is_error_set(), "No Python error was set!");
    PyErr_Fetch(&type_, &value_, &traceback_);
  }

  /// Destruct the error scope.
  ~ErrorScope() noexcept {
    if (holds_error()) restore();
  }

  /// Move-construct the error scope.
  ErrorScope(ErrorScope&& other) noexcept
      : type_{std::exchange(other.type_, nullptr)},
        value_{std::exchange(other.value_, nullptr)},
        traceback_{std::exchange(other.traceback_, nullptr)} {}

  /// Move-assign the error scope.
  auto operator=(ErrorScope&& other) noexcept -> ErrorScope& {
    if (this != &other) {
      type_ = std::exchange(other.type_, nullptr);
      value_ = std::exchange(other.value_, nullptr);
      traceback_ = std::exchange(other.traceback_, nullptr);
    }
    return *this;
  }

  /// Get the error type.
  auto type() const noexcept -> PyObject* {
    return type_;
  }

  /// Get the error value.
  auto value() noexcept -> PyObject*& {
    return value_;
  }
  auto value() const noexcept -> PyObject* {
    return value_;
  }

  /// Get the error traceback.
  auto traceback() const noexcept -> PyObject* {
    return traceback_;
  }

  /// Check if the error scope holds an error.
  auto holds_error() const noexcept -> bool {
    return type_ != nullptr;
  }

  /// Restore the Python error.
  void restore() noexcept {
    TIT_ASSERT(holds_error(), "Python error is null!");
    PyErr_Restore(type_, value_, traceback_);
    type_ = value_ = traceback_ = nullptr;
  }

  /// Normalize the error.
  void normalize() noexcept {
    TIT_ASSERT(holds_error(), "Python error is null!");
    PyErr_NormalizeException(&type_, &value_, &traceback_);
  }

private:

  PyObject* type_ = nullptr;
  PyObject* value_ = nullptr;
  PyObject* traceback_ = nullptr;

}; // class ErrorScope

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// C++ exception caused by a Python error.
class Error final : public std::exception {
public:

  /// Construct the error. Scope is stored inside of the exception object
  /// and restored upon destruction.
  explicit Error(ErrorScope scope) : scope_{std::move(scope)} {
    TIT_ASSERT(scope_.holds_error(), "Scope does not hold an error!");
    scope_.normalize();
  }

  /// Prepend the text to the error message.
  template<class... Args>
  void prefix_message(std::format_string<Args...> fmt, Args&&... args) {
    auto* new_message = PyUnicode_FromFormat(
        "%s: %S",
        std::format(fmt, std::forward<Args>(args)...).c_str(),
        scope_.value());
    Py_CLEAR(scope_.value());
    scope_.value() =
        PyObject_CallFunctionObjArgs(scope_.type(), new_message, nullptr);
    Py_CLEAR(new_message);
  }

  /// Get the error message.
  // NOLINTNEXTLINE(*-exception-escape)
  auto what() const noexcept -> const char* override {
    if (message_.empty()) {
      TIT_ASSERT(!is_error_set(), "Different Python error was set!");

      auto* type_name = PyObject_GetAttrString(scope_.type(), "__name__");
      auto* value_str = PyObject_Str(scope_.value());
      message_ = std::format( //
          "{}: {}",
          PyUnicode_AsUTF8AndSize(type_name, /*size=*/nullptr),
          PyUnicode_AsUTF8AndSize(value_str, /*size=*/nullptr));
      Py_CLEAR(type_name);
      Py_CLEAR(value_str);

      if (auto* const traceback = scope_.traceback(); traceback != nullptr) {
        auto* traceback_str{PyObject_Str(traceback)};
        message_ = std::format( //
            "{}\n{}",
            message_,
            PyUnicode_AsUTF8AndSize(traceback_str, /*size=*/nullptr));
        Py_CLEAR(traceback_str);
      }
    }
    return message_.c_str();
  }

private:

  ErrorScope scope_;
  mutable std::string message_;

}; // class Error

// Declared in `_python.hpp` to avoid circular dependencies.
[[noreturn]] inline void raise() {
  TIT_ASSERT(is_error_set(), "Python error is not set!");
  throw Error{ErrorScope{}};
}

/// Raise an assertion error and throw a corresponding C++ exception.
[[noreturn]] inline void raise_assertion_error(CStrView message) {
  TIT_ASSERT(!is_error_set(), "Python error is already set!");
  PyErr_SetString(PyExc_AssertionError, message.c_str());
  raise();
}

/// Raise a type error and throw a corresponding C++ exception.
[[noreturn]] inline void raise_type_error(CStrView message, auto... args) {
  TIT_ASSERT(!is_error_set(), "Python error is already set!");
  PyErr_Format(PyExc_TypeError, message.c_str(), args...);
  raise();
}
[[noreturn]] inline void raise_type_error(PyObject* obj, CStrView expected) {
  TIT_ASSERT(!is_error_set(), "Python error is already set!");
  auto* type = PyObject_Type(obj);
  auto* type_name = PyObject_GetAttrString(type, "__name__");
  PyErr_Format(PyExc_TypeError, // NOLINT(*-vararg)
               "'%s' is not a %s",
               PyUnicode_AsUTF8AndSize(type_name, /*size=*/nullptr),
               expected.c_str());
  Py_CLEAR(type);
  Py_CLEAR(type_name);
  raise();
}

/// Raise a value error and throw a corresponding C++ exception.
[[noreturn]] inline void raise_value_error(CStrView message, auto... args) {
  TIT_ASSERT(!is_error_set(), "Python error is already set!");
  PyErr_Format(PyExc_ValueError, message.c_str(), args...);
  raise();
}

/// Raise a runtime error and throw a corresponding C++ exception.
[[noreturn]] inline void raise_runtime_error(CStrView message) {
  TIT_ASSERT(!is_error_set(), "Python error is already set!");
  PyErr_SetString(PyExc_RuntimeError, message.c_str());
  raise();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Declared in `objects.hpp` to avoid circular dependencies.
template<std::derived_from<Object> Derived>
auto expect(const Object& arg) -> Derived {
  if (Derived::isinstance(arg)) return static_cast<const Derived&>(arg);
  /// @todo Improve the error message.
  raise_type_error(static_cast<const Derived&>(arg).get(), "<todo>");
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// NOLINTEND(*-include-cleaner)

} // namespace tit::py
