/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <cstdlib>
#include <utility>

#include "tit/core/checks.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/sys/utils.hpp"

#ifdef TIT_HAVE_GCOV
extern "C" void __gcov_dump(); // NOLINT(*-reserved-identifier)
#endif

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void checked_atexit(atexit_callback_t callback) {
  TIT_ASSERT(callback != nullptr, "At-exit callback is invalid!");
  const auto status = std::atexit(callback);
  TIT_ENSURE(status == 0, "Unable to register at-exit callback!");
}

[[noreturn]] void exit(ExitCode exit_code) noexcept {
  std::exit(std::to_underlying(exit_code)); // NOLINT(concurrency-mt-unsafe)
}

[[noreturn]] void fast_exit(ExitCode exit_code) noexcept {
#ifdef TIT_HAVE_GCOV
  __gcov_dump();
#endif
  std::_Exit(std::to_underlying(exit_code));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
