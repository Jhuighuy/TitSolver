/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <format>
#include <optional>

#include "tit/core/exception.hpp"
#include "tit/core/str.hpp"
#include "tit/core/sys.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Set the value of an environment variable.
/// @{
void set_env(CStrView name, CStrView val);
template<std::formattable<char> Val>
void set_env(CStrView name, Val val) {
  set_env(name, CStrView{std::format("{}", val)});
}
/// @}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Get the value of an environment variable.
auto get_env(CStrView name) noexcept -> std::optional<CStrView>;

/// Get the value of an environment variable of a specific type.
/// Throw an exception if the value is present but invalid.
/// @{
template<class Val>
auto get_env(CStrView name) -> std::optional<Val> {
  return get_env(name).and_then([name](CStrView val) {
    const auto result = str_to<Val>(val);
    TIT_ENSURE(result.has_value(),
               "Invalid value of environment variable '{}={}'. Must be '{}'.",
               name,
               val,
               maybe_demangle_type<Val>());
    return result;
  });
}
template<class Val>
auto get_env(CStrView name, Val fallback) -> Val {
  return get_env<Val>(name).value_or(fallback);
}
/// @}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
