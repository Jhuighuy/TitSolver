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
#include <memory>
#include <ranges>
#include <string_view>
#include <tuple>
#include <vector>

#include <nanoflann.hpp>

#include "tit/sph/field.hpp"
#include "tit/utils/meta.hpp"
#include "tit/utils/misc.hpp"
#include "tit/utils/types.hpp"
#include "tit/utils/vec.hpp"

namespace tit {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** Space specification.
\******************************************************************************/
template<class _Real, dim_t Dim>
  requires (1 <= Dim && Dim <= 3)
class Space {
public:

  /** Real number type, used in this space. */
  using Real = _Real;

  /** Number of spatial dimensions. */
  static constexpr dim_t dim = Dim;

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

  /** Real type that is used for the particles. */
  using Real = typename std::remove_const_t<ParticleArray>::Real;
  /** Number of the spatial dimensions. */
  static constexpr dim_t dim = std::remove_const_t<ParticleArray>::dim;

  /** Set of particle fields that are present. */
  static constexpr auto fields = std::remove_const_t<ParticleArray>::fields;
  /** Subset of particle fields that are array-wise constants. */
  static constexpr auto constants =
      std::remove_const_t<ParticleArray>::constants;
  /** Subset of particle fields that are individual for each particle. */
  static constexpr auto variables =
      std::remove_const_t<ParticleArray>::variables;

  /** Initialize a particle view. */
  constexpr ParticleView(ParticleArray& particles,
                         size_t particle_index) noexcept
      : _particles{&particles}, _particle_index{particle_index} {}

  /** Associated particle array. */
  constexpr ParticleArray& array() const noexcept {
    return *_particles;
  }

  /** Associated particle index. */
  constexpr size_t index() const noexcept {
    return _particle_index;
  }

