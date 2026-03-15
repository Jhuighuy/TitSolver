/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <algorithm>
#include <format>
#include <memory>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/str.hpp"
#include "tit/prop/spec.hpp"
#include "tit/prop/tree.hpp"

namespace tit::prop {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// SpecType
//

auto spec_type_is_scalar(SpecType type) -> bool {
  using enum SpecType;
  switch (type) {
    case Bool:
    case Int:
    case Real:
    case String:
    case Enum:    return true;
    case Array:
    case Record:
    case Variant:
    case App:     return false;
  }
  std::unreachable();
}

auto spec_type_to_string(SpecType type) -> std::string_view {
  using enum SpecType;
  switch (type) {
    case Bool:    return "bool";
    case Int:     return "int";
    case Real:    return "real";
    case String:  return "string";
    case Enum:    return "enum";
    case Array:   return "array";
    case Record:
    case Variant:
    case App:     return "record";
  }
  std::unreachable();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// BoolSpec
//

auto BoolSpec::type() const noexcept -> SpecType {
  return SpecType::Bool;
}

auto BoolSpec::default_value(bool val) && -> BoolSpec&& {
  default_ = val;
  return std::move(*this);
}

auto BoolSpec::default_value() const noexcept -> std::optional<bool> {
  return default_;
}

void BoolSpec::validate(Tree& tree, std::string_view path) const {
  if (tree.is_null()) {
    TIT_ENSURE(default_.has_value(),
               "'{}': required boolean property is missing.",
               path);
    tree.set(default_.value());
    return;
  }

  const auto value = tree.to_bool();
  TIT_ENSURE(value.has_value(),
             "'{}': expected boolean, got {}.",
             path,
             tree.type_name());
  tree.set(*value);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// IntSpec
//

auto IntSpec::type() const noexcept -> SpecType {
  return SpecType::Int;
}

auto IntSpec::min() const noexcept -> std::optional<int64_t> {
  return min_;
}

auto IntSpec::min(int64_t val) && -> IntSpec&& {
  if (max_.has_value()) {
    TIT_ENSURE(val <= max_.value(),
               "Min value '{}' must be <= max value '{}'.",
               val,
               max_.value());
  }
  if (default_.has_value()) {
    TIT_ENSURE(val <= default_.value(),
               "Min value '{}' must be <= default value '{}'.",
               val,
               default_.value());
  }

  min_ = val;
  return std::move(*this);
}

auto IntSpec::max() const noexcept -> std::optional<int64_t> {
  return max_;
}

auto IntSpec::max(int64_t val) && -> IntSpec&& {
  if (min_.has_value()) {
    TIT_ENSURE(val >= min_.value(),
               "Max value '{}' must be >= min value '{}'.",
               val,
               min_.value());
  }
  if (default_.has_value()) {
    TIT_ENSURE(val >= default_.value(),
               "Max value '{}' must be >= default value '{}'.",
               val,
               default_.value());
  }

  max_ = val;
  return std::move(*this);
}

auto IntSpec::range(int64_t min_val, int64_t max_val) && -> IntSpec&& {
  return std::move(*this).min(min_val).max(max_val);
}

auto IntSpec::default_value() const noexcept -> std::optional<int64_t> {
  return default_;
}

auto IntSpec::default_value(int64_t val) && -> IntSpec&& {
  if (min_.has_value()) {
    TIT_ENSURE(val >= min_.value(),
               "Default value '{}' must be >= min value '{}'.",
               val,
               min_.value());
  }
  if (max_.has_value()) {
    TIT_ENSURE(val <= max_.value(),
               "Default value '{}' must be <= max value '{}'.",
               val,
               max_.value());
  }

  default_ = val;
  return std::move(*this);
}

void IntSpec::validate(Tree& tree, std::string_view path) const {
  if (tree.is_null()) {
    TIT_ENSURE(default_.has_value(),
               "'{}': required integer property is missing.",
               path);
    tree.set(default_.value());
    return;
  }

  const auto value = tree.to_int();
  TIT_ENSURE(value.has_value(),
             "'{}': expected integer, got {}.",
             path,
             tree.type_name());
  tree.set(*value);

  const auto val = tree.as_int();
  if (min_.has_value()) {
    TIT_ENSURE(val >= min_.value(),
               "'{}': value {} is below minimum {}.",
               path,
               val,
               min_.value());
  }
  if (max_.has_value()) {
    TIT_ENSURE(val <= max_.value(),
               "'{}': value {} is above maximum {}.",
               path,
               val,
               max_.value());
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// RealSpec
//

auto RealSpec::type() const noexcept -> SpecType {
  return SpecType::Real;
}

auto RealSpec::min() const noexcept -> std::optional<float64_t> {
  return min_;
}

auto RealSpec::min(float64_t val) && -> RealSpec&& {
  if (max_.has_value()) {
    TIT_ENSURE(val <= max_.value(),
               "Min value '{}' must be <= max value '{}'.",
               val,
               max_.value());
  }
  if (default_.has_value()) {
    TIT_ENSURE(val <= default_.value(),
               "Default value '{}' must be >= min value '{}'.",
               default_.value(),
               val);
  }

  min_ = val;
  return std::move(*this);
}

auto RealSpec::max() const noexcept -> std::optional<float64_t> {
  return max_;
}

auto RealSpec::max(float64_t val) && -> RealSpec&& {
  if (min_.has_value()) {
    TIT_ENSURE(val >= min_.value(),
               "Max value '{}' must be >= min value '{}'.",
               val,
               min_.value());
  }
  if (default_.has_value()) {
    TIT_ENSURE(val >= default_.value(),
               "Max value '{}' must be >= default value '{}'.",
               val,
               default_.value());
  }

  max_ = val;
  return std::move(*this);
}

auto RealSpec::range(float64_t min_val, float64_t max_val) && -> RealSpec&& {
  return std::move(*this).min(min_val).max(max_val);
}

auto RealSpec::default_value() const noexcept -> std::optional<float64_t> {
  return default_;
}

auto RealSpec::default_value(float64_t val) && -> RealSpec&& {
  if (min_.has_value()) {
    TIT_ENSURE(val >= min_.value(),
               "Default value '{}' must be >= min value '{}'.",
               val,
               min_.value());
  }
  if (max_.has_value()) {
    TIT_ENSURE(val <= max_.value(),
               "Default value '{}' must be <= max value '{}'.",
               val,
               max_.value());
  }

  default_ = val;
  return std::move(*this);
}

void RealSpec::validate(Tree& tree, std::string_view path) const {
  if (tree.is_null()) {
    TIT_ENSURE(default_.has_value(),
               "'{}': required real property is missing.",
               path);
    tree.set(default_.value());
    return;
  }

  const auto value = tree.to_real();
  TIT_ENSURE(value.has_value(),
             "'{}': expected number, got {}.",
             path,
             tree.type_name());
  tree.set(*value);

  const auto val = tree.as_real();
  if (min_.has_value()) {
    TIT_ENSURE(val >= min_.value(),
               "'{}': value {} is below minimum {}.",
               path,
               val,
               min_.value());
  }
  if (max_.has_value()) {
    TIT_ENSURE(val <= max_.value(),
               "'{}': value {} is above maximum {}.",
               path,
               val,
               max_.value());
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// StringSpec
//

auto StringSpec::type() const noexcept -> SpecType {
  return SpecType::String;
}

auto StringSpec::default_value() const noexcept
    -> const std::optional<std::string>& {
  return default_;
}

auto StringSpec::default_value(std::string val) && -> StringSpec&& {
  default_ = std::move(val);
  return std::move(*this);
}

void StringSpec::validate(Tree& tree, std::string_view path) const {
  if (tree.is_null()) {
    TIT_ENSURE(default_.has_value(),
               "'{}': required string property is missing.",
               path);
    tree.set(default_.value());
    return;
  }

  const auto scalar = tree.to_string();
  TIT_ENSURE(scalar.has_value(),
             "'{}': expected string scalar, got {}.",
             path,
             tree.type_name());
  tree.set(*scalar);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// EnumSpec
//

auto EnumSpec::type() const noexcept -> SpecType {
  return SpecType::Enum;
}

auto EnumSpec::options() const noexcept -> const std::vector<Option>& {
  return options_;
}

auto EnumSpec::option(std::string_view id) const -> const Option* {
  const auto iter = std::ranges::find(
      options_,
      id,
      [](const Option& option) -> std::string_view { return option.id; });
  return iter == options_.end() ? nullptr : std::to_address(iter);
}

auto EnumSpec::option(std::string_view id,
                      std::string_view name) && -> EnumSpec&& {
  TIT_ENSURE(str_is_identifier(id),
             "Enum option ID '{}' must be a valid identifier.",
             id);
  TIT_ENSURE(option(id) == nullptr, "Enum option ID '{}' is duplicate.", id);

  options_.emplace_back(std::string{id}, std::string{name});
  return std::move(*this);
}

auto EnumSpec::default_value() const noexcept
    -> const std::optional<std::string>& {
  return default_;
}

auto EnumSpec::default_value(std::string val) && -> EnumSpec&& {
  if (!options_.empty()) {
    TIT_ENSURE(option(val) != nullptr,
               "Enum default value '{}' is not a valid option ID.",
               val);
  }

  default_ = std::move(val);
  return std::move(*this);
}

void EnumSpec::validate(Tree& tree, std::string_view path) const {
  if (tree.is_null()) {
    TIT_ENSURE(default_.has_value(),
               "'{}': required enum property is missing.",
               path);
    tree.set(default_.value());
    return;
  }

  const auto scalar = tree.to_string();
  TIT_ENSURE(scalar.has_value(),
             "'{}': expected string scalar, got {}.",
             path,
             tree.type_name());
  tree.set(*scalar);

  const auto val = tree.as_string();
  TIT_ENSURE(option(val) != nullptr,
             "'{}': '{}' is not a valid enum value.",
             path,
             val);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// ArraySpec
//

auto ArraySpec::type() const noexcept -> SpecType {
  return SpecType::Array;
}

auto ArraySpec::item() const -> const Spec& {
  TIT_ASSERT(item_spec_ != nullptr, "Item spec is not set!");
  return *item_spec_;
}

auto ArraySpec::item(SpecPtr spec) && -> ArraySpec&& {
  TIT_ASSERT(spec != nullptr, "Array item spec must not be null.");

  item_spec_ = std::move(spec);
  return std::move(*this);
}

void ArraySpec::validate(Tree& tree, std::string_view path) const {
  if (tree.is_null()) {
    tree.set(Tree::Array{});
    return;
  }

  TIT_ENSURE(tree.is_array(),
             "'{}': expected array, got {}.",
             path,
             tree.type_name());

  // Validate each element.
  for (size_t i = 0; i < tree.size(); ++i) {
    item().validate(tree.get(i), std::format("{}[{}]", path, i));
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// RecordSpec
//

auto RecordSpec::type() const noexcept -> SpecType {
  return SpecType::Record;
}

auto RecordSpec::fields() const noexcept -> const std::vector<Field>& {
  return fields_;
}

auto RecordSpec::field(std::string_view id) const -> const Field* {
  const auto iter = std::ranges::find(
      fields_,
      id,
      [](const Field& field) -> std::string_view { return field.id; });
  return iter == fields_.end() ? nullptr : std::to_address(iter);
}

auto RecordSpec::field(std::string_view id,
                       std::string_view name,
                       SpecPtr spec) && -> RecordSpec&& {
  TIT_ASSERT(spec != nullptr, "Record field spec must not be null.");
  TIT_ENSURE(str_is_identifier(id),
             "Record field ID '{}' must be a valid identifier.",
             id);
  TIT_ENSURE(field(id) == nullptr, "Record field ID '{}' is duplicate.", id);

  fields_.emplace_back(std::string{id}, std::string{name}, std::move(spec));
  return std::move(*this);
}

void RecordSpec::validate(Tree& tree, std::string_view path) const {
  if (tree.is_null()) tree.set(Tree::Map{});
  TIT_ENSURE(tree.is_map(),
             "'{}': expected object (record), got {}.",
             path,
             tree.type_name());

  // Check for unknown keys.
  for (const auto key : tree.keys()) {
    TIT_ENSURE(field(key) != nullptr, "'{}': unknown field '{}'.", path, key);
  }

  // Validate each field.
  for (const auto& field : fields_) {
    field.spec->validate(tree.get(field.id),
                         str_join_nonempty({path, field.id}, "."));
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// VariantSpec
//

auto VariantSpec::type() const noexcept -> SpecType {
  return SpecType::Variant;
}

auto VariantSpec::options() const noexcept -> const std::vector<Option>& {
  return options_;
}

auto VariantSpec::option(std::string_view id) const -> const Option* {
  const auto iter = std::ranges::find(
      options_,
      id,
      [](const Option& option) -> std::string_view { return option.id; });
  return iter == options_.end() ? nullptr : std::to_address(iter);
}

auto VariantSpec::option(std::string_view id,
                         std::string_view name,
                         SpecPtr spec) && -> VariantSpec&& {
  TIT_ASSERT(spec != nullptr, "Variant option spec must not be null.");
  TIT_ENSURE(id != "_active", "Variant option ID '_active' is reserved.");
  TIT_ENSURE(str_is_identifier(id),
             "Variant option ID '{}' must be a valid identifier.",
             id);
  TIT_ENSURE(option(id) == nullptr, "Variant option ID '{}' is duplicate.", id);

  options_.emplace_back(std::string{id}, std::string{name}, std::move(spec));
  return std::move(*this);
}

auto VariantSpec::default_value() const noexcept
    -> const std::optional<std::string>& {
  return default_;
}

auto VariantSpec::default_value(std::string val) && -> VariantSpec&& {
  if (!options_.empty()) {
    TIT_ENSURE(option(val) != nullptr,
               "Variant default value '{}' is not a valid option ID.",
               val);
  }

  default_ = std::move(val);
  return std::move(*this);
}

void VariantSpec::validate(Tree& tree, std::string_view path) const {
  if (tree.is_null()) {
    tree.set(Tree::Map{});
  } else {
    TIT_ENSURE(tree.is_map(),
               "'{}': expected object (variant), got {}.",
               path,
               tree.type_name());
  }

  // Check for unknown keys.
  for (const auto key : tree.keys()) {
    TIT_ENSURE(key == "_active" || option(key) != nullptr,
               "'{}': unknown option '{}'. "
               "Expected one of: {}.",
               path,
               key,
               options_ | std::views::transform(&Option::id));
  }

  // Get the active option.
  auto& active_node = tree.get("_active");
  if (active_node.is_null() && default_.has_value()) {
    active_node.set(std::string{*default_});
  } else {
    TIT_ENSURE(active_node.is_string(),
               "'{}': value of '_active' must be a string, got {}.",
               path,
               active_node.type_name());
  }
  TIT_ENSURE(!active_node.is_null(),
             "'{}': variant has no active option. "
             "Provide '_active' key with one of: {}.",
             path,
             options_ | std::views::transform(&Option::id));

  const auto active_id = active_node.as_string();
  const auto* const active_option = option(active_id);
  TIT_ENSURE(active_option != nullptr,
             "'{}': variant active option '{}' is not valid. "
             "Expected one of: {}.",
             path,
             active_id,
             options_ | std::views::transform(&Option::id));

  // Validate the active option.
  active_option->spec->validate(tree.get(active_id),
                                str_join_nonempty({path, active_id}, "."));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// AppSpec
//

auto AppSpec::type() const noexcept -> SpecType {
  return SpecType::App;
}

auto AppSpec::name() const -> std::string_view {
  TIT_ENSURE(!name_.empty(), "App name must be set.");
  return name_;
}

auto AppSpec::name(std::string name) && -> AppSpec&& {
  TIT_ENSURE(!name.empty(), "App name must not be empty.");
  name_ = std::move(name);
  return std::move(*this);
}

auto AppSpec::description() const -> std::string_view {
  TIT_ENSURE(!description_.empty(), "App description must be set.");
  return description_;
}

auto AppSpec::description(std::string desc) && -> AppSpec&& {
  TIT_ENSURE(!desc.empty(), "App description must not be empty.");
  description_ = std::move(desc);
  return std::move(*this);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::prop
