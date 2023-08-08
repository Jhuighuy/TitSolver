/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Copyright (C) 2020-2023 Oleg Butakov                                       *
 *                                                                            *
 * Permission is hereby granted, free of charge, to any person obtaining a    *
 * copy of this software and associated documentation files (the "Software"), *
 * to deal in the Software without restriction, including without limitation  *
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,   *
 * and/or sell copies of the Software, and to permit persons to whom the      *
 * Software is furnished to do so, subject to the following conditions:       *
 *                                                                            *
 * The above copyright notice and this permission notice shall be included in *
 * all copies or substantial portions of the Software.                        *
 *                                                                            *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR *
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   *
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    *
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER *
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    *
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        *
 * DEALINGS IN THE SOFTWARE.                                                  *
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <cstddef>
#include <fstream>
#include <iterator>
#include <ranges>
#include <span> // IWUY wants me to include it..
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>

#include "tit/core/assert.hpp"
#include "tit/core/bbox.hpp"
#include "tit/core/graph.hpp"
#include "tit/core/mat.hpp"
#include "tit/core/meta.hpp"
#include "tit/core/misc.hpp"
#include "tit/core/search_engine.hpp"
#include "tit/core/types.hpp"
#include "tit/core/vec.hpp"
#include "tit/sph/field.hpp"

namespace tit {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** Space specification.
\******************************************************************************/
template<class _Real, size_t Dim>
  requires (1 <= Dim && Dim <= 3)
class Space {
public:

  /** Real number type, used in this space. */
  using Real = _Real;

  /** Number of spatial dimensions. */
  static constexpr size_t dim = Dim;

}; // class Space

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** Particle view.
\******************************************************************************/
template<class ParticleArray>
  requires std::is_object_v<ParticleArray>
class ParticleView final {
private:

  ParticleArray* _particles;
  size_t _particle_index;

public:

  /** Set of particle fields that are present. */
  static constexpr auto fields = std::remove_const_t<ParticleArray>::fields;
  /** Subset of particle fields that are array-wise constants. */
  static constexpr auto constants =
      std::remove_const_t<ParticleArray>::constants;
  /** Subset of particle fields that are individual for each particle. */
  static constexpr auto variables =
      std::remove_const_t<ParticleArray>::variables;

  /** Construct a particle view. */
  constexpr ParticleView(ParticleArray& particles,
                         size_t particle_index) noexcept
      : _particles{&particles}, _particle_index{particle_index} {}

  /** Associated particle array. */
  constexpr ParticleArray& array() const noexcept {
    TIT_ASSERT(_particles != nullptr, "Particle array was not set.");
    return *_particles;
  }
  /** Associated particle index. */
  constexpr auto index() const noexcept -> size_t {
    return _particle_index;
  }

  /** Compare particle views. */
  /** @{ */
  constexpr auto operator==(ParticleView<ParticleArray> other) const noexcept {
    TIT_ASSERT(&array() == &other.array(),
               "Particles must belong to the same array.");
    return index() == other.index();
  }
  constexpr auto operator!=(ParticleView<ParticleArray> other) const noexcept {
    TIT_ASSERT(&array() == &other.array(),
               "Particles must belong to the same array.");
    return index() != other.index();
  }
  /** @} */

  /** Particle field value. */
  template<meta::type Field>
    requires (has<ParticleView, Field>())
  constexpr auto operator[](Field field) const noexcept -> decltype(auto) {
    return array()[index(), field];
  }

}; // class ParticleView

template<class ParticleArray>
ParticleView(ParticleArray&) -> ParticleView<ParticleArray>;

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

// TODO: move it to an appropriate place!
#if HARD_DAM_BREAKING
inline constexpr auto Domain = BBox{Vec{0.0, 0.0}, Vec{4.0, 3.0}};
#elif EASY_DAM_BREAKING
inline constexpr auto Domain = BBox{Vec{0.0, 0.0}, Vec{3.2196, 1.5}};
#else
inline constexpr auto Domain = BBox{Vec{0.0, 0.0}, Vec{0.0, 0.0}};
#endif

/******************************************************************************\
 ** Particle adjacency graph.
\******************************************************************************/
template<class ParticleArray, class EngineFactory = KDTreeFactory>
  requires std::is_object_v<ParticleArray> && std::is_object_v<EngineFactory>
class ParticleAdjacency final {
private:

  ParticleArray* _particles;
  EngineFactory _engine_factory;
  Graph _adjacency;
  Graph _interp_adjacency;

public:

  /** Construct a particle adjacency graph.
   ** @param engine_factory Nearest-neighbours search engine factory. */
  constexpr ParticleAdjacency(ParticleArray& particles,
                              EngineFactory engine_factory = {}) noexcept
      : _particles{&particles}, _engine_factory{std::move(engine_factory)} {}

  /** Associated particle array. */
  constexpr ParticleArray& array() const noexcept {
    TIT_ASSERT(_particles != nullptr, "Particle array was not set.");
    return *_particles;
  }

