/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <cerrno>
#include <cstring>
#include <format>
#include <source_location>
#include <string>
#include <utility>

#include "tit/core/checks.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/stacktrace.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Exception::Exception(std::string message,
                     std::source_location location,
                     Stacktrace stacktrace)
    : message_{std::move(message)}, location_{location},
      stacktrace_{std::move(stacktrace)} {}

auto Exception::what() const noexcept -> const char* {
  return message_.c_str();
}

auto Exception::where() const noexcept -> const std::source_location& {
  return location_;
}

auto Exception::when() const noexcept -> const Stacktrace& {
  return stacktrace_;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace {

auto errno_message(int errno_value) -> std::string {
  TIT_ASSERT(errno_value != 0, "No error information available!");
  return std::strerror(errno_value);
}

} // namespace

ErrnoException::ErrnoException(std::string message,
                               std::source_location location,
                               Stacktrace stacktrace)
    : ErrnoException{errno,
                     std::move(message),
                     location,
                     std::move(stacktrace)} {}

ErrnoException::ErrnoException(int errno_value,
                               std::string message,
                               std::source_location location,
                               Stacktrace stacktrace)
    : Exception{std::format("{} {}.", message, errno_message(errno_value)),
                location,
                std::move(stacktrace)},
      errno_value_{errno_value} {}

auto ErrnoException::errno_value() const -> int {
  return errno_value_;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
