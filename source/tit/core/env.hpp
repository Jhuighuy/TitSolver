/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <format>
#include <optional>

#include "tit/core/exception.hpp"
#include "tit/core/str.hpp"
#include "tit/core/type.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Get the value of an environment variable.
auto get_env(CStrView name) noexcept -> std::optional<CStrView>;

/// Get the value of an environment variable and convert it to a specific type.
/// Throw an exception if the value is present but cannot be converted.
template<class Val>
auto get_env(CStrView name) -> std::optional<Val> {
  return get_env(name).and_then([name](CStrView val) {
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
auto get_env(CStrView name, Val fallback) -> Val {
  return get_env<Val>(name).value_or(fallback);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Set the value of an environment variable.
void set_env(CStrView name, CStrView val);

/// Set the value of an environment variable from a non-string type.
template<std::formattable<char> Val>
void set_env(CStrView name, Val val) {
  set_env(name, CStrView{std::format("{}", val)});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Unset an environment variable.
void unset_env(CStrView name);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
