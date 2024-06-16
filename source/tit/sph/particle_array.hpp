/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <format>
#include <fstream>
#include <ranges>
#include <span>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/mat.hpp"
#include "tit/core/meta.hpp"
#include "tit/core/vec.hpp"

#include "tit/sph/field.hpp"

namespace tit {

template<class PV, auto... Fields>
concept particle_view = has<PV>(Fields...);

template<class PA, auto... Fields>
concept particle_array = has<PA>(Fields...);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Particle view.
template<class ParticleArray>
  requires std::is_object_v<ParticleArray>
class ParticleView final {
public:

  /// Set of particle fields that are present.
  static constexpr auto fields = std::remove_const_t<ParticleArray>::fields;

  /// Subset of particle fields that are array-wise constants.
  static constexpr auto constants =
      std::remove_const_t<ParticleArray>::constants;

  /// Subset of particle fields that are individual for each particle.
  static constexpr auto variables =
      std::remove_const_t<ParticleArray>::variables;

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Construct a particle view.
  constexpr ParticleView(ParticleArray& array, size_t index) noexcept
      : array_{&array}, index_{index} {}

  /// Associated particle array.
  constexpr auto array() const noexcept -> ParticleArray& {
    TIT_ASSERT(array_ != nullptr, "Particle array was not set.");
    return *array_;
  }

  /// Associated particle index.
  constexpr auto index() const noexcept -> size_t {
    return index_;
  }

  /// Compare particle views.
  constexpr auto operator==(ParticleView<ParticleArray> other) const noexcept
      -> bool {
    TIT_ASSERT(&array() == &other.array(),
               "Cannot compare particle views of different arrays!");
    return index() == other.index();
  }

  /// Particle field value.
  template<meta::type Field>
    requires particle_view<ParticleView, Field{}>
  constexpr auto operator[](Field field) const noexcept -> decltype(auto) {
    return array()[index(), field];
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  ParticleArray* array_;
  size_t index_;

}; // class ParticleView

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Particle array.
template<meta::type Space, meta::type FieldSet, meta::type ConstSet>
class ParticleArray final {
public:

  /// Set of particle fields that are present.
  static constexpr auto fields = FieldSet{};

  /// Subset of particle fields that are array-wise constants.
  static constexpr auto constants = ConstSet{};

  /// Subset of particle fields that are individual for each particle.
  static constexpr auto variables = fields - constants;

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Construct a particle array.
  /// @{
  template<meta::type FieldSubset, meta::type ConstSubset>
    requires (meta::is_set_v<FieldSubset> && fields.includes(FieldSubset{})) &&
             (meta::is_set_v<ConstSubset> && constants.includes(ConstSubset{}))
  constexpr explicit ParticleArray(Space /*space*/,
                                   FieldSubset /*field_subset*/ = {},
                                   ConstSubset /*const_subset*/ = {}) {}
  template<meta::type FieldSubset, meta::type... ConstSubset>
    requires (meta::is_set_v<FieldSubset> && fields.includes(FieldSubset{})) &&
             (constants.includes(meta::Set<ConstSubset...>{}))
  constexpr explicit ParticleArray(Space /*space*/,
                                   FieldSubset /*field_subset*/ = {},
                                   ConstSubset... /*consts*/) {}
  /// @}

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Number of particles.
  constexpr auto size() const noexcept -> size_t {
    return std::get<0>(variables_).size();
  }

  /// Reserve amount of particles.
  constexpr void reserve(size_t capacity) {
    std::apply([capacity](auto&... vecs) { ((vecs.reserve(capacity)), ...); },
               variables_);
  }

  /// Appends a new particle.
  constexpr auto append() {
    std::apply([](auto&... vecs) { ((vecs.emplace_back()), ...); }, variables_);
    return (*this)[size() - 1];
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Particles.
  constexpr auto views(this auto& self) noexcept {
    return std::views::iota(size_t{0}, self.size()) |
           std::views::transform([&self](size_t index) { return self[index]; });
  }

  /// Particle at index.
  constexpr auto operator[](this auto& self, size_t index) noexcept {
    TIT_ASSERT(index < self.size(), "Particle index is out of range.");
    return ParticleView{self, index};
  }

  /// Particle field at index.
  template<meta::type Field>
    requires (fields.contains(Field{}))
  constexpr auto operator[](this auto& self,
                            [[maybe_unused]] size_t index,
                            Field /*field*/) noexcept -> decltype(auto) {
    TIT_ASSERT(index < self.size(), "Particle index is out of range.");
    if constexpr (constants.contains(Field{})) {
      return std::get<constants.find(Field{})>(self.constants_);
    } else if constexpr (variables.contains(Field{})) {
      return std::get<variables.find(Field{})>(self.variables_)[index];
    } else static_assert(false);
  }

  /// Values for the specified field.
  template<meta::type Field>
    requires (fields.contains(Field{}))
  constexpr auto operator[](this auto& self,
                            Field /*field*/) noexcept -> decltype(auto) {
    if constexpr (constants.contains(Field{})) {
      return std::get<constants.find(Field{})>(self.constants_);
    } else if constexpr (variables.contains(Field{})) {
      return std::span{std::get<variables.find(Field{})>(self.variables_)};
    } else static_assert(false);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  using Constants_ = decltype([]<class... Consts>(meta::Set<Consts...>) {
    return std::tuple<field_value_type_t<Consts, Space>...>{};
  }(constants));

  using Variables_ = decltype([]<class... Vars>(meta::Set<Vars...>) {
    return std::tuple<std::vector<field_value_type_t<Vars, Space>>...>{};
  }(variables));

  Constants_ constants_;
  Variables_ variables_;

}; // class ParticleArray

// This template deduction guides ensure that constants are always included
// into a set of fields.
template<class Space, class Fields, class Consts>
ParticleArray(Space, Fields, Consts)
    -> ParticleArray<Space, decltype(Fields{} | Consts{}), Consts>;

template<class Space, class Fields, class... Consts>
  requires meta::is_set_v<Fields>
ParticleArray(Space, Fields, Consts...)
    -> ParticleArray<Space,
                     decltype(Fields{} | meta::Set<Consts...>{}),
                     meta::Set<Consts...>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace impl {

template<class Num>
constexpr auto format_field_name(const std::string& prefix,
                                 meta::ID<Num> /*scal*/) -> const std::string& {
  return prefix;
}

template<class Num, size_t Dim>
constexpr auto format_field_name(const std::string& prefix,
                                 meta::ID<Vec<Num, Dim>> /*vec*/)
    -> std::string {
  if constexpr (Dim == 1) return prefix;
  else if constexpr (Dim == 2) return std::format("{0}_x {0}_y", prefix);
  else if constexpr (Dim == 3) return std::format("{0}_x {0}_y {0}_z", prefix);
  else static_assert(false);
}

template<class Num, size_t Dim>
constexpr auto format_field_name(const std::string& prefix,
                                 meta::ID<Mat<Num, Dim>> /*mat*/)
    -> std::string {
  if constexpr (Dim == 1) return prefix;
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
                           meta::ID<field_value_type_t<Field, Space>>{});
}

} // namespace impl

/// A blueprint for printing particle arrays in CSV-like format.
template<meta::type Space, meta::type... Fields, meta::type ConstSet>
void print(const ParticleArray<Space, meta::Set<Fields...>, ConstSet>& array,
           const std::string& path) {
  std::ofstream output{};
  output.open(path);
  ((output << impl::make_field_name(Space{}, Fields{}) << " "), ...);
  output << '\n';
  for (const auto a : array.views()) {
    ((output << Fields{}[a] << " "), ...);
    output << '\n';
  };
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
