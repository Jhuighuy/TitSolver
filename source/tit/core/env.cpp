/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <cstdlib>
#include <optional>

#include "tit/core/env.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/str.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto get_env(CStrView name) noexcept -> std::optional<CStrView> {
  const auto* const value = std::getenv(name.c_str()); // NOLINT(*-mt-unsafe)
  if (value == nullptr) return std::nullopt;
  return CStrView{value};
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void set_env(CStrView name, CStrView val) {
  // NOLINTNEXTLINE(*-mt-unsafe,*-include-cleaner)
  const auto status = setenv(name.c_str(), val.c_str(), /*overwrite=*/1);
  TIT_ENSURE(status == 0,
             "Unable to set environment variable '{}' value to '{}'.",
             name,
             val);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void unset_env(CStrView name) {
  // NOLINTNEXTLINE(*-mt-unsafe,*-include-cleaner)
  const auto status = unsetenv(name.c_str());
  TIT_ENSURE(status == 0, "Unable to unset environment variable '{}'.", name);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
