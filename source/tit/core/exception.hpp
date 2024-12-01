/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <exception>
#include <format> // IWYU pragma: keep
#include <source_location>
#include <string>
#include <utility>

#include "tit/core/missing.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Exception with stack tracing support.
class Exception : public std::exception {
public:

  /// Initialize exception. Source location and stack trace are recorded.
  [[gnu::always_inline]]
  explicit Exception(
      std::string message,
      std::source_location location = std::source_location::current(),
      Std::stacktrace stacktrace = Std::stacktrace::current())
      : message_{std::move(message)}, location_{location},
        stacktrace_{std::move(stacktrace)} {}

  /// Get the exception message.
  auto what() const noexcept -> const char* override {
    return message_.c_str();
  }

  /// Retrieve the exception location.
  auto where() const noexcept -> const std::source_location& {
    return location_;
  }

  /// Retrieve the exception stack trace.
  auto when() const noexcept -> const Std::stacktrace& {
    return stacktrace_;
  }

private:

  std::string message_;
  std::source_location location_;
  Std::stacktrace stacktrace_;

}; // class Exception

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Terminate handler that catches exceptions and exits the process.
class TerminateHandler {
public:

  /// Initialize the terminate handler.
  TerminateHandler() noexcept;

  /// Terminate handler is not move-constructible.
  TerminateHandler(TerminateHandler&&) = delete;

  /// Terminate handler is not movable.
  auto operator=(TerminateHandler&&) -> TerminateHandler& = delete;

  /// Terminate handler is not copy-constructible.
  TerminateHandler(const TerminateHandler&) = delete;

  /// Terminate handler is not copyable.
  auto operator=(const TerminateHandler&) -> TerminateHandler& = delete;

  /// Reset terminate handling.
  ~TerminateHandler() noexcept;

private:

  [[noreturn]]
  static void handle_terminate_();

  std::terminate_handler prev_handler_;

}; // class TerminateHandler

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Throw an exception.
#define TIT_THROW(message, ...)                                                \
  throw tit::Exception(std::format((message) __VA_OPT__(, __VA_ARGS__)))

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
