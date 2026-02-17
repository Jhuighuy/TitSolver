/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <generator>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

#include "tit/core/basic_types.hpp"

namespace tit::prop {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// JSON value.
///
/// This class is a thin wrapper around `nlohmann::json`, mostly to enforce
/// consistent usage patterns and provide consistent error messages.
class JSON final {
public:

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Parse a JSON value from a string.
  static auto parse(std::string_view string) -> JSON;

  /// Dump the JSON value to a compact string.
  auto dump() const -> std::string;

  /// Dump the JSON value to a pretty-printed string.
  auto dump_pretty() const -> std::string;

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Construct a null JSON value.
  static auto null() -> const JSON&;

  /// Construct a JSON value from a boolean.
  /// @{
  static auto from_bool(bool value) -> JSON;
  static auto from_bool_opt(std::optional<bool> value) -> JSON;
  /// @}

  /// Construct a JSON value from an integer.
  /// @{
  static auto from_int(int64_t value) -> JSON;
  static auto from_int_opt(std::optional<int64_t> value) -> JSON;
  /// @}

  /// Construct a JSON value from a floating-point number.
  /// @{
  static auto from_float(float64_t value) -> JSON;
  static auto from_float_opt(std::optional<float64_t> value) -> JSON;
  /// @}

  /// Construct a JSON value from a string.
  /// @{
  static auto from_string(std::string_view value) -> JSON;
  static auto from_string_opt(std::optional<std::string_view> value) -> JSON;
  /// @}

  /// Construct an empty JSON array.
  static auto make_array() -> JSON;

  /// Construct an empty JSON object.
  static auto make_object() -> JSON;

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Check if the JSON represents a null value.
  auto is_null() const -> bool;

  /// Check if the JSON represents a boolean value.
  auto is_bool() const -> bool;

  /// Check if the JSON represents an integer value.
  auto is_int() const -> bool;

  /// Check if the JSON represents a floating-point value.
  auto is_float() const -> bool;

  /// Check if the JSON represents a string value.
  auto is_string() const -> bool;

  /// Check if the JSON represents an array value.
  auto is_array() const -> bool;

  /// Check if the JSON represents an object value.
  auto is_object() const -> bool;

  /// Check if the JSON represents an empty array or object.
  auto is_empty() const -> bool;

  /// Check if the JSON object contains a key.
  auto has_key(std::string_view key) const -> bool;

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Ensure that the JSON represents a boolean value.
  void ensure_bool() const;

  /// Ensure that the JSON represents an integer value.
  void ensure_int() const;

  /// Ensure that the JSON represents a floating-point value.
  void ensure_float() const;

  /// Ensure that the JSON represents a string value.
  void ensure_string() const;

  /// Ensure that the JSON represents an array value.
  void ensure_array() const;

  /// Ensure that the JSON represents an object value.
  void ensure_object() const;

  /// Ensure that the JSON represents an empty array or object.
  void ensure_empty() const;

  /// Ensure that the JSON object contains a key.
  void ensure_key(std::string_view key) const;

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Get a boolean value.
  auto as_bool() const -> bool;

  /// Get an integer value.
  auto as_int() const -> int64_t;

  /// Get a floating-point value.
  auto as_float() const -> float64_t;

  /// Get a string value.
  auto as_string() const -> std::string_view;

  /// Iterate over an array value.
  auto as_array() const -> std::generator<JSON>;

  /// Iterate over an object value.
  auto as_object() const -> std::generator<std::pair<std::string_view, JSON>>;

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Append a non-null item to the end of a JSON array.
  void append(JSON item);

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Get a value from a JSON object by key, or throw an error.
  auto get(std::string_view key) const -> JSON;

  /// Get a value from a JSON object by key, or return a default (null) value.
  auto get(std::string_view key, const JSON& def) const -> JSON;

  /// Get a value from a JSON object and remove it from the object, or throw an
  /// error.
  auto pop(std::string_view key) const -> JSON;

  /// Get a value from a JSON object and remove it from the object, or return a
  /// default (null) value.
  auto pop(std::string_view key, const JSON& def = null()) -> JSON;

  /// Set a value in a JSON object by key. If the value is null, do nothing.
  void set(std::string_view key, JSON value);

  /// Set a value in a JSON object by key, ensuring the key is not already set.
  /// If the value is null, do nothing.
  void push(std::string_view key, JSON value);

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  explicit JSON(nlohmann::json json) noexcept;

  nlohmann::json json_;

}; // class JSON

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::prop