  /** Particle field value. */
  template<meta::type Field>
    requires (has<ParticleView, Field>())
  constexpr decltype(auto) operator[](Field field) const noexcept {
    return array()[index(), field];
  }

}; // class ParticleView

template<class ParticleArray>
ParticleView(ParticleArray&) -> ParticleView<ParticleArray>;

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

template<meta::type Space, meta::type Fields, meta::type Consts = meta::Set<>>
class ParticleArray;

template<class Space, class Fields, class Consts>
  requires meta::is_set_v<Consts>
ParticleArray(Space, Fields, Consts) -> ParticleArray<Space, Fields, Consts>;
template<class Space, class Fields, class... Consts>
ParticleArray(Space, Fields, Consts...)
    -> ParticleArray<Space, Fields, meta::Set<Consts...>>;

/******************************************************************************\
 ** Particle array.
\******************************************************************************/
template<class _Real, dim_t Dim, meta::type... Fields, meta::type... Consts>
class ParticleArray<Space<_Real, Dim>, //
                    meta::Set<Fields...>, meta::Set<Consts...>> {
public:

  /** Real type that is used for the particles. */
  using Real = _Real;
  /** Number of the spatial dimensions. */
  static constexpr dim_t dim = Dim;

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
  constexpr ParticleArray([[maybe_unused]] Space<Real, Dim> space,
                          [[maybe_unused]] meta::Set<Fields...> fields = {},
                          [[maybe_unused]] Consts... consts) {}
  constexpr ParticleArray([[maybe_unused]] Space<Real, Dim> space,
                          [[maybe_unused]] meta::Set<Fields...> fields = {},
                          [[maybe_unused]] meta::Set<Consts...> consts = {}) {}
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

  /** Iterate through the particles. */
  /** @{ */
  template<class Func>
    requires std::invocable<Func, ParticleView<ParticleArray>>
  constexpr void for_each(const Func& func) {
    std::ranges::for_each( //
        std::views::iota(size_t{0}, size()), [&](size_t particle_index) {
          return func(ParticleView{*this, particle_index});
        });
  }
  template<class Func>
    requires std::invocable<Func, ParticleView</*const*/ ParticleArray>>
  constexpr void for_each(const Func& func) const {
    std::ranges::for_each( //
        std::views::iota(size_t{0}, size()), [&](size_t particle_index) {
          return func(ParticleView{*this, particle_index});
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
  constexpr decltype(auto) operator[]([[maybe_unused]] size_t particle_index,
                                      [[maybe_unused]] Field field) noexcept {
    TIT_ASSERT(particle_index < size(), "Particle index is out of range.");
    if constexpr (meta::contains_v<Field, Consts...>) {
      return auto((*this)[field]); // Return a copy for constants.
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
  constexpr decltype(auto) operator[]([[maybe_unused]] Field field) noexcept {
    if constexpr (meta::contains_v<Field, Consts...>) {
      return std::get<meta::index_of_v<Field, Consts...>>(_constants);
    } else {
      return OnAssignment{[&](auto value) {
        for_each([&](ParticleView<ParticleArray> a) { a[field] = value; });
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

  constexpr void sort()
    requires (Dim == 1)
  {
    std::sort(this->begin(), this->end(), [](const auto& va, const auto& vb) {
      using namespace particle_fields;
      return r[&va][0] < r[&vb][0];
    });
  }

  template<class Func>
  constexpr void nearby(auto a, Real search_radius, const Func& func)
    requires (Dim == 1)
  {
    const size_t aIndex = a - this->data();

    // Neighbours to the left.
    for (size_t bIndex = aIndex - 1; bIndex != SIZE_MAX; --bIndex) {
      using namespace particle_fields;
      auto b = &(*this)[bIndex];
      if (r[a][0] - r[b][0] > search_radius) break;
      func(b);
    }

    // Particle itself.
    func(a);

    // Neighbours to the right.
    for (size_t bIndex = aIndex + 1; bIndex < this->size(); ++bIndex) {
      using namespace particle_fields;
      auto b = &(*this)[bIndex];
      if (r[b][0] - r[a][0] > search_radius) break;
      func(b);
    }
  }

  /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

  struct NanoflannAdapter {
    ParticleArray* _array;
    using coord_t = Real;
    constexpr size_t kdtree_get_point_count() const {
      return _array->size();
    }
    constexpr Real kdtree_get_pt(size_t idx, size_t dim) const noexcept {
      using namespace particle_fields;
      auto a = (*_array)[idx];
      return r[a][dim];
    }
    constexpr bool kdtree_get_bbox([[maybe_unused]] auto&& bbox) const {
      return false;
    }
  }; // class NanoflannAdapter

  using my_kd_tree_t = nanoflann::KDTreeSingleIndexAdaptor<
      nanoflann::L2_Simple_Adaptor<Real, NanoflannAdapter>, NanoflannAdapter,
      Dim>;

  std::unique_ptr<NanoflannAdapter> _adapter;
  std::unique_ptr<my_kd_tree_t> _index;

  constexpr void sort()
    requires (Dim > 1)
  {
    _adapter.reset(new NanoflannAdapter{this});
    _index.reset(new my_kd_tree_t(Dim, *_adapter, {10 /* max leaf */}));
  }

  template<class Func>
  constexpr void nearby(auto a, Real search_radius, const Func& func)
    requires (Dim > 1)
  {
    using namespace particle_fields;

    const Real squaredRadius = search_radius * search_radius;
    std::vector<nanoflann::ResultItem<size_t, Real>> indices_dists;
    nanoflann::RadiusResultSet<Real> resultSet(squaredRadius, indices_dists);

    // resultSet contains a;
    _index->findNeighbors(resultSet, &r[a][0]);

    for (auto& x : indices_dists) {
      size_t bIndex = x.first;
      auto b = (*this)[bIndex];
      func(b);
    }
  }

  /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

  template<class x>
  static auto _make_name(std::string n, meta::Set<x>) {
    return n;
  }
  template<class Realx, dim_t Dimx>
  static auto _make_name(std::string n, meta::Set<Vec<Realx, Dimx>>) {
    if constexpr (Dimx == 1) return n;
    if constexpr (Dimx == 2) return n + "_x " + n + "_y";
    if constexpr (Dimx == 3) return n + "_x " + n + "_y " + n + "_z";
  }

  static auto _make_name(auto v) {
    return _make_name(field_name_v<decltype(v)>,
                      meta::Set<field_value_type_t<decltype(v), Real, Dim>>{});
  }

  void print(const std::string& path) {
    std::ofstream output(path);
    using namespace particle_fields;
    ((output << _make_name(Fields{}) << " "), ...);
    output << std::endl;
    for (size_t i = 0; i < size(); ++i) {
      auto a = (*this)[i];
      ((output << Fields{}[a] << " "), ...);
      output << std::endl;
    };
    output.flush();
  }

}; // struct ParticleArray

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit
