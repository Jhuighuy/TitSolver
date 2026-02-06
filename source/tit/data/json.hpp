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

namespace tit::data::json {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

using nlohmann::json;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Get a value from a JSON object and remove it from the object.
/// @{
constexpr auto pop(json& json, std::string_view key) -> nlohmann::json {
  TIT_ENSURE(json.contains(key), "Missing '{}' key.", key);
  auto val = json.at(key);
  json.erase(key);
  return val;
}
template<class Val>
constexpr auto pop(json& json, std::string_view key) -> Val {
  return pop(json, key).get<Val>();
}
/// @}

/// Get a value from a JSON object and remove it from the object, if it exists.
template<class Val>
constexpr auto maybe_pop(json& json, std::string_view key)
    -> std::optional<Val> {
  if (json.contains(key)) return pop<Val>(json, key);
  return std::nullopt;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Set a value in a JSON object.
template<class Val>
constexpr void maybe_set(json& json,
                         std::string_view key,
                         std::optional<Val> val) {
  if (val.has_value()) json[key] = std::move(*val);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::data::json
