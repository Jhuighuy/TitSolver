/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/// @todo Tests are missing.
#include <algorithm>
#include <array>
#include <cmath>
#include <format>
#include <iterator>
#include <string>
#include <string_view>

#include "tit/core/basic_types.hpp"
#include "tit/core/str.hpp"

namespace tit {

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

  constexpr long double base = 1024.0;
  const auto val_float = static_cast<long double>(value);
  const auto exp_float = std::floor(std::log2(val_float) / std::log2(base));
  const auto exp_index = static_cast<size_t>(exp_float);
  const auto scaled = val_float / std::pow(base, exp_float);
  const auto index = std::min(exp_index, prefixes.size() - 1);

  return std::format("{:.{}f} {}", scaled, precision, prefixes[index]);
}

auto fmt_quantity(long double value, std::string_view unit, size_t precision)
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

  constexpr const auto center = std::ssize(prefixes) / 2;
  constexpr long double base = 1000.0;
  const auto exp_float =
      std::floor(std::log10(std::abs(value)) / std::log10(base));
  const auto exp_index = center + static_cast<ssize_t>(exp_float);
  const auto scaled = value / std::pow(base, exp_float);
  const auto index = std::clamp(exp_index, 0Z, std::ssize(prefixes) - 1);

  return std::format("{:.{}f} {}{}", scaled, precision, prefixes[index], unit);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
