/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <ranges>
#include <unordered_map>
#include <vector>

#include "tit/core/assert.hpp"
#include "tit/core/mat.hpp"
#include "tit/core/vec.hpp"
#include "tit/data/storage.hpp"
#include "tit/sph/field2.hpp"

namespace tit::sph {

enum class ParticleID : std::size_t;

/// Particle type.
enum class ParticleType : std::uint8_t {
  vertex, ///< Vertex particle.
  fluid,  ///< Fluid particle. Keep last.
  _count,
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Field view.
template<class Val>
class FieldView final {
public:

  /// Construct a field view.
  constexpr FieldView(std::span<Val> data) noexcept : data_{data} {}

  /// Field value for the specified particle.
  constexpr auto operator[](ParticleID id) const noexcept -> Val& {
    const auto index = std::to_underlying(id);
    TIT_ASSERT(index < data_.size(), "Field is corrupted.");
    return data_[index];
  }

  /// Average of the field values over the specified particles.
  /// @{
  template<std::same_as<ParticleID>... IDs>
  constexpr auto avg(IDs... ids) const noexcept -> Val {
    return tit::avg((*this)[ids]...);
  }
  template<std::same_as<ParticleID>... IDs>
  constexpr auto operator[](std::tuple<IDs...> ids) const noexcept -> Val {
    return std::apply([this](IDs... ids) { return this->avg(ids...); }, ids);
  }
  /// @}

  /// Field value delta for the specified particles.
  constexpr auto operator[](auto id_a, auto id_b) const noexcept -> Val {
    return (*this)[id_a] - (*this)[id_b];
  }

private:

  std::span<Val> data_;

}; // class FieldView

/// Scalar field view.
template<class Num>
using ScalarFieldView = FieldView<Num>;

/// Vector field view.
template<class Num, std::size_t Dim>
using VectorFieldView = FieldView<Vec<Num, Dim>>;

/// Matrix field view.
template<class Num, std::size_t Dim>
using MatrixFieldView = FieldView<Mat<Num, Dim>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Cloud of particles with associated fields.
template<class Num, std::size_t Dim>
class ParticleCloud final {
public:

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Construct a particle cloud.
  constexpr explicit ParticleCloud() noexcept = default;

