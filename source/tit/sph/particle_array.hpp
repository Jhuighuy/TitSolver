/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <array>
#include <ranges>
#include <span>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/meta.hpp"
#include "tit/core/type_utils.hpp"
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

/// Particle view.
template<class ParticleArray>
class ParticleView final {
public:

  /// Particle array type.
  using Array = std::remove_const_t<ParticleArray>;

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
  constexpr auto operator[](Field field) const noexcept -> decltype(auto) {
    static_assert(fields.contains(Field{}));
    return array()[index(), field];
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Compare particle views.
  friend constexpr auto operator==(ParticleView a, ParticleView b) noexcept
      -> bool {
    TIT_ASSERT(a.array().size() == b.array().size(),
               "Particle arrays must be of the same size!");
    return a.index() == b.index();
  }

  /// Distance between particle view indices.
  friend constexpr auto operator-(ParticleView a, ParticleView b) noexcept
      -> ssize_t {
    TIT_ASSERT(a.array().size() == b.array().size(),
               "Particle arrays must be of the same size!");
    return a.index() - b.index();
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

  /// Construct a particle array.
  ///
  /// @param space The space in which the particles are defined.
  /// @param equations The equations that define the particle fields.
  template<class Equations>
  constexpr explicit ParticleArray(Space /*space*/,
                                   Equations /*equations*/) noexcept {}

  /// Write a particle array into a data series.
  void write(real_t time,
             data::DataSeriesView<data::DataStorage> series) const {
    auto time_step = series.create_time_step(time);
    auto uniforms = time_step.uniforms();
    ParticleArray::uniform_fields.for_each([&uniforms, this](auto field) {
      uniforms.create_array(field.field_name, std::span{&field[*this], 1});
    });
    auto varyings = time_step.varyings();
    ParticleArray::varying_fields.for_each([&varyings, this](auto field) {
      varyings.create_array(field.field_name, field[*this]);
    });
  }

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
        varying_data_);
    return (*this)[index];
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

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

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

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
  constexpr auto operator[](this auto& self, Field /*field*/) noexcept
      -> decltype(auto) {
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

  std::array<size_t, std::to_underlying(ParticleType::count) + 1>
      particle_ranges_{0};

  decltype([]<class... Fields>(meta::Set<Fields...> /*fields*/) {
    return std::tuple<field_value_t<Fields, Space>...>{};
  }(uniform_fields)) uniform_data_;

  decltype([]<class... Fields>(meta::Set<Fields...> /*fields*/) {
    return std::tuple<std::vector<field_value_t<Fields, Space>>...>{};
  }(varying_fields)) varying_data_;

}; // class ParticleArray

template<class Space, class Equations>
ParticleArray(Space, Equations) -> ParticleArray<
    Space,
    decltype(Equations::required_fields - Equations::modified_fields),
    decltype(Equations::required_fields & Equations::modified_fields)>;

/// Particle array type.
///
/// @tparam fields Fields that the array should contain.
template<class PA, auto... fields>
concept particle_array =
    specialization_of<std::remove_cvref_t<PA>, ParticleArray> &&
    particle_view<ParticleView<PA>, fields...>;

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
  using type = decltype(std::declval<PA&>()[0, field]);
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
using particle_num_t = particle_field_t<h, P>;

/// Particle vector type.
template<class P>
  requires particle_view<P, r> || particle_array<P, r>
using particle_vec_t = particle_field_t<r, P>;

/// Particle space dimension.
template<class P>
  requires particle_view<P> || particle_array<P>
inline constexpr auto particle_dim_v = vec_dim_v<particle_vec_t<P>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Check particle fields presence.
template<class P>
  requires particle_view<P> || particle_array<P>
consteval auto has(field auto... fields) -> bool {
  constexpr auto present_fields = std::remove_cvref_t<P>::fields;
  return present_fields.includes(meta::Set{fields...});
}

/// Check particle uniform fields presence.
template<class P>
  requires particle_view<P> || particle_array<P>
consteval auto has_uniform(field auto... uniforms) -> bool {
  constexpr auto present_fields = std::remove_cvref_t<P>::uniform_fields;
  return present_fields.includes(meta::Set{uniforms...});
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
  meta::Set{fields...}.for_each([&particle](auto field) {
    if constexpr (has<PV>(field)) particle[field] = {};
  });
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
