/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include <nlohmann/json.hpp> // NOLINT(misc-include-cleaner)
#include <nlohmann/json_fwd.hpp>

#include "tit/prop/spec.hpp"

namespace tit::prop {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

using JSON = nlohmann::ordered_json;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

template<class Val>
void set_optional(JSON& json,
                  std::string_view key,
                  const std::optional<Val>& opt) {
  if (opt.has_value()) json[key] = opt.value();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto spec_to_json(const Spec& spec) -> JSON {
  JSON json;

  switch (spec.type()) {
    case SpecType::Bool: {
      const auto& bool_spec = spec_cast<BoolSpec>(spec);
      json["type"] = "bool";
      set_optional(json, "default", bool_spec.default_value());
      break;
    }

    case SpecType::Int: {
      const auto& int_spec = spec_cast<IntSpec>(spec);
      json["type"] = "int";
      set_optional(json, "min", int_spec.min());
      set_optional(json, "max", int_spec.max());
      set_optional(json, "default", int_spec.default_value());
      break;
    }

    case SpecType::Real: {
      const auto& real_spec = spec_cast<RealSpec>(spec);
      json["type"] = "real";
      set_optional(json, "min", real_spec.min());
      set_optional(json, "max", real_spec.max());
      set_optional(json, "default", real_spec.default_value());
      break;
    }

    case SpecType::String: {
      const auto& str_spec = spec_cast<StringSpec>(spec);
      json["type"] = "string";
      set_optional(json, "default", str_spec.default_value());
      break;
    }

    case SpecType::Enum: {
      const auto& enum_spec = spec_cast<EnumSpec>(spec);
      json["type"] = "enum";
      auto options = JSON::array();
      for (const auto& opt : enum_spec.options()) {
        options.push_back({
            {"id", opt.id},
            {"name", opt.name},
        });
      }
      json["options"] = std::move(options);
      set_optional(json, "default", enum_spec.default_value());
      break;
    }

    case SpecType::Array: {
      const auto& arr_spec = spec_cast<ArraySpec>(spec);
      json["type"] = "array";
      json["item"] = spec_to_json(arr_spec.item());
      break;
    }

    case SpecType::Record: {
      const auto& rec = spec_cast<RecordSpec>(spec);
      json["type"] = "record";
      auto fields = JSON::array();
      for (const auto& field : rec.fields()) {
        fields.push_back({
            {"id", field.id},
            {"name", field.name},
            {"spec", spec_to_json(*field.spec)},
        });
      }
      json["fields"] = std::move(fields);
      break;
    }

    case SpecType::Variant: {
      const auto& var = spec_cast<VariantSpec>(spec);
      json["type"] = "variant";
      auto options = JSON::array();
      for (const auto& opt : var.options()) {
        options.push_back({
            {"id", opt.id},
            {"name", opt.name},
            {"spec", spec_to_json(*opt.spec)},
        });
      }
      json["options"] = std::move(options);
      set_optional(json, "default", var.default_value());
      break;
    }

    default: std::unreachable();
  }

  return json;
}

} // namespace

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto spec_dump_json(const Spec& spec) -> std::string {
  return spec_to_json(spec).dump(2);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::prop
