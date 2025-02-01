/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <string_view>
#include <type_traits>
#include <utility>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/mat.hpp"
#include "tit/core/math.hpp"
#include "tit/core/meta.hpp"
#include "tit/core/utils.hpp"
#include "tit/core/vec.hpp"

#include "tit/data/type.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace impl {

template<class PV>
concept with_fields_ =
    requires { std::remove_cvref_t<PV>::fields; } && //
    meta::is_set_v<decltype(auto(std::remove_cvref_t<PV>::fields))>;

template<class PV, class Field>
concept has_field_ = with_fields_<PV> && meta::type<Field> &&
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

  /// Field value for the specified particle view or default value.
  template<class Self, impl::with_fields_ PV, class Default>
  constexpr auto get(this const Self& self,
                     PV&& a,
                     Default&& default_val) noexcept -> decltype(auto) {
    if constexpr (impl::has_field_<PV, Self>) return std::forward<PV>(a)[self];
    else return std::forward<Default>(default_val);
  }

  /// Field value delta for the specified particle view.
  template<class Self, impl::has_field_<Self> PVa, impl::has_field_<Self> PVb>
  constexpr auto operator[](this const Self& self, PVa&& a, PVb&& b) noexcept {
    return std::forward<PVa>(a)[self] - std::forward<PVb>(b)[self];
  }

  /// Average of the field values over the specified particle views.
  template<class Self, impl::has_field_<Self>... PVs>
  constexpr auto avg(this const Self& self, PVs&&... ai) {
    return tit::avg(std::forward<PVs>(ai)[self]...);
  }

  /// Harmonic average of the field values over the specified particle views.
  template<class Self, impl::has_field_<Self>... PVs>
  constexpr auto havg(this const Self& self, PVs&&... ai) {
    return tit::havg(std::forward<PVs>(ai)[self]...);
  }

}; // class BaseField

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Field specification type.
template<class Field>
concept field = meta::type<Field> && std::derived_from<Field, BaseField>;

namespace impl {
template<class FieldSet>
inline constexpr bool is_field_set_v = false;
template<field... Fields>
inline constexpr bool is_field_set_v<meta::Set<Fields...>> = true;
} // namespace impl

/// Field set specification type.
template<class FieldSet>
concept field_set = impl::is_field_set_v<FieldSet>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Declare a particle field.
#define TIT_DEFINE_FIELD(type, name, ...)                                      \
  class name##_t final : public BaseField {                                    \
  public:                                                                      \
                                                                               \
    /** Field name. */                                                         \
    static constexpr std::string_view field_name = #name;                      \
                                                                               \
    /** Field type. */                                                         \
    template<class Real, size_t Dim>                                           \
    using field_value_type = type;                                             \
                                                                               \
  }; /* class name##_t */                                                      \
  inline constexpr name##_t name __VA_OPT__(, __VA_ARGS__);

/// Declare a scalar particle field.
#define TIT_DEFINE_SCALAR_FIELD(name, ...)                                     \
  TIT_DEFINE_FIELD(Real, name __VA_OPT__(, __VA_ARGS__))

/// Declare a vector particle field.
#define TIT_DEFINE_VECTOR_FIELD(name, ...)                                     \
  TIT_DEFINE_FIELD(TIT_PASS(Vec<Real, Dim>), name __VA_OPT__(, __VA_ARGS__))

/// Declare a matrix particle field.
#define TIT_DEFINE_MATRIX_FIELD(name, ...)                                     \
  TIT_DEFINE_FIELD(TIT_PASS(Mat<Real, Dim>), name __VA_OPT__(, __VA_ARGS__))

/// Field name.
template<meta::type Field>
inline constexpr auto field_name_v = std::remove_cvref_t<Field>::field_name;

/// Space specification.
/// @todo We shall think more about the concept of space.
template<class Num, size_t Dim>
struct Space {};

namespace impl {
template<class T>
struct is_space : std::false_type {};
template<class Num, size_t Dim>
struct is_space<Space<Num, Dim>> : std::true_type {};
} // namespace impl

/// Space type.
template<class S>
concept space = impl::is_space<S>::value;

template<meta::type Field, class Space>
struct field_value;

