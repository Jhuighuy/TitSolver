/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <ranges>
#include <span>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "tit/core/assert.hpp"
#include "tit/core/float.hpp"
#include "tit/core/serialization.hpp"
#include "tit/core/type.hpp"
#include "tit/core/vec.hpp"
#include "tit/data/storage.hpp"
#include "tit/sph/field.hpp"

namespace tit::sph {

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

  /// Check if the particle is a ghost (mirror of a remotely-owned particle).
  constexpr auto is_ghost() const noexcept -> bool {
    return array().is_ghost(index());
  }

  /// Check if the particle is owned by the current process.
  constexpr auto is_owned() const noexcept -> bool {
    return !is_ghost();
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
    TIT_ASSERT(a.array().size() == b.array().size(),
               "Particle arrays must be of the same size!");
    return a.index() == b.index();
  }

  /// Distance between particle view indices.
  friend constexpr auto operator-(ParticleView a, ParticleView b) noexcept
      -> std::ptrdiff_t {
    TIT_ASSERT(a.array().size() == b.array().size(),
               "Particle arrays must be of the same size!");
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

  /// Write a particle array into a series.
  ///
  /// Only the owned particles are written: ghost particles are transient
  /// mirrors of particles owned (and written) by other processes. The global
  /// identifier field is solver-internal and is not part of the frame.
  void write(field_value_t<h_t, Space> time,
             data::SeriesView<data::Storage> series) const {
    auto frame = series.create_frame(static_cast<float64_t>(time));
    ParticleArray::varying_fields.for_each([&frame, this](auto field) {
      if constexpr (!infra_fields.contains(decltype(field){})) {
        const auto array = frame.create_array(field.field_name);
        array.write(field[*this].first(num_owned()));
      }
    });
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Number of particles, including the ghost particles.
  constexpr auto size() const noexcept -> std::size_t {
    return std::get<0>(varying_data_).size();
  }

  /// Number of the owned (non-ghost) particles.
  constexpr auto num_owned() const noexcept -> std::size_t {
    return ranges_[num_types_];
  }

  /// Reserve amount of particles.
  constexpr void reserve(std::size_t capacity) {
    auto& [... cols] = varying_data_;
    ((cols.reserve(capacity)), ...);
  }

  /// Appends a new owned particle of the specified type @p type.
  constexpr auto append(ParticleType type) -> ParticleView<ParticleArray> {
    return append_n(type, 1).front();
  }

  /// Appends @p count new owned particles of the specified type @p type.
  ///
  /// Returns the views of the appended particles.
  constexpr auto append_n(ParticleType type, std::size_t count) {
    const auto index = insert_(seg_index_(type, /*ghost=*/false), count);
    return views_(index, count);
  }

  /// Appends @p count new ghost particles of the specified type @p type.
  ///
  /// Ghost particles mirror particles owned by other processes. They carry
  /// no identity of their own: the global identifiers are to be filled by
  /// the communication layer from the owning process.
  constexpr auto append_ghosts_n(ParticleType type, std::size_t count) {
    const auto index = insert_(seg_index_(type, /*ghost=*/true), count);
    return views_(index, count);
  }

  /// Remove all the ghost particles.
  constexpr void clear_ghosts() {
    auto& [... cols] = varying_data_;
    ((cols.resize(num_owned())), ...);
    std::ranges::fill(ranges_ | std::views::drop(num_types_ + 1), num_owned());
  }

  /// Remove the owned particles at the specified indices.
  ///
  /// @param indices Sorted unique indices of the owned particles to remove.
  constexpr void erase_owned(std::span<const std::size_t> indices) {
    if (indices.empty()) return;
    TIT_ASSERT(std::ranges::is_sorted(indices), "Indices must be sorted.");
    TIT_ASSERT(indices.back() < num_owned(),
               "Only the owned particles can be erased.");

    // Compact each column by moving the surviving rows over the erased ones.
    auto& [... cols] = varying_data_;
    const auto erase_col = [indices](auto& col) {
      auto out = col.begin() + static_cast<std::ptrdiff_t>(indices.front());
      for (const auto [cursor, index] : std::views::enumerate(indices)) {
        const auto next =
            static_cast<std::size_t>(cursor) + 1 < indices.size() ?
                indices[static_cast<std::size_t>(cursor) + 1] :
                col.size();
        out = std::move(col.begin() + static_cast<std::ptrdiff_t>(index) + 1,
                        col.begin() + static_cast<std::ptrdiff_t>(next),
                        out);
      }
      col.erase(out, col.end());
    };
    ((erase_col(cols)), ...);

    // Adjust the segment boundaries.
    for (auto& range : ranges_ | std::views::drop(1)) {
      range -= static_cast<std::size_t>(
          std::ranges::lower_bound(indices, range) - indices.begin());
    }
  }

  /// Reorder the owned particles.
  ///
  /// @param perm Permutation of the owned particles: the particle that ends
  ///             up at position `i` is the one currently at `perm[i]`. The
  ///             permutation must map each segment onto itself.
  constexpr void reorder_owned(std::span<const std::size_t> perm) {
    TIT_ASSERT(perm.size() == num_owned(),
               "Permutation size must match the number of owned particles.");
    auto& [... cols] = varying_data_;
    const auto reorder_col = [perm]<class Val>(std::vector<Val>& col) {
      std::vector<Val> reordered(perm.size());
      for (const auto [index, old_index] : std::views::enumerate(perm)) {
        reordered[static_cast<std::size_t>(index)] = std::move(col[old_index]);
      }
      std::ranges::move(reordered, col.begin());
    };
    ((reorder_col(cols)), ...);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// All particles, including the ghost particles.
  constexpr auto all(this auto& self) noexcept {
    return self.views_(0, self.size());
  }

  /// All owned (non-ghost) particles.
  constexpr auto owned(this auto& self) noexcept {
    return self.views_(0, self.num_owned());
  }

  /// All ghost particles.
  constexpr auto ghost(this auto& self) noexcept {
    return self.views_(self.num_owned(), self.size() - self.num_owned());
  }

  /// Owned particles of the specified type.
  constexpr auto typed(this auto& self, ParticleType type) noexcept {
    TIT_ASSERT(type < ParticleType::count, "Invalid particle type.");
    const auto seg = seg_index_(type, /*ghost=*/false);
    return self.views_(self.ranges_[seg],
                       self.ranges_[seg + 1] - self.ranges_[seg]);
  }

  /// Owned fluid particles.
  constexpr auto fluid(this auto& self) noexcept {
    return self.typed(ParticleType::fluid);
  }

  /// Owned fixed particles.
  constexpr auto fixed(this auto& self) noexcept {
    return self.typed(ParticleType::fixed);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Check if the particle has the specified type.
  ///
  /// Both owned and ghost particles carry a type: a ghost mirror of a remote
  /// fluid particle is a fluid particle.
  constexpr auto has_type(std::size_t index, ParticleType type) const noexcept
      -> bool {
    TIT_ASSERT(type < ParticleType::count, "Invalid particle type.");
    const auto in_seg = [index, this](std::size_t seg) {
      return ranges_[seg] <= index && index < ranges_[seg + 1];
    };
    return in_seg(seg_index_(type, /*ghost=*/false)) ||
           in_seg(seg_index_(type, /*ghost=*/true));
  }

  /// Check if the particle is a ghost.
  constexpr auto is_ghost(std::size_t index) const noexcept -> bool {
    TIT_ASSERT(index < size(), "Particle index is out of range.");
    return index >= num_owned();
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

  /// Number of bytes used to pack the given fields of a single particle.
  template<field_set Fields>
  static consteval auto pack_width(Fields packed_fields) -> std::size_t {
    static_assert(Fields{} <= varying_fields);
    std::size_t width = 0;
    packed_fields.for_each([&width](auto field) {
      width += sizeof(field_value_t<decltype(field), Space>);
    });
    return width;
  }

  /// Pack the given fields of the given particles into a byte buffer.
  ///
  /// Fields of each particle are laid out contiguously, in the order they
  /// appear in @p fields; particles follow the order of @p indices.
  template<field_set Fields>
  constexpr void pack(std::span<const std::size_t> indices,
                      Fields packed_fields,
                      std::span<std::byte> out) const {
    TIT_ASSERT(out.size() >= indices.size() * pack_width(Fields{}),
               "Packing buffer is too small.");
    auto out_iter = out.begin();
    for (const auto index : indices) {
      TIT_ASSERT(index < size(), "Particle index is out of range.");
      packed_fields.for_each([index, &out_iter, this](auto field) {
        const auto bytes = to_byte_array((*this)[index, field]);
        out_iter = std::ranges::copy(bytes, out_iter).out;
      });
    }
  }

  /// Unpack the given fields of @p count particles starting at @p first
  /// from a byte buffer produced by @c pack.
  template<field_set Fields>
  constexpr void unpack(std::size_t first,
                        std::size_t count,
                        Fields packed_fields,
                        std::span<const std::byte> in) {
    TIT_ASSERT(first + count <= size(), "Particle range is out of bounds.");
    TIT_ASSERT(in.size() >= count * pack_width(Fields{}),
               "Unpacking buffer is too small.");
    for (const auto index : std::views::iota(first, first + count)) {
      packed_fields.for_each([index, &in, this](auto field) {
        using Val = field_value_t<decltype(field), Space>;
        (*this)[index, field] = from_bytes<Val>(in.first(sizeof(Val)));
        in = in.subspan(sizeof(Val));
      });
    }
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  // Storage segment index for the given particle type and ghost status.
  // Particles are stored contiguously in the following segment order:
  //   [ owned fluid | owned fixed | ghost fluid | ghost fixed ]
  static constexpr auto num_types_ = std::to_underlying(ParticleType::count);
  static constexpr auto seg_index_(ParticleType type, bool ghost) noexcept
      -> std::size_t {
    TIT_ASSERT(type < ParticleType::count, "Invalid particle type.");
    return (ghost ? num_types_ : 0) + std::to_underlying(type);
  }

  // Insert `count` value-initialized particles at the end of the segment
  // `seg` and return the index of the first inserted particle.
  constexpr auto insert_(std::size_t seg, std::size_t count) -> std::size_t {
    const auto index = ranges_[seg + 1];
    auto& [... cols] = varying_data_;
    ((cols.insert(cols.begin() + static_cast<std::ptrdiff_t>(index),
                  count,
                  {})),
     ...);
    for (auto& range : ranges_ | std::views::drop(seg + 1)) range += count;
    if constexpr (varying_fields.contains(gid_t{})) {
      // Ghost identities are filled by the communication layer.
      if (seg < num_types_) {
        for (auto& g : (*this)[gid_t{}].subspan(index, count)) g = next_gid_++;
      }
    }
    return index;
  }

  // Views of `count` particles starting at `index`.
  constexpr auto views_(this auto& self,
                        std::size_t index,
                        std::size_t count) noexcept {
    TIT_ASSERT(index + count <= self.size(), "Particle range is out of range.");
    return std::views::iota(index, index + count) |
           std::views::transform([&self](std::size_t i) { return self[i]; });
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  std::array<std::size_t, 2 * num_types_ + 1> ranges_{};
  std::uint64_t next_gid_ = 0;

  [[no_unique_address]] decltype([]<class... Fields>(TypeSet<Fields...> /*f*/) {
    return std::tuple<field_value_t<Fields, Space>...>{};
  }(uniform_fields)) uniform_data_;

  [[no_unique_address]] decltype([]<class... Fields>(TypeSet<Fields...> /*f*/) {
    return std::tuple<std::vector<field_value_t<Fields, Space>>...>{};
  }(varying_fields)) varying_data_;

}; // class ParticleArray

template<class Space, class Equations>
ParticleArray(Space, Equations) -> ParticleArray<
    Space,
    decltype(Equations::required_fields - Equations::modified_fields),
    decltype((Equations::required_fields & Equations::modified_fields) |
             infra_fields)>;

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
