/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <cstddef>
#include <fstream>
#include <iterator>
#include <ranges>
#include <span>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>

#include <oneapi/tbb/concurrent_vector.h>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/graph.hpp"
#include "tit/core/io.hpp"
#include "tit/core/mat.hpp"
#include "tit/core/meta.hpp"
#include "tit/core/multivector.hpp"
#include "tit/core/par.hpp"
#include "tit/core/profiler.hpp"
#include "tit/core/vec.hpp"

#include "tit/geom/bbox.hpp"
#include "tit/geom/inertial_bisection.hpp"
#include "tit/geom/search.hpp"

#include "tit/sph/field.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Space specification.
template<class Real_, size_t Dim>
  requires (1 <= Dim && Dim <= 3)
class Space {
public:

  /// Real number type, used in this space.
  using Real = Real_;

  /// Number of spatial dimensions.
  static constexpr size_t dim = Dim;

}; // class Space

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Particle view.
template<class ParticleArray>
  requires std::is_object_v<ParticleArray>
class ParticleView final {
private:

  ParticleArray* particles_;
  size_t particle_index_;

public:

  /// Set of particle fields that are present.
  static constexpr auto fields = std::remove_const_t<ParticleArray>::fields;

  /// Subset of particle fields that are array-wise constants.
  static constexpr auto constants =
      std::remove_const_t<ParticleArray>::constants;

  /// Subset of particle fields that are individual for each particle.
  static constexpr auto variables =
      std::remove_const_t<ParticleArray>::variables;

  /// Construct a particle view.
  constexpr ParticleView(ParticleArray& particles,
                         size_t particle_index) noexcept
      : particles_{&particles}, particle_index_{particle_index} {}

  /// Associated particle array.
  constexpr auto array() const noexcept -> ParticleArray& {
    TIT_ASSERT(particles_ != nullptr, "Particle array was not set.");
    return *particles_;
  }

  /// Associated particle index.
  constexpr auto index() const noexcept -> size_t {
    return particle_index_;
  }

  /// Compare particle views.
  constexpr auto operator==(ParticleView<ParticleArray> other) const noexcept
      -> bool {
    TIT_ASSERT(&array() == &other.array(),
               "Particles must belong to the same array.");
    return index() == other.index();
  }

  /// Particle field value.
  template<meta::type Field>
    requires (has<ParticleView, Field>())
  constexpr auto operator[](Field field) const noexcept -> decltype(auto) {
    return array()[index(), field];
  }

}; // class ParticleView

template<class ParticleArray>
ParticleView(ParticleArray&) -> ParticleView<ParticleArray>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// TODO: move it to an appropriate place!
#if COMPRESSIBLE_SOD_PROBLEM
inline constexpr auto Domain = geom::BBox{Vec{0.0}, Vec{2.0}};
#elif HARD_DAM_BREAKING
inline constexpr auto Domain = geom::BBox{Vec{0.0, 0.0}, Vec{4.0, 3.0}};
#elif EASY_DAM_BREAKING
inline constexpr auto Domain = geom::BBox{Vec{0.0, 0.0}, Vec{3.2196, 1.5}};
#else
inline constexpr auto Domain = geom::BBox{Vec{0.0, 0.0}, Vec{0.0, 0.0}};
#endif

/// Particle adjacency graph.
template<class ParticleArray, class EngineFactory = geom::GridFactory>
  requires std::is_object_v<ParticleArray> && std::is_object_v<EngineFactory>
class ParticleAdjacency final {
private:

  ParticleArray* particles_;
  EngineFactory engine_factory_;
  Graph adjacency_;
  tbb::concurrent_vector<size_t> fixed_;
  Graph interp_adjacency_;
  Multivector<std::tuple<size_t, size_t>> block_adjacency_;

public:

  /// Construct a particle adjacency graph.
  ///
  /// @param engine_factory Nearest-neighbors search engine factory.
  constexpr explicit ParticleAdjacency(
      ParticleArray& particles,
      EngineFactory engine_factory = {}) noexcept
      : particles_{&particles}, engine_factory_{std::move(engine_factory)} {}

  /// Associated particle array.
  constexpr auto array() const noexcept -> ParticleArray& {
    TIT_ASSERT(particles_ != nullptr, "Particle array was not set.");
    return *particles_;
  }

