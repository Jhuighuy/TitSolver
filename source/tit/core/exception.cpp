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
#include "tit/core/system.hpp"

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

// Report unhandled exception that derives from `std::exception`.
void report_std_exception_(const std::exception& e) {
  eprint("\n\n");
  if (const auto* const ee = dynamic_cast<const Exception*>(&e);
      ee != nullptr) {
    const auto& loc = ee->where();
    eprint("{}:{}:{}: ", loc.file_name(), loc.line(), loc.column());
  }
  eprint("Terminating due to an unhandled exception.\n\n");
  const auto throw_expression =
      std::format("throw {}(...);", demangle_type_name_(typeid(e).name()));
  eprint("  {}\n", throw_expression);
  eprint("  ^{:~>{}} {}\n\n", "", throw_expression.size() - 1, e.what());
}

// Report unhandled exception that does not derive from `std::exception`.
void report_non_std_exception() {
  eprint("\n\nTerminating due to an unhandled exception.");
}

} // namespace

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TerminateHandler::TerminateHandler() noexcept
    : prev_handler_{std::set_terminate(handle_terminate_)} {}

TerminateHandler::~TerminateHandler() noexcept {
  std::set_terminate(prev_handler_);
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
      report_non_std_exception();
    }
  } else {
    // We've got here for some other reason.
    eprint("\n\nTerminating due to a call to std::terminate.\n\n");
  }
  fast_exit(1);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
