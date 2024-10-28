/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <cstddef> // IWYU pragma: keep

#define BOOST_STACKTRACE_GNU_SOURCE_NOT_REQUIRED
#include <boost/stacktrace/stacktrace.hpp>

// IWYU pragma: begin_exports
#ifdef __GLIBCXX__
#include "tit/core/missing.libstdc++.hpp"
#endif
#ifdef _LIBCPP_VERSION
#include "tit/core/missing.libc++.hpp"
#endif
// IWYU pragma: end_exports

namespace tit::Std {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Stack trace.
class stacktrace final : public boost::stacktrace::stacktrace {
public:

  using boost::stacktrace::stacktrace::stacktrace;

  /// Retrieve the current stack trace.
  [[gnu::always_inline]]
  static auto current() noexcept -> stacktrace {
    return {};
  }

}; // class stacktrace

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::Std
