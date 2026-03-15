/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <format>
#include <iomanip>
#include <iterator>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/str.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto is_alpha(char c) -> bool {
  return std::isalpha(static_cast<unsigned char>(c)) != 0;
}

auto is_alnum(char c) -> bool {
  return std::isalnum(static_cast<unsigned char>(c)) != 0;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace

auto str_is_identifier(std::string_view str) -> bool {
  return !str.empty() && (is_alpha(str.front()) || str.front() == '_') &&
         std::ranges::all_of(str.substr(1),
                             [](char c) { return is_alnum(c) || c == '_'; });
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto str_split(std::string_view str, char delim)
    -> std::vector<std::string_view> {
  std::vector<std::string_view> result;
  while (true) {
    const auto next = str.find(delim);
    result.push_back(str.substr(
        0,
        next == std::string_view::npos ? std::string_view::npos : next));
    if (next == std::string_view::npos) break;
    str.remove_prefix(next + 1);
  }
  return result;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto str_quoted(std::string_view str) -> std::string {
  return (std::ostringstream{} << std::quoted(std::string{str})).str();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto fmt_real(float64_t value) -> std::string {
  auto str = std::format("{}", value);
  if (!str.contains('.') && !str.contains('e')) str += ".0";
  return str;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto fmt_memsize(uint64_t value, size_t precision) -> std::string {
  static constexpr const auto prefixes = std::to_array<std::string_view>({
      "bytes", // 1024^0
      "KiB",   // 1024^1, kibi
      "MiB",   // 1024^2, mebi
      "GiB",   // 1024^3, gibi
      "TiB",   // 1024^4, tebi
      "PiB",   // 1024^5, pebi
      "EiB",   // 1024^6, exbi
      "ZiB",   // 1024^7, zebi
      "YiB",   // 1024^8, yobi
  });

  if (value == 0) return "0 bytes";

  constexpr float64_t base = 1024.0;
  const auto val_float = static_cast<float64_t>(value);
  const auto exponent = std::floor(std::log2(val_float) / std::log2(base));
  const auto index =
      std::min(static_cast<size_t>(exponent), prefixes.size() - 1);
  const auto scaled = val_float / std::pow(base, index);

  return std::format("{:.{}f} {}", scaled, precision, prefixes[index]);
}

auto fmt_quantity(float64_t value, std::string_view unit, size_t precision)
    -> std::string {
  constexpr const auto prefixes = std::to_array<std::string_view>({
      "y", // 10^-24, yocto
      "z", // 10^-21, zepto
      "a", // 10^-18, atto
      "f", // 10^-15, femto
      "p", // 10^-12, pico
      "n", // 10^-9,  nano
      "μ", // 10^-6,  micro
      "m", // 10^-3,  milli
      "",  // 10^ 0
      "k", // 10^+3,  kilo
      "M", // 10^+6,  mega
      "G", // 10^+9,  giga
      "T", // 10^+12, tera
      "P", // 10^+15, peta
      "E", // 10^+18, exa
      "Z", // 10^+21, zetta
      "Y", // 10^+24, yotta
  });

  if (value == 0.0) return std::format("0 {}", unit);

  constexpr float64_t base = 1000.0;
  const auto exponent =
      std::floor(std::log10(std::abs(value)) / std::log10(base));
  constexpr const auto center = std::ssize(prefixes) / 2;
  auto index = std::clamp(static_cast<ssize_t>(exponent), -center, center);
  const auto scaled = value / std::pow(base, index);
  index += center;

  return std::format("{:.{}f} {}{}", scaled, precision, prefixes[index], unit);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
