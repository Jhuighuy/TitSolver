/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <exception>
#include <format>
#include <string>

#include "tit/core/checks.hpp"
#include "tit/core/str_utils.hpp"
#include "tit/core/utils.hpp"

#include "tit/py/object.hpp"
#include "tit/py/type.hpp"
#include "tit/py/typing.hpp"

/// @todo Error handling code is not as clean as it could be. With that said,
///       I do not want to spend too much time on it, since a lot will change
///       with Python 3.12, so it makes sense to just leave it as is for now.
namespace tit::py {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Python traceback object reference.
class Traceback : public Object {
public:

  /// Get the type object of the `Traceback`.
  static auto type() -> Type;

  /// Check if the object is a subclass of `Traceback`.
  static auto isinstance(const Object& obj) -> bool;

  /// Render the traceback as a string.
  auto render() const -> std::string;

}; // class Traceback

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Python error reference.
class BaseException : public Object {
public:

  /// Get the type object of the `BaseException`.
  static auto type() -> Type;

  /// Check if the object is a subclass of `BaseException`.
  static auto isinstance(const Object& obj) -> bool;

  /// Get the traceback object.
  auto traceback() const -> Optional<Traceback>;

  /// Render the exception as a string.
  auto render() const -> std::string;

}; // class BaseException

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Store the currently active error.
class ErrorScope : public NonCopyableBase {
public:

  /// Construct the error scope and save the current error.
  ErrorScope() noexcept;

  /// Move-construct the error scope.
  ErrorScope(ErrorScope&& other) noexcept;

  /// Move-assign the error scope.
  auto operator=(ErrorScope&& other) noexcept -> ErrorScope&;

  /// Destruct the error scope and free the error.
  ~ErrorScope() noexcept;

  /// Access the saved error.
  /// @{
  auto error() const -> BaseException;
  void set_error(BaseException value) noexcept;
  /// @}

  /// Restore the error.
  void restore() noexcept;

private:

  PyObject* type_ = nullptr;
  PyObject* value_ = nullptr;
  PyObject* traceback_ = nullptr;

}; // class ErrorScope

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// C++ exception caused by a Python error.
class ErrorException final : public std::exception, public ErrorScope {
public:

  /// Construct the exception.
  explicit ErrorException();

  /// Get the error message.
  auto what() const noexcept -> const char* override;

private:

  std::string message_;

}; // class Error

/// Throw a C++ exception caused by a Python error that is already set.
[[noreturn]] void raise();

/// Set `TypeError` and throw a C++ exception.
/// @{
[[noreturn]] void raise_type_error(CStrView message);
template<class... Args>
[[noreturn]] void raise_type_error(std::format_string<Args...> fmt,
                                   Args&&... args) {
  raise_type_error(std::format(fmt, std::forward<Args>(args)...));
}
/// @}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Convert a Python object reference to another type.
template<std::derived_from<Object> Derived>
auto expect(const Object& arg) -> Derived {
  if (Derived::isinstance(arg)) return static_cast<const Derived&>(arg);
  raise_type_error("expected '{}', got '{}'",
                   type_name<Derived>(),
                   type(arg).fully_qualified_name());
}

/// Steal the reference to the object expected to be of the given type.
template<std::derived_from<Object> Derived>
auto steal(PyObject* obj) -> Derived {
  return expect<Derived>(steal(obj));
}

/// Borrow the reference to the object expected to be of the given type.
template<std::derived_from<Object> Derived>
auto borrow(PyObject* obj) -> Derived {
  return expect<Derived>(borrow(obj));
}

/// Maybe steal the reference to the object if it is not null.
template<std::derived_from<Object> Derived = Object>
auto maybe_steal(PyObject* obj) -> Optional<Derived> {
  if (obj != nullptr) return steal<Derived>(obj);
  return None();
}

/// Maybe borrow the reference to the object if it is not null.
template<std::derived_from<Object> Derived = Object>
auto maybe_borrow(PyObject* obj) -> Optional<Derived> {
  if (obj != nullptr) return borrow<Derived>(obj);
  return None();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Check if the Python error is already set.
auto is_error_set() -> bool;

/// Clear the Python error.
void clear_error();

/// Ensure there is no error.
inline void ensure_no_error() {
  if (is_error_set()) raise();
}

/// Ensure that the status code represents a successful operation.
template<class Result = bool, std::integral Int>
auto ensure(Int status) -> Result {
  if constexpr (std::signed_integral<Int>) {
    if (status < 0) {
      TIT_ASSERT(is_error_set(),
                 "Status code represents a failure, but error is not set!");
      raise();
    }
  }
  return static_cast<Result>(status);
}

/// Ensure that the object returned by a Python function represents a success.
inline auto ensure(PyObject* ptr) -> PyObject* {
  if (ptr == nullptr) {
    TIT_ASSERT(is_error_set(), "Object is null!");
    raise();
  }
  return ptr;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::py
