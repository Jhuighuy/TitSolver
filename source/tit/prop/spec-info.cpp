/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <ranges>
#include <string>
#include <utility>
#include <vector>

#include "tit/core/str.hpp"
#include "tit/prop/spec.hpp"

namespace tit::prop {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// SpecInfo
//

auto spec_info(const Spec& spec) -> SpecInfo {
  SpecInfo info;

  using enum SpecType;
  switch (spec.type()) {
    case Bool: {
      const auto& bool_spec = spec_cast<BoolSpec>(spec);
      if (bool_spec.default_value().has_value()) {
        info.default_value = *bool_spec.default_value() ? "true" : "false";
      }
      break;
    }

    case Int: {
      const auto& int_spec = spec_cast<IntSpec>(spec);
      if (int_spec.default_value().has_value()) {
        info.default_value = std::to_string(int_spec.default_value().value());
      }
      if (int_spec.min().has_value()) {
        info.min_value = std::to_string(int_spec.min().value());
      }
      if (int_spec.max().has_value()) {
        info.max_value = std::to_string(int_spec.max().value());
      }
      break;
    }

    case Real: {
      const auto& real_spec = spec_cast<RealSpec>(spec);
      if (real_spec.default_value().has_value()) {
        info.default_value = fmt_real(real_spec.default_value().value());
      }
      if (real_spec.min().has_value()) {
        info.min_value = fmt_real(real_spec.min().value());
      }
      if (real_spec.max().has_value()) {
        info.max_value = fmt_real(real_spec.max().value());
      }
      break;
    }

    case String: {
      const auto& str_spec = spec_cast<StringSpec>(spec);
      if (str_spec.default_value().has_value()) {
        info.default_value = str_quoted(str_spec.default_value().value());
      }
      break;
    }

    case Enum: {
      const auto& enum_spec = spec_cast<EnumSpec>(spec);
      if (enum_spec.default_value().has_value()) {
        info.default_value = str_quoted(enum_spec.default_value().value());
      }
      info.options.assign_range(enum_spec.options() |
                                std::views::transform(&EnumSpec::Option::id) |
                                std::views::transform(str_quoted));
      break;
    }

    case Variant: {
      const auto& variant_spec = spec_cast<VariantSpec>(spec);
      if (variant_spec.default_value().has_value()) {
        info.default_value = str_quoted(variant_spec.default_value().value());
      }
      info.options.assign_range(
          variant_spec.options() |
          std::views::transform(&VariantSpec::Option::id) |
          std::views::transform(str_quoted));
      break;
    }

    case App:
    case Array:
    case Record: {
      // Compound specifications do not have presentation-oriented metadata.
      break;
    }

    default: std::unreachable();
  }

  return info;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::prop
