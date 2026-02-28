/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <cstdlib>
#include <optional>
#include <string>

#include "tit/core/env.hpp"
#include "tit/core/exception.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto get_env(const std::string& name) noexcept -> std::optional<std::string> {
  const auto* const value = std::getenv(name.c_str());
  if (value == nullptr) return std::nullopt;
  return value;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void set_env(const std::string& name, const std::string& val) {
  // NOLINTNEXTLINE(*-include-cleaner)
  const auto status = setenv(name.c_str(), val.c_str(), /*overwrite=*/1);
  TIT_ENSURE_ERRNO(status == 0,
                   "Unable to set environment variable '{}' value to '{}'.",
                   name,
                   val);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void unset_env(const std::string& name) {
  // NOLINTNEXTLINE(*-include-cleaner)
  const auto status = unsetenv(name.c_str());
  TIT_ENSURE_ERRNO(status == 0,
                   "Unable to unset environment variable '{}'.",
                   name);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
