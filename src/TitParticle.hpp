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
#include <string_view>
#include <tuple>
#include <vector>

#include <nanoflann.hpp>

#include "tit/utils/meta.hpp"
#include "tit/utils/vec.hpp"

#define TIT_PASS(...) __VA_ARGS__

namespace tit {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Declare a particle variable. */
#define TIT_DEF_VARIABLE(type, name, ...)                                      \
  inline constexpr struct name##_t {                                           \
    /** Variable type. */                                                      \
    template<class Real, dim_t Dim>                                            \
    using var_type = type;                                                     \
    /** Variable name. */                                                      \
    static constexpr const char* var_name = #name;                             \
    /** Variable value for the specified particle. */                          \
    template<has_variable<name##_t> ParticleView>                              \
    constexpr auto& operator[](ParticleView a) const noexcept {                \
      return a->template get<name##_t>();                                      \
    }                                                                          \
    /** Variable value delta for the specified particles. */                   \
    template<has_variable<name##_t> ParticleView>                              \
    constexpr auto operator[](ParticleView a, ParticleView b) const noexcept { \
      return (*this)[a] - (*this)[b];                                          \
    }                                                                          \
  } name __VA_OPT__(, __VA_ARGS__)

/** Declare a scalar particle variable. */
#define TIT_DEF_SCALAR_VARIABLE(name, ...) \
  TIT_DEF_VARIABLE(Real, name __VA_OPT__(, __VA_ARGS__))
/** Declare a scalar particle variable. */
#define TIT_DEF_VECTOR_VARIABLE(name, ...) \
  TIT_DEF_VARIABLE(TIT_PASS(Vec<Real, Dim>), name __VA_OPT__(, __VA_ARGS__))

/** Variable type. */
template<class Var, class Real, dim_t Dim>
using variable_type_t = typename Var::var_type<Real, Dim>;

/** Variable name. */
template<class Var>
inline constexpr auto variable_name_v = Var::var_name;

/** Variables that are present in the particle. */
template<class T>
  requires meta::is_set_v<typename std::remove_cvref_t<
               std::remove_pointer_t<T>>::variables>
using variables_t =
    typename std::remove_cvref_t<std::remove_pointer_t<T>>::variables;
/** Variables that are required by this equation. */
template<class Equation>
  requires meta::is_set_v<typename Equation::required_variables>
using required_variables_t = typename Equation::required_variables;

/** Check that particle has a variable. */
template<class T, class Var>
concept has_variable = variables_t<T>::contains(Var{});

/** Check that particle has a set of variable. */
template<class T, class Vars>
concept has_variables = true;
// meta::is_set_v<Vars> && variables_t<T>::includes(Vars{});

template<class A, class Var>
static consteval bool has(Var) {
  return (has_variable<A, Var>);
}
template<class A, class... Vars>
static consteval bool has(Vars...) {
  return (has_variable<A, Vars> && ...);
}

/** Particle properties accesstors. */
namespace particle_variables {
  /** Particle mass. */
  TIT_DEF_SCALAR_VARIABLE(m);
  /** Particle density. */
  TIT_DEF_SCALAR_VARIABLE(rho);
  /** Particle is fixed? */
  TIT_DEF_VARIABLE(bool, fixed);

  /** Particle width. */
  TIT_DEF_SCALAR_VARIABLE(h);
  /** WHAT AM I??? */
  TIT_DEF_SCALAR_VARIABLE(Omega);

  /** Particle pressure. */
  TIT_DEF_SCALAR_VARIABLE(p);
  /** Particle sound speed. */
  TIT_DEF_SCALAR_VARIABLE(cs);

  /** Particle internal (thermal) energy. */
  TIT_DEF_SCALAR_VARIABLE(eps);
  /** Particle internal (thermal) energy time derivative. */
  TIT_DEF_SCALAR_VARIABLE(deps_dt);