  /// Build an adjacency graph.
  ///
  /// @param radius_func Function that returns search radius for the
  ///                    specified particle view.
  template<class SearchRadiusFunc>
  constexpr void build(const SearchRadiusFunc& radius_func) {
    TIT_PROFILE_SECTION("ParticleAdjacency::build()");
    using PV = ParticleView<ParticleArray>;
    // -------------------------------------------------------------------------
    // STEP I: neighbors search.
    const auto positions = array()[r];
    {
      TIT_PROFILE_SECTION("ParticleAdjacency::search()");
      const auto engine = engine_factory_(positions);
      // -----------------------------------------------------------------------
      fixed_.clear();
      fixed_.reserve(array().size() / 2);
      static std::vector<std::vector<size_t>> adj_vov{};
      adj_vov.resize(array().size());
      par::for_each(array().all(), [&radius_func, &engine, this](PV a) {
        const auto search_point = r[a];
        const auto search_radius = radius_func(a);
        TIT_ASSERT(search_radius > 0.0, "Search radius must be positive.");
        auto& search_results = adj_vov[a.index()];
        search_results.clear();
        engine.search(search_point,
                      search_radius,
                      std::back_inserter(search_results));
        if (fixed[a]) *fixed_.grow_by(1) = a.index();
      });
      adjacency_.clear();
      for (const auto& adj : adj_vov) adjacency_.push_back(adj);
      adjacency_.sort();
      // -----------------------------------------------------------------------
      static std::vector<std::vector<size_t>> interp_adj_vov{};
      interp_adj_vov.resize(fixed_.size());
      par::for_each(_fixed(), [&radius_func, &engine, this](auto ia) {
        auto [i, a] = ia;
        const auto search_point = r[a];
        const auto search_radius = 3 * radius_func(a);
        const auto clipped_point = Domain.clamp(search_point);
        const auto interp_point = 2 * clipped_point - search_point;
        auto& search_results = interp_adj_vov[i];
        search_results.clear();
        engine.search(interp_point,
                      search_radius,
                      std::back_inserter(search_results));
        std::erase_if(search_results, [this](size_t b_index) {
          return fixed[array()[b_index]];
        });
      });
      interp_adjacency_.clear();
      for (const auto& adj : interp_adj_vov) interp_adjacency_.push_back(adj);
      interp_adjacency_.sort();
    }
    // -------------------------------------------------------------------------
    // STEP II: partitioning.
    size_t nparts = par::num_threads();
    const auto parts = array()[parinfo];
    auto partitioner = geom::InertialBisection(positions, parts, nparts);
    // -------------------------------------------------------------------------
    // STEP III: assembly.
    nparts += 1; // since we have the leftover
    block_adjacency_.assemble_wide(nparts, adjacency_.edges(), [&](auto ij) {
      auto [i, j] = ij;
      return parts[i] == parts[j] ? parts[i] : (nparts - 1);
    });
    {
      // Finalize color ranges.
      eprint("NCOL: ");
      for (size_t i = 0; i < block_adjacency_.size(); ++i) {
        eprint("{} ", block_adjacency_[i].size());
      }
      eprint("\n");
    }
  }

  constexpr auto _fixed() const noexcept {
    return std::views::iota(0UZ, fixed_.size()) |
           std::views::transform(
               [&](size_t i) { return std::tuple{i, array()[fixed_[i]]}; });
  }

  /// Adjacent particles.
  constexpr auto operator[](ParticleView<ParticleArray> a) const noexcept {
    TIT_ASSERT(&a.array() != &array(),
               "Particle belongs to a different array.");
    TIT_ASSERT(a.index() < array().size(), "Particle is out of range.");
    return std::views::all(adjacency_[a.index()]) |
           std::views::transform(
               [this](size_t b_index) { return array()[b_index]; });
  }

  /// Adjacent particles.
  constexpr auto operator[](std::nullptr_t, size_t a) const noexcept {
    return std::views::all(interp_adjacency_[a]) |
           std::views::transform(
               [this](size_t b_index) { return array()[b_index]; });
  }

  /// Unique pairs of the adjacent particles.
  constexpr auto pairs() const noexcept {
    return std::views::all(adjacency_.edges()) |
           std::views::transform([this](auto ab_indices) {
             auto [a_index, b_index] = ab_indices;
             return std::tuple{array()[a_index], array()[b_index]};
           });
  }
  constexpr auto block_pairs() const noexcept {
    return std::views::iota(0UZ, block_adjacency_.size()) |
           std::views::transform([&](size_t block_index) {
             return block_adjacency_[block_index] |
                    std::views::transform([this](auto ab_indices) {
                      auto [a_index, b_index] = ab_indices;
                      return std::tuple{array()[a_index], array()[b_index]};
                    });
           });
  }

}; // class ParticleAdjacency

template<class ParticleArray>
ParticleAdjacency(ParticleArray&) -> ParticleAdjacency<ParticleArray>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

template<meta::type Space, meta::type Fields, meta::type Consts = meta::Set<>>
class ParticleArray;

// This template deduction guides ensure that constants are always included
// into a set of fields.
template<class Space, class Fields, class Consts>
  requires meta::is_set_v<Fields> && meta::is_set_v<Consts>
ParticleArray(Space, Fields, Consts)
    -> ParticleArray<Space, decltype(Fields{} | Consts{}), Consts>;
template<class Space, class Fields, class... Consts>
  requires meta::is_set_v<Fields>
ParticleArray(Space, Fields, Consts...)
    -> ParticleArray<Space,
                     decltype(Fields{} | meta::Set<Consts...>{}),
                     meta::Set<Consts...>>;

