/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <array>
#include <concepts>
#include <ranges>
#include <span>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/type.hpp"
#include "tit/core/vec.hpp"
#include "tit/data/storage.hpp"
#include "tit/sph/field.hpp"

namespace tit::sph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Particle type.
enum class ParticleType : uint8_t {
  fluid, ///< Fluid particle.
  fixed, ///< Fixed (boundary) particle.
  count, ///< Number of particle types.
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Standalone particle.
template<space Space, field_set Fields>
class Particle final {
public:

  /// Particle space.
  static constexpr Space space{};

  /// Set of particle fields that are present.
  static constexpr Fields fields{};

  /// Construct a particle.
  constexpr explicit Particle(Space /*space*/, Fields /*fields*/) noexcept {}

  /// Check if the particle has the specified type.
  constexpr auto has_type(ParticleType type) const noexcept -> bool {
    // For now, we'll assume that all standalone particles are fluid.
    return type == ParticleType::fluid;
  }

  /// Check if the particle is fluid.
  constexpr auto is_fluid() const noexcept -> bool {
    return has_type(ParticleType::fluid);
  }

  /// Check if the particle is fixed.
  constexpr auto is_fixed() const noexcept -> bool {
    return has_type(ParticleType::fixed);
  }

  /// Particle field value.
  /// @{
  template<field Field>
  constexpr auto operator[](Field /*field*/) noexcept -> auto& {
    static_assert(fields.contains(Field{}));
    return std::get<fields.find(Field{})>(data_);
  }
  template<field Field>
  constexpr auto operator[](Field /*field*/) const noexcept -> const auto& {
    static_assert(fields.contains(Field{}));
    return std::get<fields.find(Field{})>(data_);
  }
  /// @}

private:

  [[no_unique_address]] decltype([]<class... Fs>(TypeSet<Fs...> /*fs*/) {
    return std::tuple<field_value_t<Fs, decltype(auto{space})>...>{};
  }(fields)) data_;

}; // class Particle

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Particle view.
template<class ParticleArray>
class ParticleView final {
public:

  /// Particle array type.
  using Array = std::remove_const_t<ParticleArray>;

  /// Particle space.
  static constexpr space auto space = Array::space;

  /// Set of particle fields that are present.
  static constexpr field_set auto fields = Array::fields;

  /// Construct a particle view.
  constexpr ParticleView(ParticleArray& array, size_t index) noexcept
      : array_{&array}, index_{index} {}

  /// Associated particle array.
  constexpr auto array() const noexcept -> ParticleArray& {
    return *array_;
  }

  /// Associated particle index.
  constexpr auto index() const noexcept -> size_t {
    return index_;
  }

  /// Check if the particle has the specified type.
  constexpr auto has_type(ParticleType type) const noexcept -> bool {
    return array().has_type(index(), type);
  }

  /// Check if the particle is fluid.
  constexpr auto is_fluid() const noexcept -> bool {
    return has_type(ParticleType::fluid);
  }

  /// Check if the particle is fixed.
  constexpr auto is_fixed() const noexcept -> bool {
    return has_type(ParticleType::fixed);
  }

  /// Particle field value.
  template<field Field>
  constexpr auto operator[](Field field) const noexcept -> auto& {
    static_assert(fields.contains(Field{}));
    return array()[field][index()];
  }

private:

  ParticleArray* array_;
  size_t index_;

}; // class ParticleView

/// Particle array.
template<space Space, field_set Fields>
class ParticleArray final {
public:

  /// Particle space.
  static constexpr Space space{};

  /// Set of particle fields that are present.
  static constexpr Fields fields{};

  /// Construct a particle array.
  constexpr explicit ParticleArray(Space /*space*/,
                                   Fields /*fields*/) noexcept {}

  /// Appends a new particle of the specified type @p type.
  constexpr auto append(ParticleType type) -> ParticleView<ParticleArray> {
    TIT_ASSERT(type < ParticleType::count, "Invalid particle type.");
    const auto type_index = std::to_underlying(type);

    // Get the index of the next particle of the specified type and increment
    // the range of particles for the next types.
    const size_t index = particle_ranges_[type_index + 1];
    for (auto& p : particle_ranges_ | std::views::drop(type_index + 1)) p += 1;

    // Insert the new particle.
    std::apply(
        [index](auto&... cols) { ((cols.emplace(cols.begin() + index)), ...); },
        data_);
    return (*this)[index];
  }

  /// Number of particles.
  constexpr auto size() const noexcept -> size_t {
    return std::get<0>(data_).size();
  }

  /// All particles.
  constexpr auto all(this auto& self) noexcept {
    return std::views::iota(size_t{0}, self.size()) |
           std::views::transform([&self](size_t index) { return self[index]; });
  }

  /// Particles of the specified type.
  constexpr auto typed(this auto& self, ParticleType type) noexcept {
    TIT_ASSERT(type < ParticleType::count, "Invalid particle type.");
    const auto type_index = std::to_underlying(type);
    return std::views::iota(self.particle_ranges_[type_index],
                            self.particle_ranges_[type_index + 1]) |
           std::views::transform([&self](size_t index) { return self[index]; });
  }

