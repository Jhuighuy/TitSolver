/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

#include "tit/core/mat.hpp"
#include "tit/core/math.hpp"
#include "tit/core/type.hpp"
#include "tit/core/vec.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace impl {

template<class PV>
concept with_fields_ =
    requires { std::remove_cvref_t<PV>::fields; } && //
    is_type_set_v<decltype(auto(std::remove_cvref_t<PV>::fields))>;

template<class PV, class Field>
concept has_field_ = with_fields_<PV> && empty_type<Field> &&
                     std::remove_cvref_t<PV>::fields.contains(Field{});

} // namespace impl

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Base field specification.
class BaseField {
public:

  /// Field value for the specified particle view.
  template<class Self, impl::has_field_<Self> PV>
  constexpr auto operator[](this const Self& self, PV&& a) noexcept
      -> decltype(auto) {
    return std::forward<PV>(a)[self];
  }

  /// Field value delta for the specified particle view.
  template<class Self, impl::has_field_<Self> PVa, impl::has_field_<Self> PVb>
  constexpr auto operator[](this const Self& self, PVa&& a, PVb&& b) noexcept {
    return std::forward<PVa>(a)[self] - std::forward<PVb>(b)[self];
  }

  /// Average of the field values over the specified particle views.
  /// @{
  template<class Self, impl::has_field_<Self>... PVs>
  constexpr auto avg(this const Self& self, PVs&&... ai) {
    return tit::avg(std::forward<PVs>(ai)[self]...);
  }
  template<class Self, impl::has_field_<Self>... PVs>
  constexpr auto operator[](this const Self& self, std::tuple<PVs...> a) {
    return std::apply([&self](PVs... as) { return self.avg(as...); }, a);
  }
  /// @}

  /// Field value delta for the specified particle view and average of tuple.
  /// @{
  template<class Self,
           impl::has_field_<Self> PVa,
           impl::has_field_<Self>... PVs>
  constexpr auto operator[](this const Self& self,
                            PVa&& a,
                            std::tuple<PVs...> b) {
    return std::forward<PVa>(a)[self] -
           std::apply([&self](PVs... bs) { return self.avg(bs...); }, b);
  }
  template<class Self,
           impl::has_field_<Self> PVb,
           impl::has_field_<Self>... PVs>
  constexpr auto operator[](this const Self& self,
                            std::tuple<PVs...> a,
                            PVb&& b) {
    return std::apply([&self](PVs... as) { return self.avg(as...); }, a) -
           std::forward<PVb>(b)[self];
  }
  /// @}

}; // class BaseField

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Field specification type.
template<class Field>
concept field = empty_type<Field> && std::derived_from<Field, BaseField>;

namespace impl {
template<class FieldSet>
inline constexpr bool is_field_set_v = false;
template<field... Fields>
inline constexpr bool is_field_set_v<TypeSet<Fields...>> = true;
} // namespace impl

/// Field set specification type.
template<class FieldSet>
concept field_set = impl::is_field_set_v<FieldSet>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Declare a particle field.
#define TIT_DEFINE_FIELD(name, ...)                                            \
  class name##_t final : public BaseField {                                    \
  public:                                                                      \
                                                                               \
    /** Field name. */                                                         \
    static constexpr std::string_view field_name = #name;                      \
                                                                               \
    /** Field type. */                                                         \
    template<class Real, size_t Dim>                                           \
      requires (std::same_as<__VA_ARGS__, Real> ||                             \
                std::same_as<__VA_ARGS__, Vec<Real, Dim>> ||                   \
                std::same_as<__VA_ARGS__, Mat<Real, Dim>>)                     \
    using field_value_type = __VA_ARGS__;                                      \
                                                                               \
  }; /* class name##_t */                                                      \
  inline constexpr name##_t name

/// Declare a scalar particle field.
#define TIT_DEFINE_SCALAR_FIELD(name) TIT_DEFINE_FIELD(name, Real)

/// Declare a vector particle field.
#define TIT_DEFINE_VECTOR_FIELD(name) TIT_DEFINE_FIELD(name, Vec<Real, Dim>)

/// Declare a matrix particle field.
#define TIT_DEFINE_MATRIX_FIELD(name) TIT_DEFINE_FIELD(name, Mat<Real, Dim>)

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Space specification.
/// @todo We shall think more about the concept of space.
template<class Num, std::size_t Dim>
struct Space {};

namespace impl {
template<class T>
struct is_space : std::false_type {};
template<class Num, std::size_t Dim>
struct is_space<Space<Num, Dim>> : std::true_type {};
} // namespace impl

/// Space type.
template<class S>
concept space = impl::is_space<S>::value;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

template<empty_type Field, class Space>
struct field_value;

template<empty_type Field, class Real, std::size_t Dim>
struct field_value<Field, Space<Real, Dim>> {
  using type = std::remove_cvref_t<Field>::template field_value_type<Real, Dim>;
};

/// Field value type.
template<empty_type Field, class Space>
using field_value_t = field_value<Field, Space>::type;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Particle width.
TIT_DEFINE_SCALAR_FIELD(h);
/// Particle mass.
TIT_DEFINE_SCALAR_FIELD(m);

namespace sph {
/// Particle position.
TIT_DEFINE_VECTOR_FIELD(r);
} // namespace sph

/// Particle velocity.
TIT_DEFINE_VECTOR_FIELD(v);
/// Particle acceleration.
TIT_DEFINE_VECTOR_FIELD(dv_dt);
/// Particle velocity gradient.
TIT_DEFINE_MATRIX_FIELD(grad_v);

/// Particle semi-analytical volume correction.
TIT_DEFINE_SCALAR_FIELD(gamma);
/// Particle semi-analytical volume correction gradient.
TIT_DEFINE_VECTOR_FIELD(grad_gamma);

/// Particle density.
TIT_DEFINE_SCALAR_FIELD(rho);
/// Particle density gradient.
TIT_DEFINE_VECTOR_FIELD(grad_rho);
/// Particle density time derivative.
TIT_DEFINE_SCALAR_FIELD(drho_dt);

/// Particle pressure.
TIT_DEFINE_SCALAR_FIELD(p);
/// Particle sound speed.
TIT_DEFINE_SCALAR_FIELD(cs);

/// Particle normal vector.
TIT_DEFINE_VECTOR_FIELD(N);
/// Particle renormalization matrix.
TIT_DEFINE_MATRIX_FIELD(L);
namespace sph {
/// Particle free surface indicator.
TIT_DEFINE_SCALAR_FIELD(phi);
} // namespace sph
/// Particle shift.
TIT_DEFINE_VECTOR_FIELD(dr);

/// Scratch field for free surface correction.
TIT_DEFINE_SCALAR_FIELD(rho_raw);

/// Particle global identifier.
///
/// Unlike the physical fields, the global identifier is a solver-internal
/// field: it provides stable particle identity that survives reordering and,
/// in distributed runs, ownership migration between processes.
class gid_t final : public BaseField {
public:

  /** Field name. */
  static constexpr std::string_view field_name = "gid";

  /** Field type. */
  template<class Real, size_t Dim>
  using field_value_type = std::uint64_t;

}; // class gid_t

/// @copydoc gid_t
inline constexpr gid_t gid;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