/// Particle array.
template<class Real, size_t Dim, meta::type... Fields, meta::type... Consts>
class ParticleArray<Space<Real, Dim>,
                    meta::Set<Fields...>,
                    meta::Set<Consts...>> {
public:

  /// Set of particle fields that are present.
  static constexpr auto fields = meta::Set<Fields...>{};

  /// Subset of particle fields that are array-wise constants.
  static constexpr auto constants = meta::Set<Consts...>{};

  /// Subset of particle fields that are individual for each particle.
  static constexpr auto variables = fields - constants;

private:

  using Constants_ = std::tuple<field_value_type_t<Consts, Real, Dim>...>;
  using Particles_ = decltype([]<class... Vars>(meta::Set<Vars...>) {
    return std::tuple<std::vector<field_value_type_t<Vars, Real, Dim>>...>{};
  }(variables));

  Constants_ constants_;
  Particles_ particles_;

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

public:

  /// Construct a particle array.
  /// @{
  // clang-format off
  template<meta::type FieldSubset, meta::type ConstSubset>
    requires meta::is_set_v<FieldSubset> && (fields.includes(FieldSubset{})) &&
             meta::is_set_v<ConstSubset> && (constants.includes(ConstSubset{}))
  constexpr explicit ParticleArray(Space<Real, Dim> /*space*/,
                                   FieldSubset /*fields*/ = {},
                                   ConstSubset /*consts*/ = {}) {}
  // clang-format on
  template<meta::type FieldSubset, meta::type... ConstSubset>
    requires meta::is_set_v<FieldSubset> && (fields.includes(FieldSubset{})) &&
             (constants.includes(meta::Set<ConstSubset...>{}))
  constexpr explicit ParticleArray(Space<Real, Dim> /*space*/,
                                   FieldSubset /*fields*/ = {},
                                   ConstSubset... /*consts*/) {}
  /// @}

  /// Number of particles.
  constexpr auto size() const noexcept -> size_t {
    return std::get<0>(particles_).size();
  }

  /// Reserve amount of particles.
  constexpr void reserve(size_t capacity) {
    std::apply([capacity](auto&... vecs) { ((vecs.reserve(capacity)), ...); },
               particles_);
  }

  /// Appends a new particle.
  constexpr auto append() {
    std::apply([](auto&... vecs) { ((vecs.emplace_back()), ...); }, particles_);
    return (*this)[size() - 1];
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Range of particles.
  /// @{
  constexpr auto all() noexcept {
    return std::views::iota(0UZ, size()) |
           std::views::transform([this](size_t particle_index) {
             return ParticleView{*this, particle_index};
           });
  }
  constexpr auto all() const noexcept {
    return std::views::iota(0UZ, size()) |
           std::views::transform([this](size_t particle_index) {
             return ParticleView{*this, particle_index};
           });
  }
  /// @}

  /// Particle view at index.
  /// @{
  constexpr auto operator[](size_t particle_index) noexcept {
    TIT_ASSERT(particle_index < size(), "Particle index is out of range.");
    return ParticleView{*this, particle_index};
  }
  constexpr auto operator[](size_t particle_index) const noexcept {
    TIT_ASSERT(particle_index < size(), "Particle index is out of range.");
    return ParticleView{*this, particle_index};
  }
  /// @}

  /// Particle field at index.
  /// @{
  template<meta::type Field>
    requires (fields.contains(Field{}))
  constexpr auto operator[]([[maybe_unused]] size_t particle_index,
                            [[maybe_unused]] Field field) noexcept
      -> decltype(auto) {
    TIT_ASSERT(particle_index < size(), "Particle index is out of range.");
    if constexpr (constants.contains(Field{})) {
      return std::get<constants.find(Field{})>(constants_);
    } else {
      return std::get<variables.find(Field{})>(particles_)[particle_index];
    }
  }
  template<meta::type Field>
    requires (fields.contains(Field{}))
  constexpr auto operator[]([[maybe_unused]] size_t particle_index,
                            [[maybe_unused]] Field field) const noexcept
      -> decltype(auto) {
    TIT_ASSERT(particle_index < size(), "Particle index is out of range.");
    if constexpr (constants.contains(Field{})) {
      return std::get<constants.find(Field{})>(constants_);
    } else {
      return std::get<variables.find(Field{})>(particles_)[particle_index];
    }
  }
  /// @}

  /// Get array-wise constant at index or assign value to all particles.
  /// @{
  template<meta::type Field>
    requires (fields.contains(Field{}))
  constexpr auto operator[]([[maybe_unused]] Field field) noexcept
      -> decltype(auto) {
    if constexpr (constants.contains(Field{})) {
      return std::get<constants.find(Field{})>(constants_);
    } else {
      return std::span{std::get<variables.find(Field{})>(particles_)};
    }
  }
  template<meta::type Field>
    requires (constants.contains(Field{}))
  constexpr auto operator[]([[maybe_unused]] Field field) const noexcept
      -> decltype(auto) {
    if constexpr (constants.contains(Field{})) {
      return std::get<constants.find(Field{})>(constants_);
    } else {
      return std::span{std::get<variables.find(Field{})>(particles_)};
    }
  }
  /// @}

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  template<class x>
  static auto _make_name(const std::string& n, std::tuple<x> /**/) {
    return n;
  }
  template<class Realx, size_t Dimx>
  static auto _make_name(const std::string& n,
                         std::tuple<Vec<Realx, Dimx>> /**/) {
    if constexpr (Dimx == 1) return n;
    if constexpr (Dimx == 2) return n + "_x " + n + "_y";
    if constexpr (Dimx == 3) return n + "_x " + n + "_y " + n + "_z";
  }
  template<class Realx, size_t Dimx>
  static auto _make_name(const std::string& n,
                         std::tuple<Mat<Realx, Dimx>> /**/) {
    if constexpr (Dimx == 1) return n;
    if constexpr (Dimx == 2)
      return n + "_xx " + n + "_xy " + //
             n + "_yx " + n + "_yy";
    if constexpr (Dimx == 3)
      return n + "_xx " + n + "_xy " + n + "_xz " + //
             n + "_yx " + n + "_yy " + n + "_yz " + //
             n + "_zx " + n + "_zy " + n + "_zz";
  }

  template<class V>
  static auto _make_name(V /**/) {
    return _make_name(field_name_v<V>,
                      std::tuple<field_value_type_t<V, Real, Dim>>{});
  }

  void print(const std::string& path) {
    std::ofstream output{};
    output.open(path);
    ((output << _make_name(Fields{}) << " "), ...);
    output << '\n';
    for (size_t i = 0; i < size(); ++i) {
      auto a = (*this)[i];
      ((output << Fields{}[a] << " "), ...);
      output << '\n';
    };
    output.flush();
  }
}; // class ParticleArray

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
