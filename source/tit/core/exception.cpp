/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <cstdlib>
#include <exception>
#include <format>
#include <string>

#include "tit/core/checks.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/io.hpp"
#include "tit/core/par/control.hpp"
#include "tit/core/sys/stacktrace.hpp"
#include "tit/core/sys/utils.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace {

// Report unhandled exception that derives from `std::exception`.
[[gnu::always_inline]]
inline void report_std_exception_(const std::exception& e) {
  eprintln();
  eprintln();
  const auto* const tit_e = dynamic_cast<const Exception*>(&e);
  if (tit_e != nullptr) {
    const auto& loc = tit_e->where();
    eprint("{}:{}:{}: ", loc.file_name(), loc.line(), loc.column());
  }
  eprintln("Terminating due to an unhandled exception.");
  eprintln();
  const auto throw_expression =
      std::format("throw {}(...);", maybe_demangle_arg_type(e));
  eprintln("  {}", throw_expression);
  eprintln("  ^{:~>{}} {}", "", throw_expression.size() - 1, e.what());
  eprintln();
  eprintln("{}", tit_e != nullptr ? tit_e->when() : Stacktrace::current());
}

// Report unhandled exception that does not derive from `std::exception`.
[[gnu::always_inline]] inline void report_non_std_exception_() {
  eprintln();
  eprintln();
  eprintln("Terminating due to an unhandled exception.");
  eprintln();
  eprintln("{}", Stacktrace::current());
}

// Report call to `std::terminate`.
[[gnu::always_inline]] inline void report_terminate_call_() {
  eprintln();
  eprintln();
  eprintln("Terminating due to a call to std::terminate().");
  eprintln();
  eprintln("{}", Stacktrace::current());
}

} // namespace

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TerminateHandler::TerminateHandler() noexcept
    : prev_handler_{std::set_terminate(handle_terminate_)} {}

TerminateHandler::~TerminateHandler() noexcept {
  // Restore the previous terminate handler.
  const auto current_handler = std::set_terminate(prev_handler_);
  TIT_ASSERT(current_handler == handle_terminate_,
             "Terminate handler was not registered previously!");
}

[[noreturn]] void TerminateHandler::handle_terminate_() {
  const par::GlobalLock lock{};

  // Report the incident.
  if (const auto exception_ptr = std::current_exception(); exception_ptr) {
    // We've got an unhandled exception.
    try {
      std::rethrow_exception(exception_ptr);
    } catch (const std::exception& e) {
      report_std_exception_(e);
    } catch (...) {
      report_non_std_exception_();
    }
  } else {
    // We've got here for some other reason.
    report_terminate_call_();
  }

  // Fast-exit with failure.
  fast_exit(ExitCode::failure);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
