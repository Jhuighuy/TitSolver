/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <algorithm>
#include <memory>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

#include "tit/core/assert.hpp"
#include "tit/core/basic_types.hpp"
#include "tit/core/exception.hpp"
#include "tit/data/spec.hpp"

namespace tit::prop {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto prop_type_to_string(PropType type) -> std::string_view {
  using enum PropType;
  switch (type) {
    case bool_:   return "bool";
    case int_:    return "int";
    case float_:  return "float";
    case string:  return "string";
    case record:  return "record";
    case variant: return "variant";
    default:      TIT_THROW("Unknown property type: {}", static_cast<int>(type));
  }
}

auto prop_type_from_string(std::string_view type) -> PropType {
  using enum PropType;
  if (type == "bool") return bool_;
  if (type == "int") return int_;
  if (type == "float") return float_;
  if (type == "string") return string;
  if (type == "record") return record;
  if (type == "variant") return variant;
  TIT_THROW("Unknown property type name: {}", type);
}

auto prop_type_from_json(const nlohmann::json& json) -> PropType {
  TIT_ENSURE(json.is_string(), "Property type must be a string.");
  return prop_type_from_string(json.get<std::string>());
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto PropSpec::from_json(const nlohmann::json& json) -> PropSpecPtr {
  TIT_ENSURE(json.is_object(), "Property specification must be a JSON object.");
  TIT_ENSURE(json.contains("type"), "Property specification must have a type.");
  using enum PropType;
  switch (prop_type_from_json(json.at("type"))) {
    case bool_:   return std::make_unique<BoolPropSpec>(json);
    case int_:    return std::make_unique<IntPropSpec>(json);
    case float_:  return std::make_unique<FloatPropSpec>(json);
    case string:  return std::make_unique<StringPropSpec>(json);
    case record:  return std::make_unique<RecordPropSpec>(json);
    case variant: return std::make_unique<VariantPropSpec>(json);
    default:      std::unreachable();
  }
}

PropSpec::PropSpec(const nlohmann::json& json) {
  TIT_ENSURE(json.is_object(), "Property specification must be a JSON object.");

  TIT_ENSURE(json.contains("name"), "Property specification must have a name.");
  const auto& name = json.at("name");
  TIT_ENSURE(name.is_string(),
             "Property name specification must be a string (is '{}').",
             name.type_name());
  name_ = name.get<std::string>();

  TIT_ENSURE(json.contains("description"),
             "Property '{}' specification does not contain description",
             this->name());
  const auto& description = json.at("description");
  TIT_ENSURE(
      description.is_string(),
      "Property '{}' description specification must be a string (is '{}').",
      this->name(),
      description.type_name());
  description_ = json.at("description").get<std::string>();
}

// NOLINTBEGIN(bugprone-exception-escape) -- bug in clang-tidy.

auto PropSpec::to_json() const -> nlohmann::json {
  return {
      {"name", name_},
      {"description", description_},
      {"type", prop_type_to_string(type())},
  };
}

// NOLINTEND(bugprone-exception-escape)

namespace {

auto prop_specs_from_json(const nlohmann::json& json)
    -> std::vector<PropSpecPtr> {
  TIT_ASSERT(json.is_array(), "Invalid property specifications JSON!");
  return std::views::transform(json, &PropSpec::from_json) |
         std::ranges::to<std::vector>();
}

auto prop_specs_to_json(std::span<const PropSpecPtr> specs) -> nlohmann::json {
  return std::views::transform(
             specs,
             [](const auto& spec) { return spec->to_json(); }) |
         std::ranges::to<nlohmann::json>();
}

} // namespace

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

BoolPropSpec::BoolPropSpec(const nlohmann::json& json) : PropSpec{json} {
  if (json.contains("default_value")) {
    const auto& default_value = json.at("default");
    TIT_ENSURE(default_value.is_boolean(),
               "Boolean property '{}' default value specification must be "
               "a boolean (is '{}').",
               name(),
               default_value.type_name());
    default_value_ = default_value.get<bool>();
  }
}

auto BoolPropSpec::to_json() const -> nlohmann::json {
  auto json = PropSpec::to_json();
  if (default_value_.has_value()) json["default_value"] = *default_value_;
  return json;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

template class NumericPropSpec<int64_t>;
template class NumericPropSpec<float64_t>;

template<class Num>
NumericPropSpec<Num>::NumericPropSpec(const nlohmann::json& json)
    : PropSpec{json} {
  if (json.contains("default_value")) {
    const auto& default_value = json.at("default");
    TIT_ENSURE(default_value.is_number(),
               "Numeric property '{}' default value specification must be "
               "a number (is '{}').",
               name(),
               default_value.type_name());
    default_value_ = default_value.get<Num>();
  }
  if (json.contains("min_value")) {
    const auto& min_value = json.at("min");
    TIT_ENSURE(min_value.is_number(),
               "Numeric property '{}' minimum value specification must be "
               "a number (is '{}').",
               name(),
               min_value.type_name());
    min_value_ = min_value.get<Num>();
  }
  if (json.contains("max_value")) {
    const auto& max_value = json.at("max");
    TIT_ENSURE(max_value.is_number(),
               "Invalid numeric property specification JSON!");
    max_value_ = max_value.get<Num>();
  }
  validate_default_and_range_();
}

template<class Num>
void NumericPropSpec<Num>::validate_default_and_range_() const {
  if (min_value_.has_value() && max_value_.has_value()) {
    TIT_ENSURE(min_value_.value() < max_value_.value(),
               "Numeric property '{}' minimum value '{}' must be less than "
               "maximum value '{}'.",
               name(),
               min_value_.value(),
               max_value_.value());
  }
  if (default_value_.has_value() && min_value_.has_value()) {
    TIT_ENSURE(default_value_.value() >= min_value_.value(),
               "Numeric property '{}' default value '{}' must be greater than "
               "or equal to minimum value '{}'.",
               name(),
               default_value_.value(),
               min_value_.value());
  }
  if (default_value_.has_value() && max_value_.has_value()) {
    TIT_ENSURE(default_value_.value() <= max_value_.value(),
               "Numeric property '{}' default value '{}' must be less than or "
               "equal to maximum value '{}'.",
               name(),
               default_value_.value(),
               max_value_.value());
  }
}

template<class Num>
auto NumericPropSpec<Num>::to_json() const -> nlohmann::json {
  auto json = PropSpec::to_json();
  if (default_value_.has_value()) json["default_value"] = *default_value_;
  if (min_value_.has_value()) json["min_value"] = *min_value_;
  if (max_value_.has_value()) json["max_value"] = *max_value_;
  return json;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

StringPropSpec::StringPropSpec(const nlohmann::json& json) : PropSpec{json} {
  if (json.contains("default_value")) {
    const auto& default_value = json.at("default");
    TIT_ENSURE(default_value.is_string(),
               "String property '{}' default value specification must be "
               "a string (is '{}').",
               name(),
               default_value.type_name());
    default_value_ = default_value.get<std::string>();
  }
}

auto StringPropSpec::to_json() const -> nlohmann::json {
  auto json = PropSpec::to_json();
  if (default_value_.has_value()) json["default_value"] = *default_value_;
  return json;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

RecordPropSpec::RecordPropSpec(const nlohmann::json& json) : PropSpec{json} {
  // NOTE: We are using arrays here, because we want to preserve the order of
  //       fields in the JSON. JSON objects are unordered by the standard.
  const auto& fields = json.at("fields");
  TIT_ENSURE(fields.is_array(),
             "Record property '{}' fields specification must be "
             "an array (is '{}').",
             name(),
             fields.type_name());
  try {
    fields_ = prop_specs_from_json(fields);
  } catch (const Exception& e) {
    TIT_RETHROW(e, "Record property '{}' fields", name());
  }
}

auto RecordPropSpec::to_json() const -> nlohmann::json {
  auto json = PropSpec::to_json();
  json["fields"] = prop_specs_to_json(fields_);
  return json;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

VariantPropSpec::VariantPropSpec(const nlohmann::json& json) : PropSpec{json} {
  // NOTE: We are using arrays here, because we want to preserve the order of
  //       fields in the JSON. JSON objects are unordered by the standard.
  const auto& options = json.at("options");
  TIT_ENSURE(options.is_array(),
             "Variant property '{}' options specification must be "
             "an array (is '{}').",
             name(),
             options.type_name());
  try {
    options_ = prop_specs_from_json(options);
  } catch (const Exception& e) {
    TIT_RETHROW(e, "Variant property '{}' options", name());
  }

  if (json.contains("default_option")) {
    const auto& default_option = json.at("default");
    TIT_ENSURE(default_option.is_string(),
               "Variant property '{}' default option specification must be "
               "a string (is '{}').",
               name(),
               default_option.type_name());
    default_option_ = default_option.get<std::string>();
    validate_default_option_();
  }
}

void VariantPropSpec::validate_default_option_() const {
  if (!default_option_.has_value()) return;
  TIT_ENSURE(
      std::ranges::contains(options_,
                            default_option_.value(),
                            [](const auto& option) { return option->name(); }),
      "Variant property '{}' default option '{}' does not exist.",
      name(),
      default_option_.value());
}

auto VariantPropSpec::to_json() const -> nlohmann::json {
  auto json = PropSpec::to_json();
  json["options"] = prop_specs_to_json(options_);
  if (default_option_.has_value()) json["default_option"] = *default_option_;
  return json;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::prop
