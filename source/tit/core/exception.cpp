/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <cstdlib>
#include <exception>
#include <format>
#include <memory>
#include <string>

#include <cxxabi.h>

#include "tit/core/checks.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/io.hpp"
#include "tit/core/sys_utils.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace {

// Demangle the type name.
auto demangle_type_name_(const char* name) -> std::string {
  TIT_ASSERT(name != nullptr, "Type name must not be null!");
  int status = -1;
  const std::unique_ptr<char, void (*)(char*)> demangled_name{
      abi::__cxa_demangle(name, /*buffer=*/nullptr, /*size=*/nullptr, &status),
      [](char* ptr) { std::free(ptr); }}; // NOLINT(*-no-malloc,*-owning-memory)
  if (status == 0) return demangled_name.get();
  return name; // Demangling failed, so just return the original name.
}

// Type name of the argument.
template<class Arg>
auto type_name_(const Arg& arg) -> std::string {
  return demangle_type_name_(typeid(arg).name());
}

// Report unhandled exception that derives from `std::exception`.
void report_std_exception_(const std::exception& e) {
  eprintln();
  eprintln();
  if (const auto* const tit_e = dynamic_cast<const Exception*>(&e);
      tit_e != nullptr) {
    /// @todo We should also report stacktrace once we have it.
    const auto& loc = tit_e->where();
    eprint("{}:{}:{}: ", loc.file_name(), loc.line(), loc.column());
  }
  eprintln("Terminating due to an unhandled exception.");
  eprintln();
  const auto throw_expression = std::format("throw {}(...);", type_name_(e));
  eprintln("  {}", throw_expression);
  eprintln("  ^{:~>{}} {}", "", throw_expression.size() - 1, e.what());
  eprintln();
}

// Report unhandled exception that does not derive from `std::exception`.
void report_non_std_exception_() {
  eprintln();
  eprintln();
  eprint("Terminating due to an unhandled exception.");
  eprintln();
}

// Report call to `std::terminate`.
void report_terminate_call_() {
  eprintln();
  eprintln();
  eprintln("Terminating due to a call to std::terminate().");
  eprintln();
}

} // namespace

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TerminateHandler::TerminateHandler() noexcept
    : prev_handler_{std::set_terminate(handle_terminate_)} {}

TerminateHandler::~TerminateHandler() noexcept {
  // Restore the previous terminate handler.
  const auto current_handler = std::set_terminate(prev_handler_);
  TIT_ENSURE(current_handler == handle_terminate_,
             "Terminate handler was not registered previously!");
}

[[noreturn]]
void TerminateHandler::handle_terminate_() {
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
  fast_exit(1);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
