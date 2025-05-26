/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <algorithm>
#include <cctype>
#include <format>
#include <memory>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/str.hpp"
#include "tit/data/json.hpp"
#include "tit/data/param_spec.hpp"

namespace tit::data {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

inline constexpr std::string_view DEFAULT = "default";
inline constexpr std::string_view FALSE_LABEL = "false_label";
inline constexpr std::string_view FIELDS = "fields";
inline constexpr std::string_view ITEM = "item";
inline constexpr std::string_view LABEL = "label";
inline constexpr std::string_view MAX = "max";
inline constexpr std::string_view MIN = "min";
inline constexpr std::string_view NAME = "name";
inline constexpr std::string_view OPTIONS = "options";
inline constexpr std::string_view TRUE_LABEL = "true_label";
inline constexpr std::string_view TYPE = "type";
inline constexpr std::string_view UNIT = "unit";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Validate a parameter/option name: must follow the same rules as C++ names.
void validate_name(std::string_view name) {
  TIT_ENSURE(!name.empty(), "Parameter name must not be empty.");
  TIT_ENSURE(std::isalpha(name.front()) || name.front() == '_',
             "Parameter name must start with a letter or underscore.");
  TIT_ENSURE(
      std::ranges::all_of(name.substr(1),
                          [](char c) { return std::isalnum(c) || c == '_'; }),
      "Parameter name must contain only letters, numbers, and "
      "underscores.");
}

// Validate a parameter/option label. Exact rules are TBD.
void validate_label(std::string_view label) {
  TIT_ENSURE(!label.empty(), "Label must not be empty.");

  TIT_ENSURE((std::isalpha(label.front()) &&
              label.front() == std::toupper(label.front())),
             "Label must start with a capital letter.");

  TIT_ENSURE(!std::isspace(label.back()), "Label must not end with a space.");
  TIT_ENSURE(label.find("  ") == std::string_view::npos,
             "Label must not contain consecutive spaces.");
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace

auto param_spec_type_to_string(ParamSpecType type) -> std::string_view {
  using enum ParamSpecType;
  switch (type) {
    case bool_:   return "bool";
    case int_:    return "int";
    case float_:  return "float";
    case str:     return "string";
    case enum_:   return "enum";
    case array:   return "array";
    case record:  return "record";
    case variant: return "variant";
  }
  std::unreachable();
}

auto param_spec_type_from_string(std::string_view string) -> ParamSpecType {
  using enum ParamSpecType;
  if (string == "bool") return bool_;
  if (string == "int") return int_;
  if (string == "float") return float_;
  if (string == "string") return str;
  if (string == "enum") return enum_;
  if (string == "array") return array;
  if (string == "record") return record;
  if (string == "variant") return variant;
  TIT_THROW("Unknown specification type name: '{}'.", string);
}

auto param_spec_type_from_json(const json::json& json) -> ParamSpecType {
  TIT_ENSURE(json.is_string(),
             "Expected string for parameter type specification, got '{}'.",
             json.dump(/*indent=*/2));
  return param_spec_type_from_string(json.get<std::string>());
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// NOLINTNEXTLINE(*-no-recursion)
auto ParamSpec::from_json(json::json json) -> ParamSpecPtr {
  auto name = json::maybe_pop<std::string>(json, NAME);
  if (name.has_value()) validate_name(name.value());

  try {
    ParamSpecPtr spec;

    using enum ParamSpecType;
    switch (param_spec_type_from_json(json::pop(json, TYPE))) {
      case bool_:   spec = BoolParamSpec::from_json(json); break;
      case int_:    spec = IntParamSpec::from_json(json); break;
      case float_:  spec = FloatParamSpec::from_json(json); break;
      case str:     spec = StrParamSpec::from_json(json); break;
      case enum_:   spec = EnumParamSpec::from_json(json); break;
      case array:   spec = ArrayParamSpec::from_json(json); break;
      case record:  spec = RecordParamSpec::from_json(json); break;
      case variant: spec = VariantParamSpec::from_json(json); break;
    }

    spec->label_ = json::maybe_pop<std::string>(json, LABEL);
    if (spec->label_.has_value()) {
      TIT_ENSURE(name.has_value(),
                 "Label is only allowed for named parameters.");
      validate_label(spec->label_.value());
    }

    TIT_ENSURE(json.empty(),
               "Parameter specification contains extra keys: '{}'.",
               json.dump(/*indent=*/2));

    spec->name_ = std::move(name);
    return spec;
  } catch (const Exception& e) {
    throw Exception(
        std::format("Error while parsing parameter '{}' specification. {}",
                    name.value_or("<unnamed>"),
                    e.what()),
        e.where(),
        e.when());
  } catch (json::json::exception& e) {
    TIT_THROW("Error while parsing parameter '{}' specification. {}",
              name.value_or("<unnamed>"),
              e.what());
  }
}

auto ParamSpec::to_json() const -> json::json {
  json::json json{
      {NAME, name_},
      {TYPE, param_spec_type_to_string(type())},
  };
  json::maybe_set(json, LABEL, label_);

  return json;
}

auto ParamSpec::from_string(std::string_view string) -> ParamSpecPtr {
  try {
    return from_json(json::json::parse(string));
  } catch (const json::json::parse_error& e) {
    TIT_THROW("Error while parsing parameter specification. {}", e.what());
  }
}

auto ParamSpec::to_string() const -> std::string {
  return to_json().dump();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto BoolParamSpec::from_json(json::json& json) -> BoolParamSpecPtr {
  auto spec = std::make_unique<BoolParamSpec>();

  spec->default_ = json::maybe_pop<bool>(json, DEFAULT);

  spec->true_label_ = json::maybe_pop<std::string>(json, TRUE_LABEL);
  if (spec->true_label_.has_value()) {
    validate_label(spec->true_label_.value());
  }

  spec->false_label_ = json::maybe_pop<std::string>(json, FALSE_LABEL);
  if (spec->false_label_.has_value()) {
    TIT_ENSURE(spec->true_label_.has_value(),
               "Either both or none of 'true_label' and 'false_label' must be "
               "specified.");
    validate_label(spec->false_label_.value());
  }

  return spec;
}

auto BoolParamSpec::to_json() const -> json::json {
  auto json = ParamSpec::to_json();

  json::maybe_set(json, DEFAULT, default_);
  json::maybe_set(json, TRUE_LABEL, true_label_);
  json::maybe_set(json, FALSE_LABEL, false_label_);

  return json;
}

auto BoolParamSpec::type() const noexcept -> ParamSpecType {
  return ParamSpecType::bool_;
}

void BoolParamSpec::validate(std::string_view value) const {
  // Note: `str_to<bool>` is too loose for our needs, so we do our own checking.
  TIT_ENSURE(value == "true" || value == "false",
             "Value '{}' is not a boolean (must be 'true' or 'false').",
             value);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto IntParamSpec::from_json(json::json& json) -> IntParamSpecPtr {
  auto spec = std::make_unique<IntParamSpec>();

  spec->min_ = json::maybe_pop<int64_t>(json, MIN);
  spec->max_ = json::maybe_pop<int64_t>(json, MAX);
  if (spec->min_.has_value() && spec->max_.has_value()) {
    TIT_ENSURE(spec->min_.value() <= spec->max_.value(),
               "Minimum value '{}' must be less than or equal to maximum '{}'.",
               spec->min_.value(),
               spec->max_.value());
  }

  spec->default_ = json::maybe_pop<int64_t>(json, DEFAULT);
  if (spec->default_.has_value() && spec->min_.has_value()) {
    TIT_ENSURE(
        spec->default_.value() >= spec->min_.value(),
        "Default value '{}' must be greater than or equal to minimum '{}'.",
        spec->default_.value(),
        spec->min_.value());
  }
  if (spec->default_.has_value() && spec->max_.has_value()) {
    TIT_ENSURE(spec->default_.value() <= spec->max_.value(),
               "Default value '{}' must be less than or equal to maximum '{}'.",
               spec->default_.value(),
               spec->max_.value());
  }

  return spec;
}

auto IntParamSpec::to_json() const -> json::json {
  auto json = ParamSpec::to_json();

  json::maybe_set(json, DEFAULT, default_);
  json::maybe_set(json, MIN, min_);
  json::maybe_set(json, MAX, max_);

  return json;
}

auto IntParamSpec::type() const noexcept -> ParamSpecType {
  return ParamSpecType::int_;
}

void IntParamSpec::validate(std::string_view value) const {
  const auto maybe_int = str_to<int64_t>(value);
  TIT_ENSURE(maybe_int.has_value(), "Value '{}' is not an integer.", value);

  const auto value_int = maybe_int.value();

  if (min_.has_value()) {
    TIT_ENSURE(value_int >= min_.value(),
               "Value '{}' is less than minimum '{}'.",
               value_int,
               min_.value());
  }

  if (max_.has_value()) {
    TIT_ENSURE(value_int <= max_.value(),
               "Value '{}' is greater than maximum '{}'.",
               value_int,
               max_.value());
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto FloatParamSpec::from_json(json::json& json) -> FloatParamSpecPtr {
  auto spec = std::make_unique<FloatParamSpec>();

  spec->default_ = json::maybe_pop<float64_t>(json, DEFAULT);
  spec->min_ = json::maybe_pop<float64_t>(json, MIN);
  spec->max_ = json::maybe_pop<float64_t>(json, MAX);

  if (spec->min_.has_value() && spec->max_.has_value()) {
    TIT_ENSURE(spec->min_.value() <= spec->max_.value(),
               "Minimum value '{}' must be less than or equal to maximum '{}'.",
               spec->min_.value(),
               spec->max_.value());
  }

  if (spec->default_.has_value() && spec->min_.has_value()) {
    TIT_ENSURE(
        spec->default_.value() >= spec->min_.value(),
        "Default value '{}' must be greater than or equal to minimum '{}'.",
        spec->default_.value(),
        spec->min_.value());
  }

  if (spec->default_.has_value() && spec->max_.has_value()) {
    TIT_ENSURE(spec->default_.value() <= spec->max_.value(),
               "Default value '{}' must be less than or equal to maximum '{}'.",
               spec->default_.value(),
               spec->max_.value());
  }

  spec->unit_ = json::maybe_pop<std::string>(json, UNIT);

  return spec;
}

auto FloatParamSpec::to_json() const -> json::json {
  auto json = ParamSpec::to_json();

  json::maybe_set(json, DEFAULT, default_);
  json::maybe_set(json, MIN, min_);
  json::maybe_set(json, MAX, max_);
  json::maybe_set(json, UNIT, unit_);

  return json;
}

auto FloatParamSpec::type() const noexcept -> ParamSpecType {
  return ParamSpecType::float_;
}

void FloatParamSpec::validate(std::string_view value) const {
  const auto maybe_float = str_to<float64_t>(value);
  TIT_ENSURE(maybe_float.has_value(), "Value '{}' is not a float.", value);

  const auto value_float = maybe_float.value();

  if (min_.has_value()) {
    TIT_ENSURE(value_float >= min_.value(),
               "Value '{}' is less than minimum '{}'.",
               value_float,
               min_.value());
  }

  if (max_.has_value()) {
    TIT_ENSURE(value_float <= max_.value(),
               "Value '{}' is greater than maximum '{}'.",
               value_float,
               max_.value());
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto StrParamSpec::from_json(json::json& json) -> StrParamSpecPtr {
  auto spec = std::make_unique<StrParamSpec>();

  spec->default_ = json::maybe_pop<std::string>(json, DEFAULT);

  return spec;
}

auto StrParamSpec::to_json() const -> json::json {
  auto json = ParamSpec::to_json();

  json::maybe_set(json, DEFAULT, default_);

  return json;
}

auto StrParamSpec::type() const noexcept -> ParamSpecType {
  return ParamSpecType::str;
}

void StrParamSpec::validate(std::string_view /*value*/) const {
  // Nothing to do.
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto EnumParamSpec::from_json(json::json& json) -> EnumParamSpecPtr {
  auto spec = std::make_unique<EnumParamSpec>();

  std::unordered_set<std::string_view> option_names;
  for (auto& option_json : json::pop(json, OPTIONS)) {
    if (option_json.is_object()) {
      auto option_name = json::pop<std::string>(option_json, NAME);
      validate_name(option_name);

      auto option_label = json::maybe_pop<std::string>(option_json, LABEL);
      if (option_label.has_value()) validate_label(option_label.value());

      TIT_ENSURE(option_json.empty(),
                 "Option '{}' contains extra keys: '{}'.",
                 spec->options_.back().name,
                 option_json.dump(/*indent=*/2));

      spec->options_.emplace_back(std::move(option_name),
                                  std::move(option_label));
    } else if (option_json.is_string()) {
      auto option_name = option_json.get<std::string>();
      validate_name(option_name);

      spec->options_.emplace_back(std::move(option_name));
    } else {
      TIT_THROW("Expected string or object for option, got '{}'.",
                option_json.dump(/*indent=*/2));
    }

    const auto& option = spec->options_.back();
    TIT_ENSURE(option_names.insert(option.name).second,
               "Duplicate option name '{}'.",
               option.name);
  }

  spec->default_ = json::maybe_pop<std::string>(json, DEFAULT);
  if (spec->default_.has_value()) {
    TIT_ENSURE(option_names.contains(spec->default_.value()),
               "Default value '{}' is not in options '{}'.",
               spec->default_.value(),
               option_names);
  }

  return spec;
}

auto EnumParamSpec::to_json() const -> json::json {
  auto json = ParamSpec::to_json();

  for (auto& options_json = json[OPTIONS];
       const auto& [name, label] : options_) {
    if (label.has_value()) {
      options_json.push_back({{NAME, name}, {LABEL, *label}});
    } else {
      options_json.push_back(name);
    }
  }

  json::maybe_set(json, DEFAULT, default_);

  return json;
}

auto EnumParamSpec::type() const noexcept -> ParamSpecType {
  return ParamSpecType::enum_;
}

void EnumParamSpec::validate(std::string_view value) const {
  constexpr auto option_name =
      [](const OptionSpec& option) -> std::string_view { return option.name; };
  TIT_ENSURE(std::ranges::contains(options_, value, option_name),
             "Value '{}' is not in options '{}'.",
             value,
             options_ | std::views::transform(option_name));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// NOLINTNEXTLINE(*-no-recursion)
auto ArrayParamSpec::from_json(json::json& json) -> ArrayParamSpecPtr {
  auto spec = std::make_unique<ArrayParamSpec>();

  spec->item_spec_ = ParamSpec::from_json(json::pop(json, ITEM));
  TIT_ENSURE(!spec->item_spec_->name().has_value(),
             "Array item specification must not have a name.");

  return spec;
}

auto ArrayParamSpec::to_json() const -> json::json {
  auto json = ParamSpec::to_json();

  json[ITEM] = item_spec_->to_json();

  return json;
}

auto ArrayParamSpec::type() const noexcept -> ParamSpecType {
  return ParamSpecType::array;
}

void ArrayParamSpec::validate(std::string_view value) const {
  TIT_ENSURE(value.empty(),
             "Array specification cannot have a non-empty value.");
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// NOLINTNEXTLINE(*-no-recursion)
auto RecordParamSpec::from_json(json::json& json) -> RecordParamSpecPtr {
  auto spec = std::make_unique<RecordParamSpec>();

  std::unordered_set<std::string_view> field_names;
  for (auto& field_json : json::pop(json, FIELDS)) {
    auto field = ParamSpec::from_json(field_json);

    const auto field_name = field->name();
    TIT_ENSURE(field_name.has_value(), "Record field must have a name.");
    TIT_ENSURE(field_names.insert(field_name.value()).second,
               "Duplicate field name '{}'.",
               field_name.value());

    spec->fields_.push_back(std::move(field));
  }

  return spec;
}

auto RecordParamSpec::to_json() const -> json::json {
  auto json = ParamSpec::to_json();

  for (auto& fields_json = json[FIELDS]; const auto& field : fields_) {
    fields_json.push_back(field->to_json());
  }

  return json;
}

auto RecordParamSpec::type() const noexcept -> ParamSpecType {
  return ParamSpecType::record;
}

void RecordParamSpec::validate(std::string_view value) const {
  TIT_ENSURE(value.empty(),
             "Record specification cannot have a non-empty value.");
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// NOLINTNEXTLINE(*-no-recursion)
auto VariantParamSpec::from_json(json::json& json) -> VariantParamSpecPtr {
  auto spec = std::make_unique<VariantParamSpec>();

  std::unordered_set<std::string_view> option_names;
  for (auto& option_json : json::pop(json, OPTIONS)) {
    auto option = ParamSpec::from_json(option_json);

    const auto option_name = option->name();
    TIT_ENSURE(option_name.has_value(), "Variant option must have a name.");
    TIT_ENSURE(option_names.insert(option_name.value()).second,
               "Duplicate option name '{}'.",
               option_name.value());

    spec->options_.push_back(std::move(option));
  }

  spec->default_ = json::maybe_pop<std::string>(json, DEFAULT);
  if (spec->default_.has_value()) {
    TIT_ENSURE(option_names.contains(spec->default_.value()),
               "Default value '{}' is not in options '{}'.",
               spec->default_.value(),
               option_names);
  }

  return spec;
}

auto VariantParamSpec::to_json() const -> json::json {
  auto json = ParamSpec::to_json();

  for (auto& options_json = json[OPTIONS]; const auto& option : options_) {
    options_json.push_back(option->to_json());
  }

  json::maybe_set(json, DEFAULT, default_);

  return json;
}

auto VariantParamSpec::type() const noexcept -> ParamSpecType {
  return ParamSpecType::variant;
}

void VariantParamSpec::validate(std::string_view value) const {
  constexpr auto option_name =
      [](const ParamSpecPtr& option) -> std::string_view {
    return option->name().value_or("<unnamed>");
  };
  TIT_ENSURE(std::ranges::contains(options_, value, option_name),
             "Value '{}' is not in options '{}'.",
             value,
             options_ | std::views::transform(option_name));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::data