template<meta::type Field, class Real, size_t Dim>
struct field_value<Field, Space<Real, Dim>> {
  using type =
      typename std::remove_cvref_t<Field>::template field_value_type<Real, Dim>;
};

/// Field value type.
template<meta::type Field, class Space>
using field_value_t = typename field_value<Field, Space>::type;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Particle partition index.
using PartIndex = uint8_t;

/// Particle multilevel partition index.
class PartVec final {
public:

  /// Number of partition levels.
  static constexpr size_t MaxNumLevels = 8;

  /// Construct a multilevel partition index.
  /// @{
  constexpr PartVec() = default;
  constexpr explicit PartVec(PartIndex part) noexcept : vec_(part) {}
  /// @}

  /// Get the partition index at the specified level.
  constexpr auto operator[](this auto&& self, size_t i) noexcept -> auto&& {
    TIT_ASSERT(i < MaxNumLevels, "Level index is out of range!");
    return std::forward_like<decltype(self)>(self.vec_[i]);
  }

  /// Find the last assigned partition index.
  constexpr auto last() const noexcept -> PartIndex {
    for (ssize_t i = (MaxNumLevels - 2); i >= 0; --i) {
      if (vec_[i] != vec_[i + 1]) return vec_[i];
    }
    return vec_[0];
  }

  /// Find the first common partition index.
  static constexpr auto common(const PartVec& a, const PartVec& b) noexcept
      -> PartIndex {
    const auto level = find_true(a.vec_ == b.vec_);
    TIT_ASSERT(level >= 0, "No common partition index!");
    return a[level];
  }

private:

  Vec<PartIndex, MaxNumLevels> vec_;

}; // class PartVec

template<>
inline constexpr auto data::type_of<PartVec> = data::type_of<uint64_t>;

template<class Stream>
constexpr void serialize(Stream& out, const PartVec& pvec) {
  serialize(out, static_cast<uint64_t>(pvec.last()));
}

/// Particle partition information.
TIT_DEFINE_FIELD(PartVec, parinfo)

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace sph {
/// Particle position.
TIT_DEFINE_VECTOR_FIELD(r)
} // namespace sph
TIT_DEFINE_VECTOR_FIELD(dr)

/// Particle velocity.
TIT_DEFINE_VECTOR_FIELD(v)
/// Particle acceleration.
TIT_DEFINE_VECTOR_FIELD(dv_dt)
/// Particle velocity gradient.
TIT_DEFINE_MATRIX_FIELD(grad_v)
/// Particle velocity divergence.
TIT_DEFINE_SCALAR_FIELD(div_v)
/// Particle velocity curl (always 3D).
TIT_DEFINE_FIELD(TIT_PASS(Vec<Real, 3>), curl_v)

/// Particle mass.
TIT_DEFINE_SCALAR_FIELD(m)
/// Particle density.
TIT_DEFINE_SCALAR_FIELD(rho)
/// Particle density gradient.
TIT_DEFINE_VECTOR_FIELD(grad_rho)
/// Particle density time derivative.
TIT_DEFINE_SCALAR_FIELD(drho_dt)

/// Particle width.
TIT_DEFINE_SCALAR_FIELD(h)

/// Particle pressure.
TIT_DEFINE_SCALAR_FIELD(p)
/// Particle sound speed.
TIT_DEFINE_SCALAR_FIELD(cs)

/// Particle thermal energy.
TIT_DEFINE_SCALAR_FIELD(u)
/// Particle thermal energy time derivative.
TIT_DEFINE_SCALAR_FIELD(du_dt)

/// Particle dynamic viscosity.
TIT_DEFINE_SCALAR_FIELD(mu)

/// Particle heat conductivity coefficient.
TIT_DEFINE_SCALAR_FIELD(kappa)

/// Particle artificial viscosity switch.
TIT_DEFINE_SCALAR_FIELD(alpha)
/// Particle artificial viscosity switch time derivative.
TIT_DEFINE_SCALAR_FIELD(dalpha_dt)

/// Particle concentration value.
TIT_DEFINE_SCALAR_FIELD(C)
/// Particle normal vector.
TIT_DEFINE_VECTOR_FIELD(N)
/// Particle renormalization matrix.
TIT_DEFINE_MATRIX_FIELD(L)

/// Particle free surface flag.
TIT_DEFINE_SCALAR_FIELD(FS)

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
