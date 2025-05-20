/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
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

/// Particle view.
template<class ParticleArray>
class ParticleView final {
public:

  /// Particle array type.
  using Array = std::remove_const_t<ParticleArray>;

  /// Space type.
  using Space = decltype(auto{Array::space});

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

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

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

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Check if the particle has all of the specified fields.
  constexpr auto has(field auto... fields) const noexcept -> bool {
    return array().have(fields...);
  }

  /// Check if the particle has any of the specified fields.
  constexpr auto has_any(field auto... fields) const noexcept -> bool {
    return array().have_any(fields...);
  }

  /// Particle field value.
  template<field Field>
  constexpr auto operator[](Field field) const noexcept
      -> std::conditional_t<std::is_const_v<ParticleArray>,
                            const field_value_t<Field, Space>&,
                            field_value_t<Field, Space>&> {
    return array()[field, index()];
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
template<class PV>
concept particle_view =
    specialization_of<std::remove_cvref_t<PV>, ParticleView>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Particle array.
template<space Space, field_set Uniforms, field_set Varyings>
class ParticleArray final {
public:

  /// Particle space.
  static constexpr Space space{};

  static constexpr Uniforms::Set static_uniforms_{};
  static constexpr Varyings::Set static_varyings_{};
  static_assert((static_uniforms_ & static_varyings_) == TypeSet{},
                "Static uniforms and varyings must be disjoint!");

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Construct a particle array.
  ///
  /// @param space The space in which the particles are defined.
  /// @param equations The equations that define the particle fields.
  template<class Equations>
  constexpr explicit ParticleArray(Space /*space*/,
                                   const Equations& equations) noexcept
      : dynamic_uniforms_{equations.required_uniforms()},
        dynamic_varyings_{equations.required_varyings()} {}

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Write a particle array into a data series.
  void write(field_value_t<h_t, Space> time,
             data::DataSeriesView<data::DataStorage> series) const {
    auto time_step = series.create_time_step(static_cast<float64_t>(time));

    // Write uniform fields.
    auto time_step_uniforms = time_step.uniforms();
    dynamic_uniforms_.for_each([&time_step_uniforms, this](auto field) {
      time_step_uniforms.create_array(field.field_name,
                                      std::span{&field[*this], 1});
    });

    // Write varying fields.
    auto time_step_varyings = time_step.varyings();
    dynamic_varyings_.for_each([&time_step_varyings, this](auto field) {
      time_step_varyings.create_array(field.field_name, field[*this]);
    });
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Number of particles.
  constexpr auto size() const noexcept -> size_t {
    return std::get<static_varyings_.find(r)>(varying_data_).size();
  }

  /// Reserve amount of particles.
  constexpr void reserve(size_t capacity) {
    dynamic_varyings_.for_each([this, capacity]<field Field>(Field /*field*/) {
      std::get<static_varyings_.find(Field{})>(varying_data_).reserve(capacity);
    });
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
    dynamic_varyings_.for_each([this, index]<field Field>(Field /*field*/) {
      auto& data = std::get<static_varyings_.find(Field{})>(varying_data_);
      data.emplace(data.begin() + index);
    });

    return (*this)[index];
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

  /// Check if the particles have all of the specified uniform fields.
  constexpr auto have_uniform(field auto... fields) const noexcept -> bool {
    return (dynamic_uniforms_.contains(fields) && ...);
  }

  /// Check if the particles have any of the specified varying fields.
  constexpr auto have_varying(field auto... fields) const noexcept -> bool {
    return (dynamic_varyings_.contains(fields) && ...);
  }

  /// Check if the particles have all of the specified fields.
  constexpr auto have(field auto... fields) const noexcept -> bool {
    return have_uniform(fields...) || have_varying(fields...);
  }

  /// Check if the particles have any of the specified fields.
  constexpr auto have_any(field auto... fields) const noexcept -> bool {
    return (have(fields) || ...);
  }

  /// Particle at index.
  constexpr auto operator[](this auto& self, size_t index) noexcept {
    TIT_ASSERT(index < self.size(), "Particle index is out of range.");
    return ParticleView{self, index};
  }

  /// Values for the specified field.
  /// @{
  template<class Self, field Field>
    requires (static_uniforms_.contains(Field{}))
  constexpr auto operator[](this Self& self, Field /*field*/) noexcept
      -> std::conditional_t<std::is_const_v<Self>,
                            const field_value_t<Field, Space>&,
                            field_value_t<Field, Space>&> {
    TIT_ASSERT(self.dynamic_uniforms_.contains(Field{}),
               "Field is not present in the dynamic uniform set!");
    return std::get<static_uniforms_.find(Field{})>(self.uniform_data_);
  }
  template<class Self, field Field>
    requires (static_varyings_.contains(Field{}))
  constexpr auto operator[](this Self& self, Field /*field*/) noexcept
      -> std::conditional_t<std::is_const_v<Self>,
                            std::span<const field_value_t<Field, Space>>,
                            std::span<field_value_t<Field, Space>>> {
    TIT_ASSERT(self.dynamic_varyings_.contains(Field{}),
               "Field is not present in the dynamic varying set!");
    return std::span{
        std::get<static_varyings_.find(Field{})>(self.varying_data_)};
  }
  /// @}

  /// Value of the specified field at the specified index.
  template<class Self, field Field>
  constexpr auto operator[](this Self& self,
                            Field /*field*/,
                            size_t index) noexcept
      -> std::conditional_t<std::is_const_v<Self>,
                            const field_value_t<Field, Space>&,
                            field_value_t<Field, Space>&> {
    TIT_ASSERT(index < self.size(), "Particle index is out of range.");

    // Check uniform fields.
    if constexpr (static_uniforms_.contains(Field{})) {
      TIT_ASSERT(self.dynamic_uniforms_.contains(Field{}),
                 "Field is not present in the dynamic uniform set!");
      return std::get<static_uniforms_.find(Field{})>(self.uniform_data_);
    }

    // Check varying fields.
    if constexpr (static_varyings_.contains(Field{})) {
      TIT_ASSERT(self.dynamic_varyings_.contains(Field{}),
                 "Field '{}' is not present in the dynamic varying set!");
      return std::get<static_varyings_.find(Field{})>(self.varying_data_) //
          [index];
    }

    TIT_ASSERT(false, "Field is not present in the static set!");
    std::unreachable();
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  std::array<size_t, std::to_underlying(ParticleType::count) + 1>
      particle_ranges_{0};

  [[no_unique_address]] Uniforms dynamic_uniforms_;
  [[no_unique_address]] Varyings dynamic_varyings_;

  decltype([]<field... Fields>(TypeSet<Fields...> /*fields*/) {
    return std::tuple<field_value_t<Fields, Space>...>{};
  }(static_uniforms_)) uniform_data_;

  decltype([]<field... Fields>(TypeSet<Fields...> /*fields*/) {
    return std::tuple<std::vector<field_value_t<Fields, Space>>...>{};
  }(static_varyings_)) varying_data_;

}; // class ParticleArray

template<class Space, class Equations>
ParticleArray(Space, const Equations&)
    -> ParticleArray<Space,
                     decltype(std::declval<Equations>().required_uniforms()),
                     decltype(std::declval<Equations>().required_varyings())>;

/// Particle array type.
template<class PA>
concept particle_array =
    specialization_of<std::remove_cvref_t<PA>, ParticleArray>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace impl {

template<field Field, class P>
struct particle_field_reference;

template<field Field, particle_view PV>
struct particle_field_reference<Field, PV> :
    std::type_identity<decltype(std::declval<PV&>()[Field{}])> {};

template<field Field, particle_array PA>
struct particle_field_reference<Field, PA> :
    std::type_identity<decltype(std::declval<PA&>()[Field{}, 0])> {};

} // namespace impl

/// Particle field reference type.
template<field Field, class P>
  requires particle_view<P> || particle_array<P>
using particle_field_reference_t =
    typename impl::particle_field_reference<Field, P>::type;

/// Particle field type.
template<field Field, class P>
  requires particle_view<P> || particle_array<P>
using particle_field_t =
    std::remove_cvref_t<particle_field_reference_t<Field, P>>;

/// Particle scalar type.
template<class P>
  requires particle_view<P> || particle_array<P>
using particle_num_t = particle_field_t<h_t, P>; // I'm sure `h` is present.

/// Particle vector type.
template<class P>
  requires particle_view<P> || particle_array<P>
using particle_vec_t = particle_field_t<r_t, P>; // I'm sure `r` is present.

/// Particle space dimension.
template<class P>
  requires particle_view<P> || particle_array<P>
inline constexpr auto particle_dim_v = vec_dim_v<particle_vec_t<P>>;

/// Particle view type with the specific number type.
template<class PV, class Num>
concept particle_view_n =
    particle_view<PV> && std::same_as<particle_num_t<PV>, Num>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Clear fields of the specified particle.
template<class PV, field... Fields>
constexpr void clear([[maybe_unused]] PV& pv, Fields... fields) {
  TypeSet{fields...}.for_each([&pv](auto field) {
    if (pv.has(field)) pv[field] = {};
  });
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
