/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <algorithm>
#include <format>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

#include "tit/core/basic_types.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/str.hpp"
#include "tit/data/param_spec.hpp"

namespace tit::data {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace {

auto pop(json::json& json, std::string_view key) -> json::json {
  TIT_ENSURE(json.contains(key), "Missing '{}' key.", key);

  auto value = json.at(key);
  json.erase(key);

  return value;
}

template<class Val>
auto pop(json::json& json, std::string_view key) -> Val {
  return pop(json, key).get<Val>();
}

template<class Val>
auto maybe_pop(json::json& json, std::string_view key) -> std::optional<Val> {
  if (json.contains(key)) return pop<Val>(json, key);
  return std::nullopt;
}

template<class Val>
void maybe_set(json::json& json,
               std::string_view key,
               std::optional<Val> value) {
  if (value.has_value()) json[key] = *value;
}

} // namespace

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto param_spec_type_to_string(ParamSpecType type) -> std::string_view {
  using enum ParamSpecType;
  switch (type) {
    case bool_:  return "bool";
    case int_:   return "int";
    case float_: return "float";
    case str:    return "string";
    case enum_:  return "enum";
    case record: return "record";
  }
  std::unreachable();
}

auto param_spec_type_from_string(std::string_view type) -> ParamSpecType {
  using enum ParamSpecType;
  if (type == "bool") return bool_;
  if (type == "int") return int_;
  if (type == "float") return float_;
  if (type == "string") return str;
  if (type == "enum") return enum_;
  if (type == "record") return record;
  TIT_THROW("Unknown specification type name: '{}'.", type);
}

auto param_spec_type_from_json(const json::json& json) -> ParamSpecType {
  return param_spec_type_from_string(json.get<std::string>());
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto ParamSpec::from_json(json::json json) -> ParamSpecPtr {
  const auto name = pop<std::string>(json, "name");

  try {
    ParamSpecPtr spec;

    using enum ParamSpecType;
    switch (param_spec_type_from_json(pop(json, "type"))) {
      case bool_:  spec = BoolParamSpec::from_json(json); break;
      case int_:   spec = IntParamSpec::from_json(json); break;
      case float_: spec = FloatParamSpec::from_json(json); break;
      case str:    spec = StrParamSpec::from_json(json); break;
      case enum_:  spec = EnumParamSpec::from_json(json); break;
      case record: spec = RecordParamSpec::from_json(json); break;
    }

    spec->name_ = name;

    TIT_ENSURE(json.empty(),
               "Parameter specification contains extra keys: '{}'.",
               json.dump(/*indent=*/2));

    return spec;
  } catch (const Exception& e) {
    throw Exception(
        std::format("Error while parsing parameter '{}' specification. {}",
                    name,
                    e.what()),
        e.where(),
        e.when());
  } catch (json::json::exception& e) {
    TIT_THROW("Error while parsing parameter '{}' specification. {}",
              name,
              e.what());
  }
}

auto ParamSpec::to_json() const -> json::json {
  return {
      {"name", name_},
      {"type", param_spec_type_to_string(type())},
  };
}

auto ParamSpec::from_string(std::string_view spec) -> ParamSpecPtr {
  return from_json(json::json::parse(spec));
}

auto ParamSpec::to_string() const -> std::string {
  return to_json().dump();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto BoolParamSpec::from_json(json::json& json) -> BoolParamSpecPtr {
  auto spec = std::make_unique<BoolParamSpec>();

  spec->default_ = maybe_pop<bool>(json, "default");

  return spec;
}

auto BoolParamSpec::to_json() const -> json::json {
  auto json = ParamSpec::to_json();

  maybe_set(json, "default", default_);

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

  spec->default_ = maybe_pop<int64_t>(json, "default");
  spec->min_ = maybe_pop<int64_t>(json, "min");
  spec->max_ = maybe_pop<int64_t>(json, "max");

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

  return spec;
}

auto IntParamSpec::to_json() const -> json::json {
  auto json = ParamSpec::to_json();

  maybe_set(json, "default", default_);
  maybe_set(json, "min", min_);
  maybe_set(json, "max", max_);

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

  spec->default_ = maybe_pop<float64_t>(json, "default");
  spec->min_ = maybe_pop<float64_t>(json, "min");
  spec->max_ = maybe_pop<float64_t>(json, "max");
  spec->unit_ = maybe_pop<std::string>(json, "unit");

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

  return spec;
}

auto FloatParamSpec::to_json() const -> json::json {
  auto json = ParamSpec::to_json();

  maybe_set(json, "default", default_);
  maybe_set(json, "min", min_);
  maybe_set(json, "max", max_);
  maybe_set(json, "unit", unit_);

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

  spec->default_ = maybe_pop<std::string>(json, "default");

  return spec;
}

auto StrParamSpec::to_json() const -> json::json {
  auto json = ParamSpec::to_json();

  maybe_set(json, "default", default_);

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

  spec->options_ = pop<std::vector<std::string>>(json, "options");
  spec->default_ = maybe_pop<std::string>(json, "default");

  if (spec->default_.has_value()) {
    TIT_ENSURE(std::ranges::contains(spec->options_, spec->default_.value()),
               "Default value '{}' is not in options '{}'.",
               spec->default_.value(),
               spec->options_);
  }

  return spec;
}

auto EnumParamSpec::to_json() const -> json::json {
  auto json = ParamSpec::to_json();

  json["options"] = options_;
  maybe_set(json, "default", default_);

  return json;
}

auto EnumParamSpec::type() const noexcept -> ParamSpecType {
  return ParamSpecType::enum_;
}

void EnumParamSpec::validate(std::string_view value) const {
  TIT_ENSURE(std::ranges::contains(options_, value),
             "Value '{}' is not in options '{}'.",
             value,
             options_);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto RecordParamSpec::from_json(json::json& /*json*/) -> RecordParamSpecPtr {
  auto spec = std::make_unique<RecordParamSpec>();

  // Nothing else to do.

  return spec;
}

auto RecordParamSpec::to_json() const -> json::json {
  auto json = ParamSpec::to_json();

  // Nothing else to do.

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

} // namespace tit::data
