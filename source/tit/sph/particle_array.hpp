/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <ranges>
#include <span>
#include <tuple>
#include <type_traits>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/meta.hpp"

#include "tit/sph/field.hpp"

namespace tit {

template<class PA, auto... Fields>
concept particle_array = has<PA>(Fields...);

template<class PV, auto... Fields>
concept particle_view = has<PV>(Fields...);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Particle view.
template<class ParticleArray>
class ParticleView final {
public:

  /// Particle space.
  static constexpr auto space = std::remove_cvref_t<ParticleArray>::space;

  /// Subset of particle fields that are array-wise constants.
  static constexpr auto uniform_fields =
      std::remove_const_t<ParticleArray>::uniform_fields;

  /// Subset of particle fields that are individual for each particle.
  static constexpr auto varying_fields =
      std::remove_const_t<ParticleArray>::varying_fields;

  /// Set of particle fields that are present.
  static constexpr auto fields = std::remove_const_t<ParticleArray>::fields;

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
template<meta::type Space, meta::Set Uniforms, meta::Set Varyings>
class ParticleArray final {
public:

  /// Particle space.
  static constexpr auto space = Space{};

  /// Subset of particle fields that are array-wise constants.
  static constexpr auto uniform_fields = Uniforms;

  /// Subset of particle fields that are individual for each particle.
  static constexpr auto varying_fields = Varyings;

  /// Set of particle fields that are present.
  static constexpr auto fields = Uniforms | Varyings;

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Construct a particle array.
  ///
  /// @param space The space in which the particles are defined.
  /// @param equations The equations that define the particle fields.
  template<class Equations>
  constexpr explicit ParticleArray(Space /*space*/,
                                   Equations /*equations*/) noexcept {}

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Number of particles.
  constexpr auto size() const noexcept -> size_t {
    return std::get<0>(varying_data_).size();
  }

  /// Reserve amount of particles.
  constexpr void reserve(size_t capacity) {
    std::apply([capacity](auto&... cols) { ((cols.reserve(capacity)), ...); },
               varying_data_);
  }

  /// Appends a new particle.
  constexpr auto append() {
    std::apply([](auto&... cols) { ((cols.emplace_back()), ...); },
               varying_data_);
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
    if constexpr (uniform_fields.contains(Field{})) {
      return std::get<uniform_fields.find(Field{})>(self.uniform_data_);
    } else if constexpr (varying_fields.contains(Field{})) {
      return std::get<varying_fields.find(Field{})>(self.varying_data_)[index];
    } else static_assert(false);
  }

  /// Values for the specified field.
  template<meta::type Field>
    requires (fields.contains(Field{}))
  constexpr auto operator[](this auto& self,
                            Field /*field*/) noexcept -> decltype(auto) {
    if constexpr (uniform_fields.contains(Field{})) {
      return std::get<uniform_fields.find(Field{})>(self.uniform_data_);
    } else if constexpr (varying_fields.contains(Field{})) {
      return std::span{
          std::get<varying_fields.find(Field{})>(self.varying_data_)};
    } else static_assert(false);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  decltype(uniform_fields.apply([](auto... consts) {
    return std::tuple<field_value_type_t<decltype(consts), Space>...>{};
  })) uniform_data_;

  decltype(varying_fields.apply([](auto... vars) {
    return std::tuple<
        std::vector<field_value_type_t<decltype(vars), Space>>...>{};
  })) varying_data_;

}; // class ParticleArray

template<class Space, class Equations>
ParticleArray(Space, Equations)
    -> ParticleArray<Space,
                     Equations::required_fields - Equations::modified_fields,
                     Equations::required_fields & Equations::modified_fields>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
