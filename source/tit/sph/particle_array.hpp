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

#include "tit/core/type_traits.hpp"
#include "tit/sph/field.hpp"

namespace tit::sph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Particle view.
template<class ParticleArray>
class ParticleView final {
public:

  /// Particle array type.
  using Array = std::remove_cvref_t<ParticleArray>;

  /// Particle space.
  static constexpr space auto space = Array::space;

  /// Subset of particle fields that are array-wise constants.
  static constexpr field_set auto uniform_fields = Array::uniform_fields;

  /// Subset of particle fields that are individual for each particle.
  static constexpr field_set auto varying_fields = Array::varying_fields;

  /// Set of particle fields that are present.
  static constexpr field_set auto fields = Array::fields;

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

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Particle field value.
  template<field Field>
  constexpr auto operator[](Field field) const noexcept -> decltype(auto) {
    static_assert(fields.contains(Field{}));
    return array()[index(), field];
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  ParticleArray* array_;
  size_t index_;

}; // class ParticleView

/// Particle view type.
///
/// @tparam fields Fields that the view should contain.
template<class PV, auto... fields>
concept particle_view =
    specialization_of<std::remove_cvref_t<PV>, ParticleView> &&
    ((field_set<decltype(meta::Set{fields})> &&
      std::remove_cvref_t<PV>::fields.includes(meta::Set{fields})) &&
     ...);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Particle array.
template<space Space, field_set Uniforms, field_set Varyings>
class ParticleArray final {
public:

  /// Particle space.
  static constexpr Space space{};

  /// Subset of particle fields that are array-wise constants.
  static constexpr Uniforms uniform_fields{};

  /// Subset of particle fields that are individual for each particle.
  static constexpr Varyings varying_fields{};

  /// Set of particle fields that are present.
  static constexpr field_set auto fields = uniform_fields | varying_fields;

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  constexpr explicit ParticleArray(Space /*space*/,
                                   auto /*varyings*/,
                                   auto /*uniforms*/) noexcept {}

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
  constexpr auto all(this auto& self) noexcept {
    return std::views::iota(size_t{0}, self.size()) |
           std::views::transform([&self](size_t index) { return self[index]; });
  }

  /// Particle at index.
  constexpr auto operator[](this auto& self, size_t index) noexcept {
    TIT_ASSERT(index < self.size(), "Particle index is out of range.");
    return ParticleView{self, index};
  }

  /// Particle field at index.
  template<field Field>
  constexpr auto operator[](this auto& self,
                            [[maybe_unused]] size_t index,
                            Field /*field*/) noexcept -> decltype(auto) {
    static_assert(fields.contains(Field{}));
    TIT_ASSERT(index < self.size(), "Particle index is out of range.");
    if constexpr (uniform_fields.contains(Field{})) {
      return std::get<uniform_fields.find(Field{})>(self.uniform_data_);
    } else if constexpr (varying_fields.contains(Field{})) {
      return std::get<varying_fields.find(Field{})>(self.varying_data_)[index];
    } else static_assert(false);
  }

  /// Values for the specified field.
  template<field Field>
  constexpr auto operator[](this auto& self,
                            Field /*field*/) noexcept -> decltype(auto) {
    static_assert(fields.contains(Field{}));
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
    return std::tuple<field_value_t<decltype(consts), Space>...>{};
  })) uniform_data_;

  decltype(varying_fields.apply([](auto... vars) {
    return std::tuple<std::vector<field_value_t<decltype(vars), Space>>...>{};
  })) varying_data_;

}; // class ParticleArray

template<class Space, class AllVariables, class Uniforms>
ParticleArray(Space, AllVariables, Uniforms)
    -> ParticleArray<Space, Uniforms, decltype(AllVariables{} - Uniforms{})>;

/// Particle array type.
///
/// @tparam fields Fields that the array should contain.
template<class PA, auto... fields>
concept particle_array =
    specialization_of<std::remove_cvref_t<PA>, ParticleArray> &&
    ((field_set<decltype(meta::Set{fields})> &&
      std::remove_cvref_t<PA>::fields.includes(meta::Set{fields})) &&
     ...);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Check particle fields presence.
template<class P>
  requires particle_array<P> || particle_view<P>
consteval auto has(field auto... fields) -> bool {
  constexpr auto present_fields = std::remove_cvref_t<P>::fields;
  return present_fields.includes(meta::Set{fields...});
}

/// Check particle uniform fields presence.
template<class P>
  requires particle_array<P> || particle_view<P>
consteval auto has_uniform(field auto... uniforms) -> bool {
  constexpr auto present_fields = std::remove_cvref_t<P>::uniform_fields;
  return present_fields.includes(meta::Set{uniforms...});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph

// a