  /** Build an adjacency graph.
   ** @param radius_func Function that returns search radius for the
   **                    specified particle view. */
  template<class SearchRadiusFunc>
  constexpr void build(const SearchRadiusFunc& radius_func) noexcept {
    using PV = ParticleView<ParticleArray>;
    auto positions = array().views() | //
                     std::views::transform([](PV a) { return a[r]; });
    const auto engine = _engine_factory(std::move(positions));
    _adjacency.clear();
    _interp_adjacency.clear();
    std::ranges::for_each(array().views(), [&](PV a) {
      const auto search_point = r[a];
      const auto search_radius = radius_func(a);
      TIT_ASSERT(search_radius > 0.0, "Search radius must be positive.");
      thread_local std::vector<size_t> search_results;
      search_results.clear();
      engine.search(search_point, search_radius,
                    std::back_inserter(search_results));
      _adjacency.append_row(search_results);
#if 1
      if (fixed[a]) {
        const auto clipped_point = Domain.clip(search_point);
        const auto interp_point = 2 * clipped_point - search_point;
        search_results.clear();
        engine.search(interp_point, 3 * search_radius,
                      std::back_inserter(search_results));
        _interp_adjacency.append_row(search_results);
      }
#endif
    });
  }

  /** Adjacent particles. */
  constexpr auto operator[](ParticleView<ParticleArray> a) const noexcept {
    TIT_ASSERT(&a.array() != &array(),
               "Particle belongs to a different array.");
    TIT_ASSERT(a.index() < array().size(), "Particle is out of range.");
    return std::views::all(_adjacency[a.index()]) |
           std::views::transform([this](size_t b_index) {
             TIT_ASSERT(b_index < array().size(), "Particle is out of range.");
             return array()[b_index];
           });
  }
  /** Adjacent particles. */
  constexpr auto operator[](std::nullptr_t, size_t a) const noexcept {
    return std::views::all(_interp_adjacency[a]) |
           std::views::transform([this](size_t b_index) {
             TIT_ASSERT(b_index < array().size(), "Particle is out of range.");
             return array()[b_index];
           });
  }

