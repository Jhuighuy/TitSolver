/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <cstdlib>
#include <optional>
#include <string_view>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/string_utils.hpp"
#include "tit/core/sys/env.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto get_env(const char* name) noexcept -> std::optional<std::string_view> {
  TIT_ASSERT(name != nullptr, "Environment variable name must not be null!");
  const auto* const value = std::getenv(name); // NOLINT(*-mt-unsafe)
  if (value == nullptr) return std::nullopt;
  return std::string_view{value};
}

auto get_env_int(const char* name) noexcept -> std::optional<int64_t> {
  return get_env(name).and_then(str_to_int);
}
auto get_env_int(const char* name, int64_t fallback) noexcept -> int64_t {
  return get_env_int(name).value_or(fallback);
}

auto get_env_float(const char* name) noexcept -> std::optional<float64_t> {
  return get_env(name).and_then(str_to_float);
}
auto get_env_float(const char* name, float64_t fallback) noexcept -> float64_t {
  return get_env_float(name).value_or(fallback);
}

auto get_env_bool(const char* name) noexcept -> std::optional<bool> {
  return get_env(name).and_then(str_to_bool);
}
auto get_env_bool(const char* name, bool fallback) noexcept -> bool {
  return get_env_bool(name).value_or(fallback);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
