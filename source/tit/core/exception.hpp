/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <exception>
#include <source_location>
#include <string>
#include <string_view>
#include <utility>

#include "tit/core/stacktrace.hpp"

namespace tit {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** Exception with stack tracing support.
\******************************************************************************/
class Exception : public std::exception {
public:

  /** Initialize exception. Source location and stack trace are recorded. */
  [[gnu::always_inline]] explicit Exception(
      std::string_view message,
      std::source_location location = std::source_location::current(),
      Std::stacktrace stacktrace = Std::stacktrace::current())
      : message_{message}, location_{location},
        stacktrace_{std::move(stacktrace)} {}

  /** Exception is move-constructible. */
  Exception(Exception&&) = default;
  /** Exception is movable. */
  auto operator=(Exception&&) -> Exception& = default;

  /** Exception is copy-constructible. */
  Exception(const Exception&) = default;
  /** Exception is copyable. */
  auto operator=(const Exception&) -> Exception& = default;

  /** Destroy the exception. */
  ~Exception() override = default;

  /** Retrieve the exception message. */
  auto what() const noexcept -> const char* override {
    return message_.c_str();
  }

  /** Retrieve the exception location. */
  auto where() const noexcept -> const std::source_location& {
    return location_;
  }

  /** Retrieve the exception stack trace. */
  auto when() const noexcept -> const Std::stacktrace& {
    return stacktrace_;
  }

private:

  std::string message_;
  std::source_location location_;
  Std::stacktrace stacktrace_;

}; // class Exception

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit
