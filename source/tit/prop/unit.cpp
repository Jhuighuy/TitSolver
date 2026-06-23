/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "tit/prop/unit.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "tit/core/exception.hpp"
#include "tit/core/float.hpp"
#include "tit/core/str.hpp"

namespace tit::prop {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto parse_real_with_optional_unit(std::string_view text)
    -> std::pair<float64_t, std::optional<std::string_view>> {
  const auto first = text.find_first_not_of(" \t\n\r");
  TIT_ENSURE(first != std::string_view::npos,
             "Expected real value, got empty string.");
  text.remove_prefix(first);

  const auto value_end = text.find_first_of(" \t\n\r");
  const auto value_text = text.substr(0, value_end);
  const auto value = str_to<float64_t>(value_text);
  TIT_ENSURE(value.has_value(),
             "Expected real value at the beginning of '{}'.",
             text);
  if (value_end == std::string_view::npos) return {*value, std::nullopt};
  text.remove_prefix(value_end);

  const auto unit_start = text.find_first_not_of(" \t\n\r");
  if (unit_start == std::string_view::npos) return {*value, std::nullopt};
  text.remove_prefix(unit_start);

  const auto unit_end = text.find_first_of(" \t\n\r");
  const auto unit_text = text.substr(0, unit_end);
  TIT_ENSURE(unit_end == std::string_view::npos ||
                 text.find_first_not_of(" \t\n\r", unit_end) ==
                     std::string_view::npos,
             "Unexpected text after unit '{}'.",
             unit_text);

  return {*value, unit_text};
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace

Unit::Unit(std::string symbol) : symbol_{std::move(symbol)} {
  TIT_ENSURE(!symbol_.empty(), "Unit symbol must not be empty.");
}

auto Unit::symbol() const noexcept -> std::string_view {
  return symbol_;
}

auto Unit::convert(std::string_view value) const -> float64_t {
  const auto [parsed_value, explicit_unit] =
      parse_real_with_optional_unit(value);
  if (explicit_unit.has_value()) {
    TIT_ENSURE(*explicit_unit == symbol_,
               "unit conversion is not implemented yet: expected '{}', got "
               "'{}'.",
               symbol_,
               *explicit_unit);
  }
  return parsed_value;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::prop
