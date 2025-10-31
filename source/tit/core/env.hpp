/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <format>
#include <optional>
#include <string>

#include "tit/core/exception.hpp"
#include "tit/core/str.hpp"
#include "tit/core/type.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Get the value of an environment variable.
auto get_env(const std::string& name) noexcept -> std::optional<std::string>;

/// Get the value of an environment variable and convert it to a specific type.
/// Throw an exception if the value is present but cannot be converted.
template<class Val>
auto get_env(const std::string& name) -> std::optional<Val> {
  return get_env(name).and_then([name](const std::string& val) {
    const auto result = str_to<Val>(val);
    TIT_ENSURE(
        result.has_value(),
        "Unable to convert the environment variable '{}' value '{}' to '{}'.",
        name,
        val,
        type_name_of<Val>());
    return result;
  });
}

/// Get the value of an environment variable and convert it to a specific type.
/// Return a fallback value if the value is not present.
/// Throw an exception if the value is present but cannot be converted.
template<class Val>
auto get_env(const std::string& name, Val fallback) -> Val {
  return get_env<Val>(name).value_or(fallback);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Set the value of an environment variable.
void set_env(const std::string& name, const std::string& val);

/// Set the value of an environment variable from a non-string type.
template<std::formattable<char> Val>
void set_env(const std::string& name, Val val) {
  set_env(name, std::format("{}", val));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Unset an environment variable.
void unset_env(const std::string& name);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
