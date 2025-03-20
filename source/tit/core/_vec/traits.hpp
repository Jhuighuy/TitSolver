/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// IWYU pragma: private, include "tit/core/vec.hpp"
#pragma once

#include <type_traits>

// Boost.Geometry is heavy-weight, so we only include the parts we need.
#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/coordinate_dimension.hpp>
#include <boost/geometry/core/coordinate_system.hpp>
#include <boost/geometry/core/coordinate_type.hpp>
#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/core/tag.hpp>
#include <boost/geometry/core/tags.hpp>

#include "tit/core/_vec/vec.hpp"
#include "tit/core/basic_types.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace impl {
template<class>
inline constexpr bool is_vec_v = false;
template<class Num, size_t Dim>
inline constexpr bool is_vec_v<Vec<Num, Dim>> = true;
} // namespace impl

/// Is the type a specialization of a vector type?
template<class T>
inline constexpr bool is_vec_v = impl::is_vec_v<T>;

/// Number type of the vector.
template<class Vec>
  requires is_vec_v<Vec>
using vec_num_t = std::remove_cvref_t<decltype(Vec{}[0])>;

/// Dimensionality of the vector.
template<class Vec>
  requires is_vec_v<Vec>
inline constexpr size_t vec_dim_v = Vec{}.dim();

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit

namespace bgt = boost::geometry::traits;

template<class Num, tit::size_t Dim>
struct bgt::tag<tit::Vec<Num, Dim>> final : std::type_identity<point_tag> {};

template<class Num, tit::size_t Dim>
struct bgt::dimension<tit::Vec<Num, Dim>> final :
    std::integral_constant<tit::size_t, Dim> {};

template<class Num, tit::size_t Dim>
struct bgt::coordinate_type<tit::Vec<Num, Dim>> final :
    std::type_identity<Num> {};

template<class Num, tit::size_t Dim>
struct bgt::coordinate_system<tit::Vec<Num, Dim>> final :
    std::type_identity<cs::cartesian> {};

template<class Num, tit::size_t Dim, tit::size_t Index>
struct bgt::access<tit::Vec<Num, Dim>, Index> {
  static_assert(Index < Dim, "Index is out of range!");
  constexpr static auto get(const tit::Vec<Num, Dim>& a) noexcept -> Num {
    return a[Index];
  }
  constexpr static void set(tit::Vec<Num, Dim>& a, Num v) noexcept {
    a[Index] = std::move(v);
  }
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