  /** Pairs of the adjacent particles. */
  constexpr auto pairs() const noexcept {
    return std::views::all(_adjacency.edges()) |
           std::views::transform([this](auto ab_indices) {
             auto [a_index, b_index] = ab_indices;
             TIT_ASSERT(a_index < array().size(), "Particle is out of range.");
             TIT_ASSERT(b_index < array().size(), "Particle is out of range.");
             return std::tuple{array()[a_index], array()[b_index]};
           });
  }

}; // class ParticleAdjacency

template<class ParticleArray>
ParticleAdjacency(ParticleArray&) -> ParticleAdjacency<ParticleArray>;

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

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
    -> ParticleArray<Space, decltype(Fields{} | meta::Set<Consts...>{}),
                     meta::Set<Consts...>>;

/******************************************************************************\
 ** Particle array.
\******************************************************************************/
template<class Real, size_t Dim, meta::type... Fields, meta::type... Consts>
class ParticleArray<Space<Real, Dim>, //
                    meta::Set<Fields...>, meta::Set<Consts...>> {
public:

  /** Set of particle fields that are present. */
  static constexpr auto fields = meta::Set<Fields...>{};
  /** Subset of particle fields that are array-wise constants. */
  static constexpr auto constants = meta::Set<Consts...>{};
  /** Subset of particle fields that are individual for each particle. */
  static constexpr auto variables = fields - constants;

private:

  using _Constants = std::tuple<field_value_type_t<Consts, Real, Dim>...>;
  using _Particle = decltype([]<class... Vars>(meta::Set<Vars...>) {
    return std::tuple<field_value_type_t<Vars, Real, Dim>...>{};
  }(variables));

  _Constants _constants;
  std::vector<_Particle> _particles;

  /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

public:

  /** Construct a particle array. */
  /** @{ */
  // clang-format off
  template<meta::type FieldSubset, meta::type ConstSubset>
    requires meta::is_set_v<FieldSubset> && (fields.includes(FieldSubset{})) &&
             meta::is_set_v<ConstSubset> && (constants.includes(FieldSubset{}))
  constexpr ParticleArray([[maybe_unused]] Space<Real, Dim> space,
                          [[maybe_unused]] FieldSubset fields = {},
                          [[maybe_unused]] ConstSubset consts = {}) {}
  // clang-format on
  template<meta::type FieldSubset, meta::type... ConstSubset>
    requires meta::is_set_v<FieldSubset> && (fields.includes(FieldSubset{})) &&
             (constants.includes(meta::Set<ConstSubset...>{}))
  constexpr ParticleArray([[maybe_unused]] Space<Real, Dim> space,
                          [[maybe_unused]] FieldSubset fields = {},
                          [[maybe_unused]] ConstSubset... consts) {}
  /** @} */

  /** Number of particles. */
  constexpr size_t size() const noexcept {
    return _particles.size();
  }

  /** Reserve amount of particles. */
  constexpr void reserve(size_t capacity) {
    _particles.reserve(capacity);
  }

  /** Appends a new particle. */
  constexpr auto append() {
    _particles.emplace_back();
    return (*this)[size() - 1];
  }

  /** Range of particles. */
  /** @{ */
  constexpr auto views() noexcept {
    return std::views::iota(size_t{0}, size()) |
           std::views::transform([this](size_t particle_index) {
             return ParticleView{*this, particle_index};
           });
  }
  constexpr auto views() const noexcept {
    return std::views::iota(size_t{0}, size()) |
           std::views::transform([this](size_t particle_index) {
             return ParticleView{*this, particle_index};
           });
  }
  /** @} */

  /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

  /** Particle view at index. */
  /** @{ */
  constexpr auto operator[](size_t particle_index) noexcept {
    TIT_ASSERT(particle_index < size(), "Particle index is out of range.");
    return ParticleView{*this, particle_index};
  }
  constexpr auto operator[](size_t particle_index) const noexcept {
    TIT_ASSERT(particle_index < size(), "Particle index is out of range.");
    return ParticleView{*this, particle_index};
  }

  /** Particle field at index. */
  /** @{ */
  template<meta::type Field>
    requires meta::contains_v<Field, Fields...>
  constexpr auto operator[]([[maybe_unused]] size_t particle_index,
                            [[maybe_unused]] Field field) noexcept
      -> decltype(auto) {
    TIT_ASSERT(particle_index < size(), "Particle index is out of range.");
    if constexpr (meta::contains_v<Field, Consts...>) {
      return auto{(*this)[field]}; // Return a copy for constants.
    } else {
      return [&]<class... Vars>(meta::Set<Vars...>) -> decltype(auto) {
        auto& particle = _particles[particle_index];
        return std::get<meta::index_of_v<Field, Vars...>>(particle);
      }(variables);
    }
  }
  template<meta::type Field>
    requires meta::contains_v<Field, Fields...>
  constexpr auto operator[]([[maybe_unused]] size_t particle_index,
                            [[maybe_unused]] Field field) const noexcept {
    TIT_ASSERT(particle_index < size(), "Particle index is out of range.");
    if constexpr (meta::contains_v<Field, Consts...>) {
      return (*this)[field];
    } else {
      return [&]<class... Vars>(meta::Set<Vars...>) {
        const auto& particle = _particles[particle_index];
        return std::get<meta::index_of_v<Field, Vars...>>(particle);
      }(variables);
    }
  }
  /** @} */

  /** Get array-wise constant at index or assign value to all particles. */
  /** @{ */
  template<meta::type Field>
    requires meta::contains_v<Field, Fields...>
  constexpr auto operator[]([[maybe_unused]] Field field) noexcept
      -> decltype(auto) {
    if constexpr (meta::contains_v<Field, Consts...>) {
      return std::get<meta::index_of_v<Field, Consts...>>(_constants);
    } else {
      return OnAssignment{[&](auto value) {
        std::ranges::for_each(views(), [&](ParticleView<ParticleArray> a) { //
          a[field] = value;
        });
      }};
    }
  }
  template<meta::type Field>
    requires meta::contains_v<Field, Consts...>
  constexpr auto operator[]([[maybe_unused]] Field field) const noexcept {
    return std::get<meta::index_of_v<Field, Consts...>>(_constants);
  }
  /** @} */

  /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

  template<class x>
  static auto _make_name(std::string n, meta::Set<x>) {
    return n;
  }
  template<class Realx, size_t Dimx>
  static auto _make_name(std::string n, meta::Set<Vec<Realx, Dimx>>) {
    if constexpr (Dimx == 1) return n;
    if constexpr (Dimx == 2) return n + "_x " + n + "_y";
    if constexpr (Dimx == 3) return n + "_x " + n + "_y " + n + "_z";
  }
  template<class Realx, size_t Dimx>
  static auto _make_name(std::string n, meta::Set<Mat<Realx, Dimx>>) {
    if constexpr (Dimx == 1) return n;
    if constexpr (Dimx == 2)
      return n + "_xx " + n + "_xy " + //
             n + "_yx " + n + "_yy";
    if constexpr (Dimx == 3)
      return n + "_xx " + n + "_xy " + n + "_xz " + //
             n + "_yx " + n + "_yy " + n + "_yz " + //
             n + "_zx " + n + "_zy " + n + "_zz";
  }

  static auto _make_name(auto v) {
    return _make_name(field_name_v<decltype(v)>,
                      meta::Set<field_value_type_t<decltype(v), Real, Dim>>{});
  }

  void print(const std::string& path) {
    std::ofstream output(path);
    ((output << _make_name(Fields{}) << " "), ...);
    output << std::endl;
    for (size_t i = 0; i < size(); ++i) {
      auto a = (*this)[i];
      ((output << Fields{}[a] << " "), ...);
      output << std::endl;
    };
    output.flush();
  }

}; // class ParticleArray

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit
