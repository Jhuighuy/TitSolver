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
#include "tit/data/spec.hpp"

namespace tit::data {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace {

template<class Val>
auto pop(json::json& json, std::string_view key) -> Val {
  TIT_ENSURE(json.contains(key), "Missing '{}' key.", key);

  const auto value = json.at(key);
  json.erase(key);

  return value.get<Val>();
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

auto spec_type_to_string(SpecType type) -> std::string_view {
  using enum SpecType;
  switch (type) {
    case bool_:  return "bool";
    case int_:   return "int";
    case float_: return "float";
    case string: return "string";
    case enum_:  return "enum";
    case record: return "record";
  }
  std::unreachable();
}

auto spec_type_from_string(std::string_view type) -> SpecType {
  using enum SpecType;
  if (type == "bool") return bool_;
  if (type == "int") return int_;
  if (type == "float") return float_;
  if (type == "string") return string;
  if (type == "enum") return enum_;
  if (type == "record") return record;
  TIT_THROW("Unknown specification type name: '{}'.", type);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto Spec::from_json(json::json json) -> SpecPtr {
  const auto name = pop<std::string>(json, "name");

  try {
    SpecPtr spec;

    using enum SpecType;
    switch (spec_type_from_string(pop<std::string>(json, "type"))) {
      case bool_:  spec = BoolSpec::from_json(json); break;
      case int_:   spec = IntSpec::from_json(json); break;
      case float_: spec = FloatSpec::from_json(json); break;
      case string: spec = StringSpec::from_json(json); break;
      case enum_:  spec = EnumSpec::from_json(json); break;
      case record: spec = RecordSpec::from_json(json); break;
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

auto Spec::to_json() const -> json::json {
  return {
      {"name", name_},
      {"type", spec_type_to_string(type())},
  };
}

auto Spec::from_string(std::string_view spec) -> SpecPtr {
  return from_json(json::json::parse(spec));
}

auto Spec::to_string() const -> std::string {
  return to_json().dump();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto BoolSpec::from_json(json::json& json) -> BoolSpecPtr {
  auto spec = std::make_unique<BoolSpec>();

  spec->default_ = maybe_pop<bool>(json, "default");

  return spec;
}

auto BoolSpec::to_json() const -> json::json {
  auto json = Spec::to_json();

  maybe_set(json, "default", default_);

  return json;
}

auto BoolSpec::type() const noexcept -> SpecType {
  return SpecType::bool_;
}

void BoolSpec::validate(std::string_view value) const {
  // Note: `str_to<bool>` is too loose for our needs, so we do our own checking.
  TIT_ENSURE(value == "true" || value == "false",
             "Value '{}' is not a boolean (must be 'true' or 'false').",
             value);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto IntSpec::from_json(json::json& json) -> IntSpecPtr {
  auto spec = std::make_unique<IntSpec>();

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

auto IntSpec::to_json() const -> json::json {
  auto json = Spec::to_json();

  maybe_set(json, "default", default_);
  maybe_set(json, "min", min_);
  maybe_set(json, "max", max_);

  return json;
}

auto IntSpec::type() const noexcept -> SpecType {
  return SpecType::int_;
}

void IntSpec::validate(std::string_view value) const {
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

auto FloatSpec::from_json(json::json& json) -> FloatSpecPtr {
  auto spec = std::make_unique<FloatSpec>();

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

auto FloatSpec::to_json() const -> json::json {
  auto json = Spec::to_json();

  maybe_set(json, "default", default_);
  maybe_set(json, "min", min_);
  maybe_set(json, "max", max_);
  maybe_set(json, "unit", unit_);

  return json;
}

auto FloatSpec::type() const noexcept -> SpecType {
  return SpecType::float_;
}

void FloatSpec::validate(std::string_view value) const {
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

auto StringSpec::from_json(json::json& json) -> StringSpecPtr {
  auto spec = std::make_unique<StringSpec>();

  spec->default_ = maybe_pop<std::string>(json, "default");

  return spec;
}

auto StringSpec::to_json() const -> json::json {
  auto json = Spec::to_json();

  maybe_set(json, "default", default_);

  return json;
}

auto StringSpec::type() const noexcept -> SpecType {
  return SpecType::string;
}

void StringSpec::validate(std::string_view /*value*/) const {
  // Nothing to do.
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto EnumSpec::from_json(json::json& json) -> EnumSpecPtr {
  auto spec = std::make_unique<EnumSpec>();

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

auto EnumSpec::to_json() const -> json::json {
  auto json = Spec::to_json();

  json["options"] = options_;
  maybe_set(json, "default", default_);

  return json;
}

auto EnumSpec::type() const noexcept -> SpecType {
  return SpecType::enum_;
}

void EnumSpec::validate(std::string_view value) const {
  TIT_ENSURE(std::ranges::contains(options_, value),
             "Value '{}' is not in options '{}'.",
             value,
             options_);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto RecordSpec::from_json(json::json& /*json*/) -> RecordSpecPtr {
  auto spec = std::make_unique<RecordSpec>();

  // Nothing else to do.

  return spec;
}

auto RecordSpec::to_json() const -> json::json {
  auto json = Spec::to_json();

  // Nothing else to do.

  return json;
}

auto RecordSpec::type() const noexcept -> SpecType {
  return SpecType::record;
}

void RecordSpec::validate(std::string_view value) const {
  TIT_ENSURE(value.empty(),
             "Record specification cannot have a non-empty value.");
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::data