  /// Fluid particles.
  constexpr auto fluid(this auto& self) noexcept {
    return self.typed(ParticleType::fluid);
  }

  /// Fixed particles.
  constexpr auto fixed(this auto& self) noexcept {
    return self.typed(ParticleType::fixed);
  }

  /// Check if the particle has the specified type.
  constexpr auto has_type(size_t index, ParticleType type) const noexcept
      -> bool {
    TIT_ASSERT(type < ParticleType::count, "Invalid particle type.");
    const auto type_index = std::to_underlying(type);
    return particle_ranges_[type_index] <= index &&
           index < particle_ranges_[type_index + 1];
  }

  /// Particle at index.
  constexpr auto operator[](this auto& self, size_t index) noexcept {
    TIT_ASSERT(index < self.size(), "Particle index is out of range.");
    return ParticleView{self, index};
  }

  /// Values for the specified field.
  template<field Field>
  constexpr auto operator[](this auto& self, Field /*field*/) noexcept
      -> decltype(auto) {
    static_assert(fields.contains(Field{}));
    return std::span{std::get<fields.find(Field{})>(self.data_)};
  }

  /// Write a particle array into a data series.
  void write(field_value_t<rho_t, Space> time,
             data::SeriesView<data::Storage> series) const {
    auto frame = series.create_frame(static_cast<float64_t>(time));
    ParticleArray::fields.for_each([&frame, this](auto field) {
      const auto array = frame.create_array(field.field_name);
      array.write((*this)[field]);
    });
  }

private:

  std::array<size_t, std::to_underlying(ParticleType::count) + 1>
      particle_ranges_{0};

  [[no_unique_address]] decltype([]<class... Fs>(TypeSet<Fs...> /*fs*/) {
    return std::tuple<std::vector<field_value_t<Fs, Space>>...>{};
  }(fields)) data_;

}; // class ParticleArray

/// Particle view type.
template<class PV, auto... fields>
concept particle_view =
    (specialization_of<std::remove_cvref_t<PV>, Particle> ||
     specialization_of<std::remove_cvref_t<PV>, ParticleView>) &&
    field_set<decltype(TypeSet{fields...})> &&
    (TypeSet{fields...} <= std::remove_cvref_t<PV>::fields);

/// Particle array type.
template<class PA, auto... fields>
concept particle_array =
    specialization_of<std::remove_cvref_t<PA>, ParticleArray> &&
    field_set<decltype(TypeSet{fields...})> &&
    (TypeSet{fields...} <= std::remove_cvref_t<PA>::fields);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace impl {

template<auto field, class P>
struct particle_field_reference;

template<auto field, particle_view<field> PV>
struct particle_field_reference<field, PV> {
  using type = decltype(std::declval<PV>()[field]);
};

template<auto field, particle_array<r> PA>
struct particle_field_reference<field, PA> {
  using type = decltype(std::declval<PA&>()[field].front());
};

} // namespace impl

/// Particle field reference type.
template<auto field, class P>
  requires particle_view<P, field> || particle_array<P, field>
using particle_field_reference_t =
    typename impl::particle_field_reference<field, P>::type;

/// Particle field type.
template<auto field, class P>
  requires particle_view<P, field> || particle_array<P, field>
using particle_field_t =
    std::remove_cvref_t<particle_field_reference_t<field, P>>;

/// Particle scalar type.
template<class P>
  requires particle_view<P> || particle_array<P>
using particle_num_t = particle_field_t<rho, P>;

/// Particle vector type.
template<class P>
  requires particle_view<P, r> || particle_array<P, r>
using particle_vec_t = particle_field_t<r, P>;

/// Particle space dimension.
template<class P>
  requires particle_view<P> || particle_array<P>
inline constexpr auto particle_dim_v = vec_dim_v<particle_vec_t<P>>;

/// Particle array type with the specific number type.
template<class PA, class Num, auto... fields>
concept particle_array_n =
    particle_array<PA, fields...> && std::same_as<particle_num_t<PA>, Num>;

/// Particle view type with the specific number type.
template<class PV, class Num, auto... fields>
concept particle_view_n =
    particle_view<PV, fields...> && std::same_as<particle_num_t<PV>, Num>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Check particle fields presence.
template<class P>
  requires particle_view<P> || particle_array<P>
consteval auto has(field auto... fields) -> bool {
  constexpr auto present_fields = std::remove_cvref_t<P>::fields;
  return TypeSet{fields...} <= present_fields;
}

/// Check presence of at least one particle field.
template<class PV, field... Fields>
consteval auto has_any(Fields... fields) -> bool {
  constexpr auto present_fields = std::remove_cvref_t<PV>::fields;
  return (... || present_fields.contains(fields));
}

// Clear the field value.
template<class PV, field... Fields>
constexpr void clear([[maybe_unused]] PV& particle, Fields... fields) {
  TypeSet{fields...}.for_each([&particle](auto field) {
    if constexpr (has<PV>(field)) particle[field] = {};
  });
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
