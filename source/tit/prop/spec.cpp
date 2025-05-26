/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <algorithm>
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
#include "tit/prop/json.hpp"
#include "tit/prop/spec.hpp"

namespace tit::prop {
namespace {

// NOLINTBEGIN(*-no-recursion)

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

constexpr std::string_view DESCR_KEY = "description";
constexpr std::string_view FALSE_LABEL_KEY = "false_label";
constexpr std::string_view FIELDS_KEY = "fields";
constexpr std::string_view INIT_KEY = "init";
constexpr std::string_view ITEM_KEY = "item";
constexpr std::string_view MAX_KEY = "max";
constexpr std::string_view MIN_KEY = "min";
constexpr std::string_view NAME_KEY = "name";
constexpr std::string_view OPTIONS_KEY = "options";
constexpr std::string_view SPEC_KEY = "spec";
constexpr std::string_view TRUE_LABEL_KEY = "true_label";
constexpr std::string_view TYPE_KEY = "type";
constexpr std::string_view UNIT_KEY = "unit";
constexpr std::string_view VARIANT_KEY = "__variant__";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace

auto spec_type_to_string(SpecType type) -> std::string_view {
  using enum SpecType;
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

auto spec_type_from_string(std::string_view string) -> SpecType {
  using enum SpecType;
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

auto spec_type_from_json(const json::JSON& json) -> SpecType {
  return spec_type_from_string(json.get<std::string>());
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Specification.
//

auto Spec::from_json(json::JSON json) -> SpecPtr {
  using enum SpecType;
  switch (spec_type_from_json(json::pop(json, TYPE_KEY))) {
    case bool_:   return BoolSpec::from_json(std::move(json));
    case int_:    return IntSpec::from_json(std::move(json));
    case float_:  return FloatSpec::from_json(std::move(json));
    case str:     return StrSpec::from_json(std::move(json));
    case enum_:   return EnumSpec::from_json(std::move(json));
    case array:   return ArraySpec::from_json(std::move(json));
    case record:  return RecordSpec::from_json(std::move(json));
    case variant: return VariantSpec::from_json(std::move(json));
  }
  std::unreachable();
}

auto Spec::from_string(std::string_view string) -> SpecPtr {
  return from_json(json::JSON::parse(string));
}

auto Spec::to_string() const -> std::string {
  return to_json().dump();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Boolean Specification.
//

auto BoolSpec::type() const noexcept -> SpecType {
  return SpecType::bool_;
}

auto BoolSpec::from_json(json::JSON json) -> BoolSpecPtr {
  auto spec = std::make_unique<BoolSpec>();

  spec->init_ = json::maybe_pop<bool>(json, INIT_KEY);

  spec->true_label_ = json::maybe_pop<std::string>(json, TRUE_LABEL_KEY);
  spec->false_label_ = json::maybe_pop<std::string>(json, FALSE_LABEL_KEY);
  if (spec->false_label_.has_value()) {
    TIT_ENSURE(spec->true_label_.has_value(),
               "Either both or none of 'true_label' and 'false_label' must be "
               "specified.");
  }

  json::ensure_empty(json, "Boolean specification");

  return spec;
}

auto BoolSpec::to_json() const -> json::JSON {
  json::JSON json;

  json::maybe_set(json, INIT_KEY, init_);
  json::maybe_set(json, TRUE_LABEL_KEY, true_label_);
  json::maybe_set(json, FALSE_LABEL_KEY, false_label_);

  return json;
}

auto BoolSpec::initial_value() const -> json::JSON {
  return init_.has_value() ? json::JSON{init_.value()} : json::null;
}

void BoolSpec::validate_value(json::JSON value) const {
  TIT_ENSURE(value.is_boolean(), "Value '{}' is not a boolean.", value.dump());
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Integer Specification.
//

auto IntSpec::type() const noexcept -> SpecType {
  return SpecType::int_;
}

auto IntSpec::from_json(json::JSON json) -> IntSpecPtr {
  auto spec = std::make_unique<IntSpec>();

  spec->min_ = json::maybe_pop<int64_t>(json, MIN_KEY);
  spec->max_ = json::maybe_pop<int64_t>(json, MAX_KEY);
  if (spec->min_.has_value() && spec->max_.has_value()) {
    TIT_ENSURE(spec->min_.value() <= spec->max_.value(),
               "Minimum value '{}' must be less than or equal to maximum '{}'.",
               spec->min_.value(),
               spec->max_.value());
  }

  spec->init_ = json::maybe_pop<int64_t>(json, INIT_KEY);
  if (spec->init_.has_value() && spec->min_.has_value()) {
    TIT_ENSURE(
        spec->init_.value() >= spec->min_.value(),
        "Default value '{}' must be greater than or equal to minimum '{}'.",
        spec->init_.value(),
        spec->min_.value());
  }
  if (spec->init_.has_value() && spec->max_.has_value()) {
    TIT_ENSURE(spec->init_.value() <= spec->max_.value(),
               "Default value '{}' must be less than or equal to maximum '{}'.",
               spec->init_.value(),
               spec->max_.value());
  }

  json::ensure_empty(json, "Integer specification");

  return spec;
}

auto IntSpec::to_json() const -> json::JSON {
  json::JSON json;

  json::maybe_set(json, INIT_KEY, init_);
  json::maybe_set(json, MIN_KEY, min_);
  json::maybe_set(json, MAX_KEY, max_);

  return json;
}

auto IntSpec::initial_value() const -> json::JSON {
  return init_.has_value() ? json::JSON{init_.value()} : json::null;
}

void IntSpec::validate_value(json::JSON value) const {
  TIT_ENSURE(value.is_number_integer(),
             "Value '{}' is not an integer.",
             value.dump());

  const auto value_int = value.get<int64_t>();

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
//
// Floating-point Specification.
//

auto FloatSpec::type() const noexcept -> SpecType {
  return SpecType::float_;
}

auto FloatSpec::from_json(json::JSON json) -> FloatSpecPtr {
  auto spec = std::make_unique<FloatSpec>();

  spec->min_ = json::maybe_pop<float64_t>(json, MIN_KEY);
  spec->max_ = json::maybe_pop<float64_t>(json, MAX_KEY);
  if (spec->min_.has_value() && spec->max_.has_value()) {
    TIT_ENSURE(spec->min_.value() <= spec->max_.value(),
               "Minimum value '{}' must be less than or equal to maximum '{}'.",
               spec->min_.value(),
               spec->max_.value());
  }

  spec->init_ = json::maybe_pop<float64_t>(json, INIT_KEY);
  if (spec->init_.has_value() && spec->min_.has_value()) {
    TIT_ENSURE(
        spec->init_.value() >= spec->min_.value(),
        "Default value '{}' must be greater than or equal to minimum '{}'.",
        spec->init_.value(),
        spec->min_.value());
  }
  if (spec->init_.has_value() && spec->max_.has_value()) {
    TIT_ENSURE(spec->init_.value() <= spec->max_.value(),
               "Default value '{}' must be less than or equal to maximum '{}'.",
               spec->init_.value(),
               spec->max_.value());
  }

  spec->unit_ = json::maybe_pop<std::string>(json, UNIT_KEY);

  json::ensure_empty(json, "Floating-point specification");

  return spec;
}

auto FloatSpec::to_json() const -> json::JSON {
  json::JSON json;

  json::maybe_set(json, INIT_KEY, init_);
  json::maybe_set(json, MIN_KEY, min_);
  json::maybe_set(json, MAX_KEY, max_);
  json::maybe_set(json, UNIT_KEY, unit_);

  return json;
}

auto FloatSpec::initial_value() const -> json::JSON {
  return init_.has_value() ? json::JSON{init_.value()} : json::null;
}

void FloatSpec::validate_value(json::JSON value) const {
  TIT_ENSURE(value.is_number(), "Value '{}' is not a number.", value.dump());

  const auto value_float = value.get<float64_t>();

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
//
// String Specification.
//

auto StrSpec::from_json(json::JSON json) -> StrSpecPtr {
  auto spec = std::make_unique<StrSpec>();

  spec->init_ = json::maybe_pop<std::string>(json, INIT_KEY);

  json::ensure_empty(json, "String specification");

  return spec;
}

auto StrSpec::to_json() const -> json::JSON {
  json::JSON json;
  json::maybe_set(json, INIT_KEY, init_);
  return json;
}

auto StrSpec::type() const noexcept -> SpecType {
  return SpecType::str;
}

auto StrSpec::initial_value() const -> json::JSON {
  return init_.has_value() ? json::JSON{init_.value()} : json::null;
}

void StrSpec::validate_value(json::JSON value) const {
  TIT_ENSURE(value.is_string(), "Value '{}' is not a string.", value.dump());
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Enumeration Specification.
//

auto EnumSpec::type() const noexcept -> SpecType {
  return SpecType::enum_;
}

auto EnumOptionSpec::from_json(json::JSON json) -> EnumOptionSpecPtr {
  auto spec = std::make_unique<EnumOptionSpec>();

  if (json.is_string()) {
    spec->name_ = json.get<std::string>();
  } else {
    spec->name_ = json::pop<std::string>(json, NAME_KEY);
    spec->descr_ = json::maybe_pop<std::string>(json, DESCR_KEY);

    json::ensure_empty(json, "Enumeration option specification");
  }

  return spec;
}

auto EnumSpec::from_json(json::JSON json) -> EnumSpecPtr {
  auto spec = std::make_unique<EnumSpec>();

  std::unordered_set<std::string_view> option_names;
  for (auto& option_json : json::pop(json, OPTIONS_KEY)) {
    auto option = EnumOptionSpec::from_json(option_json);
    TIT_ENSURE(option_names.insert(option->name()).second,
               "Duplicate enum option name '{}'.",
               option->name());

    spec->options_.push_back(std::move(option));
  }

  spec->init_ = json::maybe_pop<std::string>(json, INIT_KEY);
  if (spec->init_.has_value()) {
    TIT_ENSURE(option_names.contains(spec->init_.value()),
               "Default value '{}' is not in options '{}'.",
               spec->init_.value(),
               option_names);
  }

  json::ensure_empty(json, "Enumeration specification");

  return spec;
}

auto EnumOptionSpec::to_json() const -> json::JSON {
  if (!descr_.has_value()) return name_;
  return {{NAME_KEY, name_}, {DESCR_KEY, *descr_}};
}

auto EnumSpec::to_json() const -> json::JSON {
  json::JSON options_json;
  for (const auto& option : options_) options_json.push_back(option->to_json());

  json::JSON json{{OPTIONS_KEY, std::move(options_json)}};
  json::maybe_set(json, INIT_KEY, init_);
  return json;
}

auto EnumSpec::initial_value() const -> json::JSON {
  return init_.has_value() ? json::JSON{init_.value()} : json::null;
}

void EnumSpec::validate_value(json::JSON value) const {
  TIT_ENSURE(value.is_string(), "Value '{}' is not a string.", value.dump());
  const auto value_str = value.get<std::string>();

  constexpr auto option_name = [](const auto& option) {
    return option->name();
  };
  TIT_ENSURE(std::ranges::contains(options_, value_str, option_name),
             "Value '{}' is not in options '{}'.",
             value_str,
             options_ | std::views::transform(option_name));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Array Specification.
//

auto ArraySpec::type() const noexcept -> SpecType {
  return SpecType::array;
}

auto ArraySpec::from_json(json::JSON json) -> ArraySpecPtr {
  auto spec = std::make_unique<ArraySpec>();

  spec->item_spec_ = Spec::from_json(json::pop(json, ITEM_KEY));

  json::ensure_empty(json, "Array specification");

  return spec;
}

auto ArraySpec::to_json() const -> json::JSON {
  return {{ITEM_KEY, item_spec_->to_json()}};
}

auto ArraySpec::initial_value() const -> json::JSON {
  return json::JSON::array();
}

void ArraySpec::validate_value(json::JSON value) const {
  TIT_ENSURE(value.is_array(), "Value '{}' is not an array.", value.dump());
  for (const auto& item : value) item_spec_->validate_value(item);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Record Specification.
//

auto RecordSpec::type() const noexcept -> SpecType {
  return SpecType::record;
}

auto RecordFieldSpec::from_json(json::JSON json) -> RecordFieldSpecPtr {
  auto spec = std::make_unique<RecordFieldSpec>();

  spec->name_ = json::pop<std::string>(json, NAME_KEY);
  spec->descr_ = json::maybe_pop<std::string>(json, DESCR_KEY);
  spec->val_spec_ = Spec::from_json(json::pop(json, SPEC_KEY));

  json::ensure_empty(json, "Record field specification");

  return spec;
}

auto RecordSpec::from_json(json::JSON json) -> RecordSpecPtr {
  auto spec = std::make_unique<RecordSpec>();

  std::unordered_set<std::string_view> field_names;
  for (auto& field_json : json::pop(json, FIELDS_KEY)) {
    auto field = RecordFieldSpec::from_json(field_json);
    TIT_ENSURE(field_names.insert(field->name()).second,
               "Duplicate field name '{}'.",
               field->name());

    spec->fields_.push_back(std::move(field));
  }

  json::ensure_empty(json, "Record specification");

  return spec;
}

auto RecordFieldSpec::to_json() const -> json::JSON {
  json::JSON json{
      {NAME_KEY, name_},
      {SPEC_KEY, val_spec_->to_json()},
  };

  json::maybe_set(json, DESCR_KEY, descr_);

  return json;
}

auto RecordSpec::to_json() const -> json::JSON {
  json::JSON fields_json;
  for (const auto& field : fields_) fields_json.push_back(field->to_json());

  return {{FIELDS_KEY, std::move(fields_json)}};
}

auto RecordFieldSpec::initial_value() const -> json::JSON {
  return val_spec_->initial_value();
}

auto RecordSpec::initial_value() const -> json::JSON {
  json::JSON value;

  for (const auto& field : fields_) {
    if (auto field_value = field->initial_value(); !field_value.is_null()) {
      value[field->name()] = std::move(field_value);
    }
  }

  return value;
}

void RecordFieldSpec::validate_value(json::JSON value) const {
  val_spec_->validate_value(std::move(value));
}

void RecordSpec::validate_value(json::JSON value) const {
  TIT_ENSURE(value.is_object(), "Value '{}' is not an object.", value.dump());

  for (const auto& field : fields_) {
    field->validate_value(json::pop(value, field->name()));
  }

  json::ensure_empty(value, "Record value");
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Variant Specification.
//

auto VariantSpec::type() const noexcept -> SpecType {
  return SpecType::variant;
}

auto VariantOptionSpec::from_json(json::JSON json) -> VariantOptionSpecPtr {
  auto spec = std::make_unique<VariantOptionSpec>();

  spec->name_ = json::pop<std::string>(json, NAME_KEY);
  spec->descr_ = json::maybe_pop<std::string>(json, DESCR_KEY);
  spec->val_spec_ = Spec::from_json(json::pop(json, SPEC_KEY));

  json::ensure_empty(json, "Variant option specification");

  return spec;
}

auto VariantSpec::from_json(json::JSON json) -> VariantSpecPtr {
  auto spec = std::make_unique<VariantSpec>();

  std::unordered_set<std::string_view> option_names;
  for (auto& option_json : json::pop(json, OPTIONS_KEY)) {
    auto option = VariantOptionSpec::from_json(option_json);
    TIT_ENSURE(option_names.insert(option->name()).second,
               "Duplicate variant option name '{}'.",
               option->name());

    spec->options_.push_back(std::move(option));
  }

  spec->init_ = json::maybe_pop<std::string>(json, INIT_KEY);
  if (spec->init_.has_value()) {
    TIT_ENSURE(option_names.contains(spec->init_.value()),
               "Default value '{}' is not in options '{}'.",
               spec->init_.value(),
               option_names);
  }

  json::ensure_empty(json, "Variant specification");

  return spec;
}

auto VariantOptionSpec::to_json() const -> json::JSON {
  json::JSON json{
      {NAME_KEY, name_},
      {SPEC_KEY, val_spec_->to_json()},
  };

  json::maybe_set(json, DESCR_KEY, descr_);

  return json;
}

auto VariantSpec::to_json() const -> json::JSON {
  json::JSON options_json;
  for (const auto& option : options_) options_json.push_back(option->to_json());

  json::JSON json{{OPTIONS_KEY, std::move(options_json)}};
  json::maybe_set(json, INIT_KEY, init_);

  return json;
}

auto VariantOptionSpec::initial_value() const -> json::JSON {
  return val_spec_->initial_value();
}

auto VariantSpec::initial_value() const -> json::JSON {
  json::JSON value;

  if (init_.has_value()) value[VARIANT_KEY] = init_.value();

  for (const auto& option : options_) {
    if (auto option_value = option->initial_value(); !option_value.is_null()) {
      value[option->name()] = std::move(option_value);
    }
  }
  return value;
}

void VariantOptionSpec::validate_value(json::JSON value) const {
  val_spec_->validate_value(std::move(value));
}

void VariantSpec::validate_value(json::JSON value) const {
  TIT_ENSURE(value.is_object(), "Value '{}' is not an object.", value.dump());

  const auto variant = json::pop<std::string>(value, VARIANT_KEY);
  constexpr auto option_name = [](const auto& option) {
    return option->name();
  };
  TIT_ENSURE(std::ranges::contains(options_, variant, option_name),
             "Value '{}' is not in options '{}'.",
             variant,
             options_ | std::views::transform(option_name));

  for (const auto& option : options_) {
    option->validate_value(json::pop(value, option->name()));
  }

  TIT_ENSURE(value.empty(),
             "Variant value contains extra keys: '{}'.",
             value.dump(/*indent=*/2));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// NOLINTEND(*-no-recursion)

} // namespace tit::prop
