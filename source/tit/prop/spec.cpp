/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "tit/core/exception.hpp"
#include "tit/prop/json-2.hpp"
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

auto spec_type_to_json(SpecType type) -> JSON {
  return JSON::from_string(spec_type_to_string(type));
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

auto spec_type_from_json(const JSON& json) -> SpecType {
  return spec_type_from_string(json.as_string());
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Specification.
//

auto Spec::from_json(JSON json) -> SpecPtr {
  using enum SpecType;
  switch (spec_type_from_json(json.pop(TYPE_KEY))) {
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
  return from_json(JSON::parse(string));
}

auto Spec::to_json() const -> JSON {
  auto json = JSON::object();

  json.set(TYPE_KEY, spec_type_to_json(type()));

  return json;
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

auto BoolSpec::from_json(JSON json) -> BoolSpecPtr {
  auto spec = std::make_unique<BoolSpec>();

  spec->init_ = json.pop(INIT_KEY).as_opt_bool();
  spec->true_label_ = json.pop(TRUE_LABEL_KEY).as_opt_string();
  spec->false_label_ = json.pop(FALSE_LABEL_KEY).as_opt_string();

  TIT_ENSURE(spec->true_label_.has_value() == spec->false_label_.has_value(),
             "Either both or none of 'true_label' and 'false_label' must be "
             "specified.");

  json.ensure_empty();

  return spec;
}

auto BoolSpec::to_json() const -> JSON {
  auto json = Spec::to_json();

  json.set(INIT_KEY, JSON::from_bool(init_));
  json.set(TRUE_LABEL_KEY, JSON::from_string(true_label_));
  json.set(FALSE_LABEL_KEY, JSON::from_string(false_label_));

  return json;
}

auto BoolSpec::initial_value() const -> JSON {
  return JSON::from_bool(init_);
}

void BoolSpec::validate_value(JSON value) const {
  value.ensure_bool();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Integer Specification.
//

auto IntSpec::type() const noexcept -> SpecType {
  return SpecType::int_;
}

auto IntSpec::from_json(JSON json) -> IntSpecPtr {
  auto spec = std::make_unique<IntSpec>();

  spec->min_ = json.pop(MIN_KEY).as_opt_int();
  spec->max_ = json.pop(MAX_KEY).as_opt_int();

  if (spec->min_.has_value() && spec->max_.has_value()) {
    TIT_ENSURE(spec->min_.value() <= spec->max_.value(),
               "Minimum value '{}' must be less than or equal to maximum '{}'.",
               spec->min_.value(),
               spec->max_.value());
  }

  spec->init_ = json.pop(INIT_KEY).as_opt_int();

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

  json.ensure_empty();

  return spec;
}

auto IntSpec::to_json() const -> JSON {
  auto json = Spec::to_json();

  json.set(INIT_KEY, JSON::from_int(init_));
  json.set(MIN_KEY, JSON::from_int(min_));
  json.set(MAX_KEY, JSON::from_int(max_));

  return json;
}

auto IntSpec::initial_value() const -> JSON {
  return JSON::from_int(init_);
}

void IntSpec::validate_value(JSON value) const {
  const auto value_int = value.as_int();

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

auto FloatSpec::from_json(JSON json) -> FloatSpecPtr {
  auto spec = std::make_unique<FloatSpec>();

  spec->min_ = json.pop(MIN_KEY).as_opt_float();
  spec->max_ = json.pop(MAX_KEY).as_opt_float();

  if (spec->min_.has_value() && spec->max_.has_value()) {
    TIT_ENSURE(spec->min_.value() <= spec->max_.value(),
               "Minimum value '{}' must be less than or equal to maximum '{}'.",
               spec->min_.value(),
               spec->max_.value());
  }

  spec->init_ = json.pop(INIT_KEY).as_opt_float();

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

  spec->unit_ = json.pop(UNIT_KEY).as_opt_string();

  json.ensure_empty();

  return spec;
}

auto FloatSpec::to_json() const -> JSON {
  auto json = Spec::to_json();

  json.set(INIT_KEY, JSON::from_float(init_));
  json.set(MIN_KEY, JSON::from_float(min_));
  json.set(MAX_KEY, JSON::from_float(max_));
  json.set(UNIT_KEY, JSON::from_string(unit_));

  return json;
}

auto FloatSpec::initial_value() const -> JSON {
  return JSON::from_float(init_);
}

void FloatSpec::validate_value(JSON value) const {
  const auto value_float = value.as_float();

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

auto StrSpec::from_json(JSON json) -> StrSpecPtr {
  auto spec = std::make_unique<StrSpec>();

  spec->init_ = json.pop(INIT_KEY).as_opt_string();

  json.ensure_empty();

  return spec;
}

auto StrSpec::to_json() const -> JSON {
  auto json = Spec::to_json();

  json.set(INIT_KEY, JSON::from_string(init_));

  return json;
}

auto StrSpec::type() const noexcept -> SpecType {
  return SpecType::str;
}

auto StrSpec::initial_value() const -> JSON {
  return JSON::from_string(init_);
}

void StrSpec::validate_value(JSON value) const {
  value.ensure_string();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Enumeration Specification.
//

auto EnumSpec::type() const noexcept -> SpecType {
  return SpecType::enum_;
}

auto EnumOptionSpec::from_json(JSON json) -> EnumOptionSpec {
  EnumOptionSpec spec;

  if (json.is_string()) {
    spec.name_ = json.as_string();
  } else {
    spec.name_ = json.pop(NAME_KEY).as_string();
    spec.descr_ = json.pop(DESCR_KEY).as_opt_string();

    json.ensure_empty();
  }

  return spec;
}

auto EnumSpec::from_json(JSON json) -> EnumSpecPtr {
  auto spec = std::make_unique<EnumSpec>();

  for (auto option_json : json.pop(OPTIONS_KEY).iter_array()) {
    auto option = EnumOptionSpec::from_json(std::move(option_json));

    TIT_ENSURE(spec->find_option(option.name()) == nullptr,
               "Duplicate enum option name '{}'.",
               option.name());

    spec->options_.push_back(std::move(option));
  }

  spec->init_ = json.pop(INIT_KEY).as_opt_string();

  if (spec->init_.has_value()) {
    TIT_ENSURE(spec->find_option(spec->init_.value()) != nullptr,
               "Default value '{}' is not in options '{}'.",
               spec->init_.value(),
               spec->option_names());
  }

  json.ensure_empty();

  return spec;
}

auto EnumOptionSpec::to_json() const -> JSON {
  if (descr_.has_value()) {
    auto json = JSON::object();

    json.set(NAME_KEY, JSON::from_string(name_));
    json.set(DESCR_KEY, JSON::from_string(descr_.value()));

    return json;
  }

  return JSON::from_string(name_);
}

auto EnumSpec::to_json() const -> JSON {
  auto options_json = JSON::array();
  for (const auto& option : options_) options_json.append(option.to_json());

  auto json = Spec::to_json();

  json.set(OPTIONS_KEY, std::move(options_json));
  json.set(INIT_KEY, JSON::from_string(init_));

  return json;
}

auto EnumSpec::initial_value() const -> JSON {
  return JSON::from_string(init_);
}

void EnumSpec::validate_value(JSON value) const {
  const auto value_str = value.as_string();

  TIT_ENSURE(find_option(value_str) != nullptr,
             "Value '{}' is not in options '{}'.",
             value_str,
             option_names());
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Array Specification.
//

auto ArraySpec::type() const noexcept -> SpecType {
  return SpecType::array;
}

auto ArraySpec::from_json(JSON json) -> ArraySpecPtr {
  auto spec = std::make_unique<ArraySpec>();

  spec->item_spec_ = Spec::from_json(json.pop(ITEM_KEY));

  json.ensure_empty();

  return spec;
}

auto ArraySpec::to_json() const -> JSON {
  auto json = Spec::to_json();

  json.set(ITEM_KEY, item_spec_->to_json());

  return json;
}

void ArraySpec::validate_value(JSON value) const {
  for (auto item : value.iter_array()) {
    item_spec_->validate_value(std::move(item));
  }
}

auto ArraySpec::initial_value() const -> JSON {
  // Note: Arrays always have a default value of `[]`.
  return JSON::array();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Record Specification.
//

auto RecordSpec::type() const noexcept -> SpecType {
  return SpecType::record;
}

auto RecordFieldSpec::from_json(JSON json) -> RecordFieldSpec {
  RecordFieldSpec spec;

  spec.name_ = json.pop(NAME_KEY).as_string();
  spec.descr_ = json.pop(DESCR_KEY).as_opt_string();
  spec.val_spec_ = Spec::from_json(json.pop(SPEC_KEY));

  json.ensure_empty();

  return spec;
}

auto RecordSpec::from_json(JSON json) -> RecordSpecPtr {
  auto spec = std::make_unique<RecordSpec>();

  for (auto field_json : json.pop(FIELDS_KEY).iter_array()) {
    auto field = RecordFieldSpec::from_json(std::move(field_json));

    TIT_ENSURE(spec->find_field(field.name()) == nullptr,
               "Duplicate field name '{}'.",
               field.name());

    spec->fields_.push_back(std::move(field));
  }

  json.ensure_empty();

  return spec;
}

auto RecordFieldSpec::to_json() const -> JSON {
  auto json = JSON::object();

  json.set(NAME_KEY, JSON::from_string(name_));
  json.set(DESCR_KEY, JSON::from_string(descr_));
  json.set(SPEC_KEY, val_spec_->to_json());

  return json;
}

auto RecordSpec::to_json() const -> JSON {
  auto fields_json = JSON::array();
  for (const auto& field : fields_) fields_json.append(field.to_json());

  auto json = Spec::to_json();

  json.set(FIELDS_KEY, std::move(fields_json));

  return json;
}

auto RecordFieldSpec::initial_value() const -> JSON {
  return val_spec_->initial_value();
}

auto RecordSpec::initial_value() const -> JSON {
  // Note: Records always have a default value of at least `{}`.
  JSON value = JSON::object();

  for (const auto& field : fields_) {
    value.set(field.name(), field.initial_value());
  }

  return value;
}

void RecordFieldSpec::validate_value(JSON value) const {
  val_spec_->validate_value(std::move(value));
}

void RecordSpec::validate_value(JSON value) const {
  // Note: We validate only the present fields.
  for (const auto& field : fields_) {
    if (auto field_value = value.get(field.name()); !field_value.is_null()) {
      field.validate_value(std::move(field_value));
    }
  }

  // Make sure there are no unexpected fields.
  value.ensure_empty();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Variant Specification.
//

auto VariantSpec::type() const noexcept -> SpecType {
  return SpecType::variant;
}

auto VariantOptionSpec::from_json(JSON json) -> VariantOptionSpec {
  VariantOptionSpec spec;

  spec.name_ = json.pop(NAME_KEY).as_string();
  spec.descr_ = json.pop(DESCR_KEY).as_opt_string();
  spec.val_spec_ = Spec::from_json(json.pop(SPEC_KEY));

  json.ensure_empty();

  return spec;
}

auto VariantSpec::from_json(JSON json) -> VariantSpecPtr {
  auto spec = std::make_unique<VariantSpec>();

  for (auto option_json : json.pop(OPTIONS_KEY).iter_array()) {
    auto option = VariantOptionSpec::from_json(std::move(option_json));

    TIT_ENSURE(spec->find_option(option.name()) == nullptr,
               "Duplicate variant option name '{}'.",
               option.name());

    spec->options_.push_back(std::move(option));
  }

  spec->init_ = json.pop(INIT_KEY).as_opt_string();
  if (spec->init_.has_value()) {
    TIT_ENSURE(spec->find_option(spec->init_.value()) != nullptr,
               "Default value '{}' is not in options '{}'.",
               spec->init_.value(),
               spec->option_names());
  }

  json.ensure_empty();

  return spec;
}

auto VariantOptionSpec::to_json() const -> JSON {
  JSON json = JSON::object();

  json.set(NAME_KEY, JSON::from_string(name_));
  json.set(DESCR_KEY, JSON::from_string(descr_));
  json.set(SPEC_KEY, val_spec_->to_json());

  return json;
}

auto VariantSpec::to_json() const -> JSON {
  auto options_json = JSON::array();
  for (const auto& option : options_) options_json.append(option.to_json());

  auto json = Spec::to_json();

  json.set(OPTIONS_KEY, std::move(options_json));
  json.set(INIT_KEY, JSON::from_string(init_));

  return json;
}

auto VariantOptionSpec::initial_value() const -> JSON {
  return val_spec_->initial_value();
}

auto VariantSpec::initial_value() const -> JSON {
  // Note: Variants always have a default value of at least `{}`.
  JSON value = JSON::object();

  for (const auto& option : options_) {
    value.set(option.name(), option.initial_value());
  }

  value.set(VARIANT_KEY, JSON::from_string(init_));

  return value;
}

void VariantOptionSpec::validate_value(JSON value) const {
  val_spec_->validate_value(std::move(value));
}

void VariantSpec::validate_value(JSON value) const {
  if (const auto option_name = value.pop(VARIANT_KEY).as_opt_string();
      option_name.has_value()) {
    const auto* const option = find_option(option_name.value());

    TIT_ENSURE(option != nullptr,
               "Value '{}' is not in options '{}'.",
               option_name.value(),
               option_names());
  }

  // Note: We validate only the present options.
  for (const auto& option : options_) {
    if (auto option_value = value.pop(option.name()); !option_value.is_null()) {
      option.validate_value(std::move(option_value));
    }
  }

  // Make sure there are no unexpected options.
  value.ensure_empty();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// NOLINTEND(*-no-recursion)

} // namespace tit::prop
