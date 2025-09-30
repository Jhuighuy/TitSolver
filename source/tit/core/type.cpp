/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <cstdlib>
#include <memory>
#include <string>

#include <cxxabi.h>

#include "tit/core/checks.hpp"
#include "tit/core/type.hpp"

namespace tit::impl {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto demangle(const char* mangled_name) -> std::string {
  TIT_ASSERT(mangled_name != nullptr, "Mangled name must not be null");
  int status = 0;
  const std::unique_ptr<char, void (*)(char*)> result{
      abi::__cxa_demangle(mangled_name,
                          /*buffer=*/nullptr,
                          /*length=*/nullptr,
                          &status),
      [](char* ptr) { std::free(ptr); }, // NOLINT(*-no-malloc,*-owning-memory)
  };
  return status == 0 ? result.get() : mangled_name;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::impl
