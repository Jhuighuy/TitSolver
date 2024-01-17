/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <cstdio>
#include <format>
#include <utility>

#include <stdio.h> // NOLINT(*-deprecated-headers)

#define BOOST_STACKTRACE_GNU_SOURCE_NOT_REQUIRED
#include <boost/stacktrace/stacktrace.hpp>

namespace tit::Std {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

// NOLINTBEGIN(cert-err33-c)

/** Temporary basic implementation of `std::print`. */
/** @{ */
template<class... Args>
void print(std::format_string<Args...> fmt, Args&&... args) {
  std::fputs(std::format(fmt, std::forward<Args>(args)...).c_str(), stdout);
}
template<class... Args>
void print(std::FILE* stream, std::format_string<Args...> fmt, Args&&... args) {
  std::fputs(std::format(fmt, std::forward<Args>(args)...).c_str(), stream);
}
/** @} */

/** Temporary basic implementation of `std::println`. */
/** @{ */
template<class... Args>
void println(std::format_string<Args...> fmt, Args&&... args) {
  std::puts(std::format(fmt, std::forward<Args>(args)...).c_str());
}
template<class... Args>
void println(std::FILE* stream, std::format_string<Args...> fmt,
             Args&&... args) {
  std::fputs(std::format(fmt, std::forward<Args>(args)...).c_str(), stream);
  std::fputc('\n', stream);
}
/** @} */

// NOLINTEND(cert-err33-c)

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Temporary basic implementation of `std::stacktrace`. */
class stacktrace : public boost::stacktrace::stacktrace {
public:

  /** Get the current stack trace. */
  [[gnu::always_inline]] static auto current() -> stacktrace {
    return {};
  }

}; // struct stacktrace

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit::Std