  /// Copy selected fields from the source cloud.
  template<std::same_as<FieldID>... FieldIDs>
  constexpr explicit ParticleCloud(const ParticleCloud& source,
                                   FieldIDs... field_ids) {
    for (const auto& [source_field_id, source_field] : source.scalar_fields_) {
      if (((source_field_id == field_ids) || ...)) {
        scalar_fields_.emplace(source_field_id, source_field);
      }
    }
    for (const auto& [source_field_id, source_field] : source.vector_fields_) {
      if (((source_field_id == field_ids) || ...)) {
        vector_fields_.emplace(source_field_id, source_field);
      }
    }
    for (const auto& [source_field_id, source_field] : source.matrix_fields_) {
      if (((source_field_id == field_ids) || ...)) {
        matrix_fields_.emplace(source_field_id, source_field);
      }
    }
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Number of all particles.
  constexpr auto count() const noexcept -> std::size_t {
    return ranges_.back();
  }

  /// Number of particles of the specific type.
  constexpr auto count(ParticleType type) const noexcept -> std::size_t {
    TIT_ASSERT(type < ParticleType::_count, "Invalid particle type.");
    const auto type_index = std::to_underlying(type);
    return ranges_[type_index + 1] - ranges_[type_index];
  }

  /// Number of vertex particles.
  constexpr auto count_vertex() const noexcept -> std::size_t {
    return count(ParticleType::vertex);
  }

  /// Number of fluid particles.
  constexpr auto count_fluid() const noexcept -> std::size_t {
    return count(ParticleType::fluid);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// IDs of all particles.
  constexpr auto ids() const noexcept {
    return std::views::iota(std::size_t{0}, count()) |
           std::views::transform([](std::size_t a) { return ParticleID{a}; });
  }

  /// IDs of particles of the specific type.
  constexpr auto ids(ParticleType type) const noexcept {
    TIT_ASSERT(type < ParticleType::_count, "Invalid particle type.");
    const auto type_index = std::to_underlying(type);
    return std::views::iota(ranges_[type_index], ranges_[type_index + 1]) |
           std::views::transform([](std::size_t a) { return ParticleID{a}; });
  }

  /// IDs of vertex particles.
  constexpr auto vertex() const noexcept {
    return ids(ParticleType::vertex);
  }

  /// IDs of fluid particles.
  constexpr auto fluid() const noexcept {
    return ids(ParticleType::fluid);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Type of the particle.
  constexpr auto type(ParticleID id) const noexcept -> ParticleType {
    const auto index = std::to_underlying(id);
    TIT_ASSERT(index < count(), "Particle index is out of range.");
    const auto* const type_iter = std::ranges::lower_bound(ranges_, index);
    TIT_ASSERT(type_iter != ranges_.end(), "Particle ranges are corrupted.");
    return static_cast<ParticleType>(type_iter - ranges_.begin());
  }

  /// Check if the particle has the specified type.
  /// Faster than `type(id) == type`.
  constexpr auto type_is(ParticleID id, ParticleType type) const noexcept
      -> bool {
    TIT_ASSERT(type < ParticleType::_count, "Invalid particle type.");
    const auto type_index = std::to_underlying(type);
    const auto index = std::to_underlying(id);
    TIT_ASSERT(index < count(), "Particle index is out of range.");
    return ranges_[type_index] <= index && index < ranges_[type_index + 1];
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Request a specific scalar field. Field is created if it does not exist.
  constexpr auto request_scalar(FieldID id) noexcept -> ScalarFieldView<Num> {
    auto&& [iter, inserted] = scalar_fields_.try_emplace(id);
    if (inserted) iter->second.resize(count());
    return ScalarFieldView<Num>{iter->second};
  }

  /// Require a specific scalar field. Required field must exist.
  constexpr auto require_scalar(FieldID id) noexcept -> ScalarFieldView<Num> {
    const auto iter = scalar_fields_.find(id);
    TIT_ASSERT(iter != scalar_fields_.end(), "Scalar field does not exist.");
    return ScalarFieldView<Num>{iter->second};
  }

  /// Request a specific vector field. Field is created if it does not exist.
  constexpr auto request_vector(FieldID id) noexcept
      -> VectorFieldView<Num, Dim> {
    auto&& [iter, inserted] = vector_fields_.try_emplace(id);
    if (inserted) iter->second.resize(count());
    return VectorFieldView<Num, Dim>{iter->second};
  }

  /// Require a specific vector field. Required field must exist.
  constexpr auto require_vector(FieldID id) noexcept
      -> VectorFieldView<Num, Dim> {
    const auto iter = vector_fields_.find(id);
    TIT_ASSERT(iter != vector_fields_.end(), "Vector field does not exist.");
    return VectorFieldView<Num, Dim>{iter->second};
  }

  /// Request a specific matrix field. Field is created if it does not exist.
  constexpr auto request_matrix(FieldID id) noexcept
      -> MatrixFieldView<Num, Dim> {
    auto&& [iter, inserted] = matrix_fields_.try_emplace(id);
    if (inserted) iter->second.resize(count());
    return MatrixFieldView<Num, Dim>{iter->second};
  }

  /// Require a specific matrix field. Required field must exist.
  constexpr auto require_matrix(FieldID id) noexcept
      -> MatrixFieldView<Num, Dim> {
    const auto iter = matrix_fields_.find(id);
    TIT_ASSERT(iter != matrix_fields_.end(), "Matrix field does not exist.");
    return MatrixFieldView<Num, Dim>{iter->second};
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Write a particle array as a data series.
  void write(float64_t time, data::SeriesView<data::Storage> series) const {
    const auto frame = series.create_frame(time);
    for (const auto& [field_id, field] : scalar_fields_) {
      const auto name = field_name(field_id);
      if (name.starts_with("_")) continue;
      frame.create_array(name).write(field);
    }
    for (const auto& [field_id, field] : vector_fields_) {
      const auto name = field_name(field_id);
      if (name.starts_with("_")) continue;
      frame.create_array(name).write(field);
    }
    for (const auto& [field_id, field] : matrix_fields_) {
      const auto name = field_name(field_id);
      if (name.starts_with("_")) continue;
      frame.create_array(name).write(field);
    }
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  std::array<std::size_t, std::to_underlying(ParticleType::_count) + 1>
      ranges_{};
  std::unordered_map<FieldID, std::vector<Num>> scalar_fields_;
  std::unordered_map<FieldID, std::vector<Vec<Num, Dim>>> vector_fields_;
  std::unordered_map<FieldID, std::vector<Mat<Num, Dim>>> matrix_fields_;

}; // class ParticleCloud

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
