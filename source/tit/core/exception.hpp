/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <exception>
#include <functional>
#include <source_location>
#include <string>

#include "tit/core/stacktrace.hpp"
#include "tit/core/utils.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Terminate the application if an exception was thrown.
template<std::invocable Func>
constexpr auto terminate_on_exception(Func func) noexcept {
  try {
    return std::invoke(func);
  } catch (...) {
    std::terminate();
  }
}

/// Throw an exception of the given type.
#define TIT_BASE_THROW(ExceptionType, message, ...)                            \
  throw ExceptionType(std::format((message) __VA_OPT__(, __VA_ARGS__)))

/// Ensure that the condition is true, otherwise throw an exception of the
/// given type.
#define TIT_BASE_ENSURE(ExceptionType, condition, message, ...)                \
  do {                                                                         \
    if (const bool TIT_NAME(result) = (condition); !TIT_NAME(result))          \
      TIT_BASE_THROW(ExceptionType, (message) __VA_OPT__(, __VA_ARGS__));      \
  } while (false)

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Exception with stack tracing support.
class Exception : public std::exception {
public:

  /// Initialize exception. Source location and stack trace are recorded.
  explicit Exception(
      std::string message,
      std::source_location location = std::source_location::current(),
      Stacktrace stacktrace = Stacktrace::current());

  /// Get the exception message.
  auto what() const noexcept -> const char* override;

  /// Retrieve the exception location.
  auto where() const noexcept -> const std::source_location&;

  /// Retrieve the exception stack trace.
  auto when() const noexcept -> const Stacktrace&;

private:

  std::string message_;
  std::source_location location_;
  Stacktrace stacktrace_;

}; // class Exception

/// Throw an exception.
#define TIT_THROW(message, ...)                                                \
  TIT_BASE_THROW(tit::Exception, (message) __VA_OPT__(, __VA_ARGS__))

/// Ensure that the condition is true, otherwise throw an exception.
#define TIT_ENSURE(condition, message, ...)                                    \
  TIT_BASE_ENSURE(tit::Exception,                                              \
                  (condition),                                                 \
                  (message) __VA_OPT__(, __VA_ARGS__))

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Exception with `errno` information.
class ErrnoException final : public Exception {
public:

  /// Construct an exception with the current `errno` value.
  explicit ErrnoException(
      std::string message,
      std::source_location location = std::source_location::current(),
      Stacktrace stacktrace = Stacktrace::current());

  /// Construct an exception with given `errno` value.
  ErrnoException(
      int errno_value,
      std::string message,
      std::source_location location = std::source_location::current(),
      Stacktrace stacktrace = Stacktrace::current());

  /// Get the `errno` value.
  auto errno_value() const -> int;

private:

  int errno_value_;

}; // class ErrnoException

/// Throw an exception with `errno` information.
#define TIT_THROW_ERRNO(message, ...)                                          \
  TIT_BASE_THROW(tit::ErrnoException, (message) __VA_OPT__(, __VA_ARGS__))

/// Ensure that the condition is true, otherwise throw an exception with
/// `errno` information.
#define TIT_ENSURE_ERRNO(condition, message, ...)                              \
  TIT_BASE_ENSURE(tit::ErrnoException,                                         \
                  (condition),                                                 \
                  (message) __VA_OPT__(, __VA_ARGS__))

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
