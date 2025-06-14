/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <optional>
#include <string>

#include <cxxabi.h>

#include "tit/core/exception.hpp"
#include "tit/core/str.hpp"
#include "tit/core/type.hpp"
#include "tit/core/utils.hpp"

namespace tit::impl {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto try_demangle(CStrView mangled_name) -> std::optional<std::string> {
  int status = 0;
  const MallocPtr<char> result{abi::__cxa_demangle(mangled_name.c_str(),
                                                   /*buffer=*/nullptr,
                                                   /*length=*/nullptr,
                                                   &status)};
  if (status != 0) return std::nullopt;
  TIT_ENSURE(result != nullptr,
             "Demanging reported status code of success ({}), "
             "but returned null pointer.",
             status);
  return result.get();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::impl
