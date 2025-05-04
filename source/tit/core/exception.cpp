/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <exception>
#include <format>

#include "tit/core/checks.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/par/control.hpp"
#include "tit/core/print.hpp"
#include "tit/core/sys/utils.hpp"
#include "tit/core/type.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TerminateHandler::TerminateHandler() noexcept
    : prev_handler_{std::set_terminate(handle_terminate_)} {
  TIT_ASSERT(prev_handler_ != nullptr, "Failed to set terminate handler!");
  TIT_ASSERT(prev_handler_ != handle_terminate_,
             "Terminate handler was already registered!");
}

TerminateHandler::~TerminateHandler() noexcept {
  // Restore the previous terminate handler.
  const auto current_handler = std::set_terminate(prev_handler_);
  TIT_ASSERT(current_handler == handle_terminate_,
             "Terminate handler was not registered previously!");
}

[[noreturn]] void TerminateHandler::handle_terminate_() {
  const par::GlobalLock lock{};
  if (const auto exception_ptr = std::current_exception(); exception_ptr) {
    // How did we get here?
    try {
      std::rethrow_exception(exception_ptr);
    } catch (const Exception& e) {
      eprintln_crash_report("Terminating due to an unhandled exception.",
                            std::format("throw {}(...);", type_name_of(e)),
                            e.what(),
                            e.where(),
                            e.when());
    } catch (const std::exception& e) {
      eprintln_crash_report("Terminating due to an unhandled exception.",
                            std::format("throw {}(...);", type_name_of(e)),
                            e.what());
    } catch (...) {
      eprintln_crash_report("Terminating due to an unhandled exception.");
    }
  } else {
    // We've got here for some other reason.
    eprintln_crash_report("Terminating due to a call to std::terminate().");
  }
  fast_exit(ExitCode::failure);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
