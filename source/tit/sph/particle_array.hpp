/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <limits>
#include <optional>
#include <ranges>
#include <span>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "tit/core/assert.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/type.hpp"
#include "tit/core/vec.hpp"
#include "tit/sph/field.hpp"

namespace tit::sph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Stable particle identifier.
enum class ParticleID : std::uint64_t {};

/// Invalid particle identifier.
inline constexpr ParticleID invalid_particle_id{
    std::numeric_limits<std::uint64_t>::max()};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Particle type.
enum class ParticleType : std::uint8_t {
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
  constexpr ParticleView(ParticleArray& array, std::size_t index) noexcept
      : array_{&array}, index_{index} {}

  /// Associated particle array.
  constexpr auto array() const noexcept -> ParticleArray& {
    TIT_ASSERT(array_ != nullptr, "Particle array was not set.");
    return *array_;
  }

  /// Associated particle index.
  constexpr auto index() const noexcept -> std::size_t {
    return index_;
  }

  /// Stable particle identifier.
  constexpr auto id() const noexcept -> ParticleID {
    return array().id(index());
  }

  /// Check if the particle is owned by this process.
  constexpr auto is_owned() const noexcept -> bool {
    return array().is_owned(index());
  }

  /// Check if the particle is a read-only ghost.
  constexpr auto is_ghost() const noexcept -> bool {
    return array().is_ghost(index());
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
  template<class Self, field Field>
  constexpr auto operator[](this Self&& self, Field field) noexcept
      -> decltype(auto) {
    static_assert(fields.contains(Field{}));
    return self.array()[self.index(), field];
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Compare particle views.
  friend constexpr auto operator==(ParticleView a, ParticleView b) noexcept
      -> bool {
    TIT_ASSERT(&a.array() == &b.array(),
               "Particle views must refer to the same array!");
    return a.index() == b.index();
  }

  /// Distance between particle view indices.
  friend constexpr auto operator-(ParticleView a, ParticleView b) noexcept
      -> std::ptrdiff_t {
    TIT_ASSERT(&a.array() == &b.array(),
               "Particle views must refer to the same array!");
    return a.index() - b.index();
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  ParticleArray* array_;
  std::size_t index_;

}; // class ParticleView

/// Particle view type.
///
/// @tparam fields Fields that the view should contain.
template<class PV, auto... fields>
concept particle_view =
    specialization_of<std::remove_cvref_t<PV>, ParticleView> &&
    field_set<decltype(TypeSet{fields...})> &&
    (TypeSet{fields...} <= std::remove_cvref_t<PV>::fields);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Explicit particle field storage layout.
template<field_set Uniforms, field_set Varyings>
class ParticleLayout final {
public:

  /// Array-wise constant fields.
  static constexpr Uniforms uniform_fields{};

  /// Per-particle fields.
  static constexpr Varyings varying_fields{};

  /// Construct a field layout.
  consteval explicit ParticleLayout(Uniforms /*uniforms*/ = {},
                                    Varyings /*varyings*/ = {}) {
    static_assert((uniform_fields & varying_fields) == TypeSet{});
  }

}; // class ParticleLayout

template<class Uniforms, class Varyings>
ParticleLayout(Uniforms, Varyings) -> ParticleLayout<Uniforms, Varyings>;

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

  /// Packed width of a stable ID followed by all per-particle field values.
  static constexpr std::size_t packed_particle_size =
      sizeof(ParticleID) + []<class... Fields>(TypeSet<Fields...> /*fields*/) {
        static_assert((
            std::is_trivially_copyable_v<field_value_t<Fields, Space>> && ...));
        return (sizeof(field_value_t<Fields, Space>) + ... + std::size_t{0});
      }(varying_fields);

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Construct a particle array.
  ///
  /// @param space The space in which the particles are defined.
  /// @param layout Explicit uniform and varying field storage layout.
  constexpr explicit ParticleArray(
      Space /*space*/,
      ParticleLayout<Uniforms, Varyings> /*layout*/) noexcept {}

  /// Number of particles.
  constexpr auto size() const noexcept -> std::size_t {
    return particle_ids_.size();
  }

  /// Number of owned mobile particles.
  constexpr auto num_owned() const noexcept -> std::size_t {
    return owned_end_;
  }

  /// Number of fixed boundary particles.
  constexpr auto num_fixed() const noexcept -> std::size_t {
    return fixed_end_ - owned_end_;
  }

  /// Number of ghost particles.
  constexpr auto num_ghosts() const noexcept -> std::size_t {
    return size() - fixed_end_;
  }

  /// Reserve amount of particles.
  constexpr void reserve(std::size_t capacity) {
    particle_ids_.reserve(capacity);
    id_to_index_.reserve(capacity);
    auto& [... cols] = varying_data_;
    ((cols.reserve(capacity)), ...);
  }

  /// Appends a new particle of the specified type @p type.
  constexpr auto append(ParticleType type) -> ParticleView<ParticleArray> {
    TIT_ENSURE(next_id_ != invalid_particle_id,
               "Particle identifier space is exhausted.");
    const auto id = next_id_;
    next_id_ = ParticleID{std::to_underlying(next_id_) + 1};
    return append(type, id);
  }

  /// Appends a new particle with a stable identifier.
  constexpr auto append(ParticleType type, ParticleID id)
      -> ParticleView<ParticleArray> {
    TIT_ASSERT(type < ParticleType::count, "Invalid particle type.");
    TIT_ASSERT(id != invalid_particle_id, "Invalid particle identifier.");
    TIT_ASSERT(!id_to_index_.contains(id), "Duplicate particle identifier.");

    const auto index = type == ParticleType::fluid ? owned_end_ : fixed_end_;
    insert_(index, id);
    if (type == ParticleType::fluid) ++owned_end_;
    ++fixed_end_;
    update_next_id_(id);
    return (*this)[index];
  }

  /// Append a read-only ghost particle.
  constexpr auto append_ghost(ParticleID id) -> ParticleView<ParticleArray> {
    TIT_ASSERT(id != invalid_particle_id, "Invalid particle identifier.");
    TIT_ASSERT(!id_to_index_.contains(id), "Duplicate particle identifier.");
    const auto index = size();
    insert_(index, id);
    return (*this)[index];
  }

  /// Remove all ghost particles and invalidate their views.
  constexpr void clear_ghosts() {
    for (const auto id : particle_ids_ | std::views::drop(fixed_end_)) {
      id_to_index_.erase(id);
    }
    const auto fixed_end = static_cast<std::ptrdiff_t>(fixed_end_);
    particle_ids_.erase(particle_ids_.begin() + fixed_end, particle_ids_.end());
    auto& [... cols] = varying_data_;
    ((cols.erase(cols.begin() + fixed_end, cols.end())), ...);
  }

  /// Serialize one particle into the process-local MPI exchange format.
  void pack(std::size_t index, std::span<std::byte> buffer) const {
    TIT_ASSERT(index < size(), "Particle index is out of range.");
    TIT_ENSURE(buffer.size() == packed_particle_size,
               "Particle exchange buffer has an invalid size.");
    std::size_t offset = 0;
    pack_value_(buffer, offset, particle_ids_[index]);
    const auto& [... cols] = varying_data_;
    (pack_value_(buffer, offset, cols[index]), ...);
    TIT_ASSERT(offset == buffer.size(), "Particle packing size mismatch.");
  }

  /// Packed width of a stable ID followed by selected per-particle fields.
  template<field... Fields>
  static consteval auto packed_size(TypeSet<Fields...> /*fields*/)
      -> std::size_t {
    static_assert(TypeSet<Fields...>{} <= varying_fields);
    static_assert(
        (std::is_trivially_copyable_v<field_value_t<Fields, Space>> && ...));
    return sizeof(ParticleID) +
           (sizeof(field_value_t<Fields, Space>) + ... + std::size_t{0});
  }

  /// Serialize selected fields for an existing particle.
  template<field... Fields>
  void pack(std::size_t index,
            std::span<std::byte> buffer,
            TypeSet<Fields...> selected_fields) const {
    TIT_ASSERT(index < size(), "Particle index is out of range.");
    TIT_ENSURE(buffer.size() == packed_size(selected_fields),
               "Particle field exchange buffer has an invalid size.");
    std::size_t offset = 0;
    pack_value_(buffer, offset, particle_ids_[index]);
    (pack_value_(buffer, offset, (*this)[index, Fields{}]), ...);
    TIT_ASSERT(offset == buffer.size(),
               "Particle field packing size mismatch.");
  }

  /// Update selected fields of an existing ghost from an MPI record.
  template<field... Fields>
  void update_ghost_packed(std::span<const std::byte> buffer,
                           TypeSet<Fields...> selected_fields) {
    TIT_ENSURE(buffer.size() == packed_size(selected_fields),
               "Particle field exchange buffer has an invalid size.");
    std::size_t offset = 0;
    const auto id = unpack_value_<ParticleID>(buffer, offset);
    const auto index = find(id);
    TIT_ENSURE(index.has_value(),
               "Particle field exchange references an unknown particle.");
    TIT_ENSURE(is_ghost(*index),
               "Particle field exchange references non-ghost state.");
    (((*this)[*index, Fields{}] =
          unpack_value_<field_value_t<Fields, Space>>(buffer, offset)),
     ...);
    TIT_ASSERT(offset == buffer.size(),
               "Particle field unpacking size mismatch.");
  }

  /// Append an owned or fixed particle from the MPI exchange format.
  auto append_packed(ParticleType type, std::span<const std::byte> buffer)
      -> ParticleView<ParticleArray> {
    return append_packed_(type, false, buffer);
  }

  /// Append a read-only ghost from the MPI exchange format.
  auto append_ghost_packed(std::span<const std::byte> buffer)
      -> ParticleView<ParticleArray> {
    return append_packed_(ParticleType::fluid, true, buffer);
  }

  /// Append a read-only ghost containing selected exchanged fields.
  template<field... Fields>
  auto append_ghost_packed(std::span<const std::byte> buffer,
                           TypeSet<Fields...> selected_fields)
      -> ParticleView<ParticleArray> {
    TIT_ENSURE(buffer.size() == packed_size(selected_fields),
               "Particle field exchange buffer has an invalid size.");
    std::size_t offset = 0;
    const auto id = unpack_value_<ParticleID>(buffer, offset);
    auto particle = append_ghost(id);
    ((particle[Fields{}] =
          unpack_value_<field_value_t<Fields, Space>>(buffer, offset)),
     ...);
    TIT_ASSERT(offset == buffer.size(),
               "Particle field unpacking size mismatch.");
    return particle;
  }

  /// Remove an owned mobile particle and invalidate views at or after it.
  void erase_owned(std::size_t index) {
    TIT_ENSURE(index < owned_end_, "Particle is not owned mobile state.");
    id_to_index_.erase(particle_ids_[index]);
    const auto offset = static_cast<std::ptrdiff_t>(index);
    particle_ids_.erase(particle_ids_.begin() + offset);
    auto& [... cols] = varying_data_;
    ((cols.erase(cols.begin() + offset)), ...);
    --owned_end_;
    --fixed_end_;
    for (std::size_t i = index; i < size(); ++i) {
      id_to_index_.insert_or_assign(particle_ids_[i], i);
    }
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// All process-local particles, including ghosts.
  constexpr auto local(this auto& self) noexcept {
    return std::views::iota(std::size_t{0}, self.size()) |
           std::views::transform(
               [&self](std::size_t index) { return self[index]; });
  }

  /// All process-local particles, including ghosts.
  constexpr auto all(this auto& self) noexcept {
    return self.local();
  }

  /// Owned mobile particles.
  constexpr auto owned(this auto& self) noexcept {
    return std::views::iota(std::size_t{0}, self.owned_end_) |
           std::views::transform(
               [&self](std::size_t index) { return self[index]; });
  }

  /// Particles evaluated locally as numerical targets.
  constexpr auto active(this auto& self) noexcept {
    return std::views::iota(std::size_t{0}, self.fixed_end_) |
           std::views::transform(
               [&self](std::size_t index) { return self[index]; });
  }

  /// Owned fluid particles.
  constexpr auto fluid(this auto& self) noexcept {
    return self.owned();
  }

  /// Fixed particles.
  constexpr auto fixed(this auto& self) noexcept {
    return std::views::iota(self.owned_end_, self.fixed_end_) |
           std::views::transform(
               [&self](std::size_t index) { return self[index]; });
  }

  /// Read-only ghost particles.
  constexpr auto ghosts(this auto& self) noexcept {
    return std::views::iota(self.fixed_end_, self.size()) |
           std::views::transform(
               [&self](std::size_t index) { return self[index]; });
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Check if the particle has the specified type.
  constexpr auto has_type(std::size_t index, ParticleType type) const noexcept
      -> bool {
    TIT_ASSERT(index < size(), "Particle index is out of range.");
    TIT_ASSERT(type < ParticleType::count, "Invalid particle type.");
    if (type == ParticleType::fluid) {
      return index < owned_end_ || index >= fixed_end_;
    }
    return owned_end_ <= index && index < fixed_end_;
  }

  /// Check if the particle is owned by this process.
  constexpr auto is_owned(std::size_t index) const noexcept -> bool {
    TIT_ASSERT(index < size(), "Particle index is out of range.");
    return index < owned_end_;
  }

  /// Check if the particle is a read-only ghost.
  constexpr auto is_ghost(std::size_t index) const noexcept -> bool {
    TIT_ASSERT(index < size(), "Particle index is out of range.");
    return index >= fixed_end_;
  }

  /// Get a stable particle identifier.
  constexpr auto id(std::size_t index) const noexcept -> ParticleID {
    TIT_ASSERT(index < size(), "Particle index is out of range.");
    return particle_ids_[index];
  }

  /// Stable identifiers of all process-local particles.
  constexpr auto ids() const noexcept -> std::span<const ParticleID> {
    return particle_ids_;
  }

  /// Find the local index of a stable particle identifier.
  constexpr auto find(ParticleID id) const noexcept
      -> std::optional<std::size_t> {
    if (const auto iter = id_to_index_.find(id); iter != id_to_index_.end()) {
      return iter->second;
    }
    return std::nullopt;
  }

  /// Particle at index.
  constexpr auto operator[](this auto& self, std::size_t index) noexcept {
    TIT_ASSERT(index < self.size(), "Particle index is out of range.");
    return ParticleView{self, index};
  }

  /// Particle field at index.
  template<field Field>
  constexpr auto operator[](this auto& self,
                            [[maybe_unused]] std::size_t index,
                            Field /*field*/) noexcept -> decltype(auto) {
    static_assert(fields.contains(Field{}));
    TIT_ASSERT(index < self.size(), "Particle index is out of range.");
    if constexpr (uniform_fields.contains(Field{})) {
      return std::get<uniform_fields.find(Field{})>(self.uniform_data_);
    } else if constexpr (varying_fields.contains(Field{})) {
      return std::get<varying_fields.find(Field{})>(self.varying_data_)[index];
    } else {
      static_assert(false);
    }
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
    } else {
      static_assert(false);
    }
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  template<class Value>
  static void pack_value_(std::span<std::byte> buffer,
                          std::size_t& offset,
                          const Value& value) {
    static_assert(std::is_trivially_copyable_v<Value>);
    TIT_ASSERT(sizeof(Value) <= buffer.size() - offset,
               "Particle exchange buffer overflow.");
    std::memcpy(buffer.data() + offset, &value, sizeof(Value));
    offset += sizeof(Value);
  }

  template<class Value>
  static auto unpack_value_(std::span<const std::byte> buffer,
                            std::size_t& offset) -> Value {
    static_assert(std::is_trivially_copyable_v<Value>);
    TIT_ASSERT(sizeof(Value) <= buffer.size() - offset,
               "Particle exchange buffer overflow.");
    Value value{};
    std::memcpy(&value, buffer.data() + offset, sizeof(Value));
    offset += sizeof(Value);
    return value;
  }

  auto append_packed_(ParticleType type,
                      bool ghost,
                      std::span<const std::byte> buffer)
      -> ParticleView<ParticleArray> {
    TIT_ENSURE(buffer.size() == packed_particle_size,
               "Particle exchange buffer has an invalid size.");
    std::size_t offset = 0;
    const auto id = unpack_value_<ParticleID>(buffer, offset);
    auto particle = ghost ? append_ghost(id) : append(type, id);
    auto& [... cols] = varying_data_;
    ((cols[particle.index()] = unpack_value_<
          typename std::remove_reference_t<decltype(cols)>::value_type>(
          buffer,
          offset)),
     ...);
    TIT_ASSERT(offset == buffer.size(), "Particle unpacking size mismatch.");
    return particle;
  }

  struct ParticleIDHash_ final {
    constexpr auto operator()(ParticleID id) const noexcept -> std::size_t {
      return std::hash<std::uint64_t>{}(std::to_underlying(id));
    }
  };

  constexpr void insert_(std::size_t index, ParticleID id) {
    TIT_ASSERT(index <= size(), "Particle insertion index is out of range.");
    const auto offset = static_cast<std::ptrdiff_t>(index);
    particle_ids_.insert(particle_ids_.begin() + offset, id);
    auto& [... cols] = varying_data_;
    ((cols.emplace(cols.begin() + offset)), ...);
    for (std::size_t i = index; i < size(); ++i) {
      id_to_index_.insert_or_assign(particle_ids_[i], i);
    }
  }

  constexpr void update_next_id_(ParticleID id) noexcept {
    if (id < next_id_) return;
    const auto value = std::to_underlying(id);
    const auto max_id = std::to_underlying(invalid_particle_id);
    next_id_ =
        value == max_id - 1 ? invalid_particle_id : ParticleID{value + 1};
  }

  std::size_t owned_end_ = 0;
  std::size_t fixed_end_ = 0;
  ParticleID next_id_{0};
  std::vector<ParticleID> particle_ids_;
  std::unordered_map<ParticleID, std::size_t, ParticleIDHash_> id_to_index_;

  [[no_unique_address]] decltype([]<class... Fields>(TypeSet<Fields...> /*f*/) {
    return std::tuple<field_value_t<Fields, Space>...>{};
  }(uniform_fields)) uniform_data_;

  [[no_unique_address]] decltype([]<class... Fields>(TypeSet<Fields...> /*f*/) {
    return std::tuple<std::vector<field_value_t<Fields, Space>>...>{};
  }(varying_fields)) varying_data_;

}; // class ParticleArray

template<class Space, class Uniforms, class Varyings>
ParticleArray(Space, ParticleLayout<Uniforms, Varyings>)
    -> ParticleArray<Space, Uniforms, Varyings>;

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
    impl::particle_field_reference<field, P>::type;

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

/// Check particle uniform fields presence.
template<class P>
  requires particle_view<P> || particle_array<P>
consteval auto has_uniform(field auto... uniforms) -> bool {
  constexpr auto present_fields = std::remove_cvref_t<P>::uniform_fields;
  return TypeSet{uniforms...} <= present_fields;
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
