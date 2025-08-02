/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#ifdef __APPLE__

#include <array>
#include <filesystem>

#include <libproc.h>
#include <sys/proc_info.h>
#include <unistd.h>

#include "tit/core/exception.hpp"
#include "tit/core/path.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto exe_path() -> std::filesystem::path {
  std::array<char, PROC_PIDPATHINFO_MAXSIZE + 1> buffer{};
  const auto status = proc_pidpath(getpid(), buffer.data(), buffer.size() - 1);
  TIT_ENSURE(status > 0, "Unable to query the current executable path!");
  return buffer.data();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit

#endif // ifdef __APPLE__
