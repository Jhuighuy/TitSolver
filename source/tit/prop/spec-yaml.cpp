/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <format>
#include <ostream>
#include <print>
#include <string>
#include <string_view>
#include <utility>

#include "tit/core/exception.hpp"
#include "tit/core/print.hpp"
#include "tit/core/str.hpp"
#include "tit/prop/spec.hpp"

namespace tit::prop {

namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto yaml_type_label(const Spec& spec) -> std::string {
  auto out = std::string{spec.type() == SpecType::Variant ?
                             "variant" :
                             spec_type_to_string(spec.type())};
  const auto info = spec_info(spec);
  if (info.min_value.has_value())
    out += std::format(", min={}", *info.min_value);
  if (info.max_value.has_value())
    out += std::format(", max={}", *info.max_value);
  if (info.default_value.has_value()) {
    out += std::format(", default={}", *info.default_value);
  }
  return out;
}

auto scalar_value(const Spec& spec) -> std::string {
  using enum SpecType;
  switch (spec.type()) {
    case Bool: {
      const auto& s = spec_cast<BoolSpec>(spec);
      if (!s.default_value().has_value()) return "null";
      return *s.default_value() ? "true" : "false";
    }
    case Int: {
      const auto& s = spec_cast<IntSpec>(spec);
      return s.default_value().has_value() ?
                 std::format("{}", *s.default_value()) :
                 "null";
    }
    case Real: {
      const auto& s = spec_cast<RealSpec>(spec);
      return s.default_value().has_value() ? fmt_real(*s.default_value()) :
                                             "null";
    }
    case String: {
      const auto& s = spec_cast<StringSpec>(spec);
      return s.default_value().has_value() ? str_quoted(*s.default_value()) :
                                             "null";
    }
    case Enum: {
      const auto& s = spec_cast<EnumSpec>(spec);
      return s.default_value().has_value() ? str_quoted(*s.default_value()) :
                                             "null";
    }
    case Array:
    case Record:
    case Variant:
    case App:     TIT_THROW("Scalar value requested for non-scalar spec.");
  }
  std::unreachable();
}

auto variant_selector_label(const VariantSpec& spec) -> std::string {
  std::string out = "enum";
  if (const auto info = spec_info(spec); info.default_value.has_value()) {
    out += std::format(", default={}", *info.default_value);
  }
  return out;
}

auto is_structured(const Spec& spec) -> bool {
  using enum SpecType;
  return spec.type() == Record || spec.type() == App || spec.type() == Variant;
}

class YamlEmitter final {
public:

  explicit YamlEmitter(std::ostream& out) : out_{&out} {}

  void emit_record(const RecordSpec& spec, int indent = 0) {
    bool first = true;
    for (const auto& field : spec.fields()) {
      if (!first) std::println(*out_);
      first = false;
      emit_comment(field.name, *field.spec, indent);
      emit_field(field.id, *field.spec, indent);
    }
  }

private:

  std::ostream* out_;

  void emit_comment(std::string_view name, const Spec& spec, int indent) {
    println_indented(*out_, indent, "# {}", name);
    println_indented(*out_, indent, "# Type: {}", yaml_type_label(spec));
  }

  void emit_field(std::string_view id, const Spec& spec, int indent) {
    switch (spec.type()) {
      case SpecType::Bool:
      case SpecType::Int:
      case SpecType::Real:
      case SpecType::String:
      case SpecType::Enum:   {
        println_indented(*out_, indent, "{}: {}", id, scalar_value(spec));
        break;
      }
      case SpecType::Record:
      case SpecType::App:    {
        println_indented(*out_, indent, "{}:", id);
        emit_record(spec_cast<RecordSpec>(spec), indent + 2);
        break;
      }
      case SpecType::Array: {
        const auto& item = spec_cast<ArraySpec>(spec).item();
        if (is_structured(item)) {
          println_indented(*out_, indent, "{}:", id);
          println_indented(*out_, indent + 2, "# Example item");
          emit_array_item(item, indent + 2);
        } else {
          println_indented(*out_, indent, "{}: []", id);
          println_indented(*out_,
                           indent,
                           "# Item type: {}",
                           yaml_type_label(item));
        }
        break;
      }
      case SpecType::Variant: {
        emit_variant(id, spec_cast<VariantSpec>(spec), indent);
        break;
      }
    }
  }

  void emit_array_item(const Spec& spec, int indent) {
    switch (spec.type()) {
      case SpecType::Bool:
      case SpecType::Int:
      case SpecType::Real:
      case SpecType::String:
      case SpecType::Enum:   {
        println_indented(*out_, indent, "- {}", scalar_value(spec));
        break;
      }
      case SpecType::Record:
      case SpecType::App:    {
        println_indented(*out_, indent, "-");
        emit_record(spec_cast<RecordSpec>(spec), indent + 2);
        break;
      }
      case SpecType::Variant: {
        println_indented(*out_, indent, "-");
        emit_variant_body(spec_cast<VariantSpec>(spec), indent + 2);
        break;
      }
      case SpecType::Array: {
        println_indented(*out_, indent, "- []");
        break;
      }
    }
  }

  void emit_variant(std::string_view id, const VariantSpec& spec, int indent) {
    println_indented(*out_, indent, "{}:", id);
    emit_variant_body(spec, indent + 2);
  }

  void emit_variant_body(const VariantSpec& spec, int indent) {
    println_indented(*out_,
                     indent,
                     "_active: {}",
                     spec.default_value().has_value() ?
                         str_quoted(*spec.default_value()) :
                         "null");

    println_indented(*out_, indent, "# Type: {}", variant_selector_label(spec));

    for (const auto& option : spec.options()) {
      std::println(*out_);
      emit_comment(option.name, *option.spec, indent);
      switch (option.spec->type()) {
        case SpecType::Record:
        case SpecType::App:    {
          println_indented(*out_, indent, "{}:", option.id);
          emit_record(spec_cast<RecordSpec>(*option.spec), indent + 2);
          break;
        }
        case SpecType::Variant: {
          emit_variant(option.id, spec_cast<VariantSpec>(*option.spec), indent);
          break;
        }
        case SpecType::Array: {
          emit_field(option.id, *option.spec, indent);
          break;
        }
        case SpecType::Bool:
        case SpecType::Int:
        case SpecType::Real:
        case SpecType::String:
        case SpecType::Enum:   {
          println_indented(*out_,
                           indent,
                           "{}: {}",
                           option.id,
                           scalar_value(*option.spec));
          break;
        }
      }
    }
  }

}; // class YamlEmitter

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace

void spec_dump_yaml_config(const RecordSpec& spec, std::ostream& os) {
  YamlEmitter{os}.emit_record(spec);
  std::println(os);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::prop