  /** Particle artificial viscosity switch. */
  TIT_DEF_SCALAR_VARIABLE(alpha);
  /** Particle artificial viscosity switch time derivative. */
  TIT_DEF_SCALAR_VARIABLE(dalpha_dt);

  /** Particle position. */
  TIT_DEF_VECTOR_VARIABLE(r);

  /** Particle velocity. */
  TIT_DEF_VECTOR_VARIABLE(v, dr_dt);
  /** Particle velocity divergence. */
  TIT_DEF_SCALAR_VARIABLE(div_v);
  /** Particle velocity curl. */
  TIT_DEF_VARIABLE(TIT_PASS(Vec<Real, 3>), curl_v);

  /** Particle acceleration. */
  TIT_DEF_VECTOR_VARIABLE(a, dv_dt);

} // namespace particle_variables

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

template<class Real, dim_t Dim, class Vars>
class Particle;

template<class Real, dim_t Dim, class... Vars>
class Particle<Real, Dim, meta::Set<Vars...>> :
    public std::tuple<variable_type_t<Vars, Real, Dim>...> {
public:

  using variables = meta::Set<Vars...>;

  template<class Var>
  static consteval bool has(Var var) {
    return has_variable<Particle, Var>;
  }

  template<class Var>
    requires meta::in_list_v<Var, Vars...>
  constexpr auto& get() noexcept {
    return std::get<meta::index_v<Var, Vars...>>(*this);
  }
  template<class Var>
    requires meta::in_list_v<Var, Vars...>
  constexpr const auto& get() const noexcept {
    return std::get<meta::index_v<Var, Vars...>>(*this);
  }

}; // class Particle

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

template<class Real, dim_t Dim, class Vars>
class ParticleArray : public std::vector<Particle<Real, Dim, Vars>> {
public:

  using variables = Vars;

  /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

  template<class Func>
  constexpr void append(const Func& func) {
    func(&this->emplace_back());
  }

  template<class Func>
  constexpr void for_each(const Func& func) {
    std::for_each(this->begin(), this->end(), [&](auto& a) { func(&a); });
  } // namespace tit

  /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

  constexpr void sort()
    requires (Dim == 1)
  {
    std::sort(this->begin(), this->end(), [](const auto& va, const auto& vb) {
      using namespace particle_variables;
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
      using namespace particle_variables;
      auto b = &(*this)[bIndex];
      if (r[a][0] - r[b][0] > search_radius) break;
      func(b);
    }

    // Particle itself.
    func(a);

    // Neighbours to the right.
    for (size_t bIndex = aIndex + 1; bIndex < this->size(); ++bIndex) {
      using namespace particle_variables;
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
      using namespace particle_variables;
      auto a = &(*_array)[idx];
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
    using namespace particle_variables;

    const Real squaredRadius = search_radius * search_radius;
    std::vector<nanoflann::ResultItem<size_t, Real>> indices_dists;
    nanoflann::RadiusResultSet<Real> resultSet(squaredRadius, indices_dists);

    _index->findNeighbors(resultSet, &r[a][0]);

    for (auto& x : indices_dists) {
      size_t bIndex = x.first;
      auto b = &(*this)[bIndex];
      func(b);
    }

    // for_each([&](auto b) {
    //   if (norm(r[a, b]) <= search_radius) func(b);
    // });
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
    return _make_name(variable_name_v<decltype(v)>,
                      meta::Set<variable_type_t<decltype(v), Real, Dim>>{});
  }

  void print(const std::string& path) {
    std::ofstream output(path);
    ([&]<class... TheVars>(meta::Set<TheVars...>) {
      using namespace particle_variables;
      ((output << _make_name(TheVars{}) << " "), ...);
      output << std::endl;
      for (const auto& va : *this) {
        auto a = &va;
        ((output << TheVars{}[a] << " "), ...);
        output << std::endl;
      };
    }(Vars{}));
    output.flush();
  }
}; // struct ParticleArray

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit
