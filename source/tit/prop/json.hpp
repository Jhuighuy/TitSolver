/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <optional>
#include <string_view>

#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

#include "tit/core/exception.hpp"

namespace tit::prop::json {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// JSON value.
using JSON = nlohmann::json;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Null JSON value.
inline const JSON null;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Get a value from a JSON object, or return a default value.
/// @{
constexpr auto get(const JSON& json,
                   std::string_view key,
                   const JSON& def = null) -> JSON {
  return json.contains(key) ? json.at(key) : def;
}
template<class Val>
constexpr auto get(const JSON& json, std::string_view key, const Val& def)
    -> Val {
  return json.contains(key) ? json.at(key).get<Val>() : def;
}
/// @}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Get a value from a JSON object and remove it from the object.
/// @{
constexpr auto pop(JSON& json, std::string_view key) -> JSON {
  TIT_ENSURE(json.contains(key), "Missing '{}' key.", key);
  auto val = json.at(key);
  json.erase(key);
  return val;
}
template<class Val>
constexpr auto pop(JSON& json, std::string_view key) -> Val {
  return pop(json, key).get<Val>();
}
/// @}

/// Get a value from a JSON object and remove it from the object, if it exists.
template<class Val>
constexpr auto maybe_pop(JSON& json, std::string_view key)
    -> std::optional<Val> {
  if (json.contains(key)) return pop<Val>(json, key);
  return std::nullopt;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Set a value in a JSON object.
template<class Val>
constexpr void maybe_set(JSON& json,
                         std::string_view key,
                         std::optional<Val> val) {
  if (val.has_value()) json[key] = std::move(*val);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Ensure that JSON value is empty.
constexpr void ensure_empty(const JSON& json, std::string_view label = "JSON") {
  TIT_ENSURE(json.empty(),
             "{} contains extra keys: '{}'.",
             label,
             json.dump(/*indent=*/2));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::prop::json
