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
///
/// In the scope of this `tit::prop`, null JSON values are  equivalent to
/// `std::nullopt`.
inline const JSON null;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Extract a value from a JSON, if it is not null.
template<class Val>
constexpr auto as(const JSON& json, std::optional<Val> default_ = std::nullopt)
    -> std::optional<Val> {
  return json.is_null() ? default_ : json.get<Val>();
}

/// Construct a JSON from an optional value.
template<class Val>
constexpr auto from(std::optional<Val> val) -> JSON {
  return val.has_value() ? JSON{val.value()} : null;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Get a value from a JSON object, or return a default value.
/// @{
constexpr auto get(const JSON& json,
                   std::string_view key,
                   const JSON& default_ = null) -> JSON {
  return json.contains(key) ? json.at(key) : default_;
}
template<class Val>
constexpr auto get(const JSON& json, //
                   std::string_view key,
                   const Val& default_) -> Val {
  return as(get<Val>(json, key), default_);
}
/// @}

/// Set a value in a JSON object, if the value is not null.
/// @{
constexpr void set(JSON& json, std::string_view key, JSON val) {
  if (!val.is_null()) json[key] = std::move(val);
}
template<class Val>
constexpr void set(JSON& json, std::string_view key, std::optional<Val> val) {
  set(json, key, val.has_value() ? JSON{val.value()} : null);
}
/// @}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Get a value from a JSON object and remove it from the object.
/// @{
constexpr auto pop(JSON& json, std::string_view key) -> JSON {
  auto val = json.at(key);
  json.erase(key);
  return val;
}
template<class Val>
constexpr auto pop(JSON& json, std::string_view key) -> Val {
  return pop(json, key).get<Val>();
}
/// @}

/// Get a value from a JSON object and remove it from the object, otherwise
/// return a default value.
/// @{
constexpr auto pop(JSON& json, std::string_view key, const JSON& default_)
    -> JSON {
  if (json.contains(key)) return pop(json, key);
  return default_;
}
template<class Val>
constexpr auto pop(JSON& json,
                   std::string_view key,
                   const std::optional<Val>& default_) -> std::optional<Val> {
  if (json.contains(key)) return pop<Val>(json, key);
  return default_;
}
/// @}

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
