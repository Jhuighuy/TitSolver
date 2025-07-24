/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <exception>
#include <format> // IWYU pragma: keep
#include <source_location>
#include <stacktrace>
#include <string>
#include <utility>

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

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Exception with stack tracing support.
class Exception : public std::exception {
public:

  /// Initialize exception. Source location and stack trace are recorded.
  [[gnu::always_inline]] explicit Exception(
      std::string message,
      std::source_location location = std::source_location::current(),
      std::stacktrace stacktrace = std::stacktrace::current())
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
  auto when() const noexcept -> const std::stacktrace& {
    return stacktrace_;
  }

private:

  std::string message_;
  std::source_location location_;
  std::stacktrace stacktrace_;

}; // class Exception

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Throw an exception.
#define TIT_THROW(message, ...)                                                \
  throw tit::Exception(std::format((message) __VA_OPT__(, __VA_ARGS__)))

/// Ensure that the condition is true, otherwise throw an exception.
#define TIT_ENSURE(condition, message, ...)                                    \
  do {                                                                         \
    if (const bool TIT_NAME(result) = (condition); !TIT_NAME(result))          \
      TIT_THROW((message) __VA_OPT__(, __VA_ARGS__));                          \
  } while (false)

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
