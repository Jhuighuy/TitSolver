/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <format>
#include <fstream>
#include <print>
#include <string>
#include <string_view>

#include "tit/core/basic_types.hpp"
#include "tit/core/mat.hpp"
#include "tit/core/meta/type.hpp"
#include "tit/core/vec.hpp"

#include "tit/sph/field.hpp"

namespace tit::sph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace impl {
template<class Num>
constexpr auto format_field_name(std::string_view prefix,
                                 meta::ID<Num> /*scal*/) -> std::string {
  return std::string{prefix};
}

template<class Num, size_t Dim>
constexpr auto format_field_name(std::string_view prefix,
                                 meta::ID<Vec<Num, Dim>> /*vec*/)
    -> std::string {
  if constexpr (Dim == 1) return std::string{prefix};
  else if constexpr (Dim == 2) return std::format("{0}_x {0}_y", prefix);
  else if constexpr (Dim == 3) return std::format("{0}_x {0}_y {0}_z", prefix);
  else static_assert(false);
}

template<class Num, size_t Dim>
constexpr auto format_field_name(std::string_view prefix,
                                 meta::ID<Mat<Num, Dim>> /*mat*/)
    -> std::string {
  if constexpr (Dim == 1) return std::string{prefix};
  else if constexpr (Dim == 2) {
    return std::format("{0}_xx {0}_xy "
                       "{0}_yx {0}_yy",
                       prefix);
  } else if constexpr (Dim == 3) {
    return std::format("{0}_xx {0}_xy {0}_xz "
                       "{0}_yx {0}_yy {0}_yz "
                       "{0}_zx {0}_zy {0}_zz",
                       prefix);
  } else static_assert(false);
}

template<class Space, class Field>
constexpr auto make_field_name(Space /*space*/, Field /*field*/) {
  return format_field_name(field_name_v<Field>,
                           meta::ID<field_value_t<Field, Space>>{});
}

} // namespace impl

/// A blueprint for printing particle arrays in CSV-like format.
template<class ParticleArray>
void print_csv(const ParticleArray& array, const std::string& path) {
  std::ofstream output(path);
  // Print field names.
  ParticleArray::fields.for_each([&output](auto field) {
    output << impl::make_field_name(ParticleArray::space, field) << " ";
  });
  output << '\n';
  // Print field columns.
  for (const auto a : array.all()) {
    ParticleArray::fields.for_each(
        [&output, a](auto field) { std::print(output, "{} ", a[field]); });
    output << '\n';
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
