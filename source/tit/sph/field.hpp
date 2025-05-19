/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/mat.hpp"
#include "tit/core/math.hpp"
#include "tit/core/type.hpp"
#include "tit/core/utils.hpp"
#include "tit/core/vec.hpp"
#include "tit/data/type.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Base field specification.
class BaseField {
public:

  /// Field value for the specified particle view.
  template<class PV>
  constexpr auto operator[](this const auto& self, PV&& a) noexcept
      -> decltype(auto) {
    return std::forward<PV>(a)[self];
  }

  /// Field value delta for the specified particle view.
  template<class PVa, class PVb>
  constexpr auto operator[](this const auto& self, PVa&& a, PVb&& b) noexcept {
    return std::forward<PVa>(a)[self] - std::forward<PVb>(b)[self];
  }

  /// Average of the field values over the specified particle views.
  template<class... PVs>
  constexpr auto avg(this const auto& self, PVs&&... ai) {
    return tit::avg(std::forward<PVs>(ai)[self]...);
  }

  /// Harmonic average of the field values over the specified particle views.
  template<class... PVs>
  constexpr auto havg(this const auto& self, PVs&&... ai) {
    return tit::havg(std::forward<PVs>(ai)[self]...);
  }

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
template<empty_type Field>
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

template<empty_type Field, class Space>
struct field_value;

template<empty_type Field, class Real, size_t Dim>
struct field_value<Field, Space<Real, Dim>> {
  using type =
      typename std::remove_cvref_t<Field>::template field_value_type<Real, Dim>;
};

/// Field value type.
template<empty_type Field, class Space>
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
    return TIT_FORWARD_LIKE(self, self.vec_[i]);
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

/// Particle thermal energy.
TIT_DEFINE_SCALAR_FIELD(u)
/// Particle thermal energy time derivative.
TIT_DEFINE_SCALAR_FIELD(du_dt)

/// Particle dynamic viscosity.
TIT_DEFINE_SCALAR_FIELD(mu)

/// Particle heat conductivity coefficient.
TIT_DEFINE_SCALAR_FIELD(kappa)

/// Particle normal vector.
TIT_DEFINE_VECTOR_FIELD(N)
/// Particle renormalization matrix.
TIT_DEFINE_MATRIX_FIELD(L)

/// Particle free surface flag.
TIT_DEFINE_SCALAR_FIELD(FS)

namespace sph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Represents absence of an equation.
class None {
public:

  /// Default constructor.
  constexpr explicit None() = default;

  /// Set of required uniform fields.
  static constexpr auto required_uniforms() noexcept {
    return TypeSet{/*empty*/};
  }

  /// Set of required varying fields.
  static constexpr auto required_varyings() noexcept {
    return TypeSet{/*empty*/};
  }

}; // class None

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Represents a variant of equations.
template<class... Equations>
class Variant final {
public:

  /// Construct the variant.
  template<class Equation>
    requires contains_v<Equation, Equations...>
  constexpr explicit(false) Variant(Equation equation) noexcept
      : equation_variant_{std::move(equation)} {}

  /// Visit the active equation.
  template<class... Visitors>
  constexpr auto visit(Visitors&&... visitors) const {
    return std::visit(Overload{std::forward<Visitors>(visitors)...},
                      equation_variant_);
  }

  /// Visit the active equation, if it is not `None`.
  template<class... Visitors>
  constexpr void and_then(Visitors&&... visitors) const {
    visit([](None /*none*/) {}, std::forward<Visitors>(visitors)...);
  }

  /// Set of required uniform fields.
  constexpr auto required_uniforms() const noexcept {
    decltype(TypeSubset{
        (std::declval<Equations>().required_uniforms() | ...)}) result{};
    visit([&result](const auto& equation) {
      result = result | equation.required_uniforms();
    });
    return result;
  }

  /// Set of required varying fields.
  constexpr auto required_varyings() const noexcept {
    decltype(TypeSubset{
        (std::declval<Equations>().required_varyings() | ...)}) result{};
    visit([&result](const auto& equation) {
      result = result | equation.required_varyings();
    });
    return result;
  }

private:

  std::variant<Equations...> equation_variant_;

}; // class Variant

template<class Equation>
class Variant<Equation> final {
public:

  /// Construct the variant.
  constexpr explicit(false) Variant(Equation equation) noexcept
      : equation_{std::move(equation)} {}

  /// Visit the active equation.
  template<class... Visitors>
  constexpr auto visit(Visitors&&... visitors) const {
    return std::invoke(Overload{std::forward<Visitors>(visitors)...},
                       equation_);
  }

  /// Visit the active equation, if it is not `None`.
  template<class... Visitors>
  constexpr void and_then(Visitors&&... visitors) const noexcept {
    visit([](None /*none*/) {}, std::forward<Visitors>(visitors)...);
  }

  /// Set of required uniform fields.
  constexpr auto required_uniforms() const noexcept {
    return equation_.required_uniforms();
  }

  /// Set of required varying fields.
  constexpr auto required_varyings() const noexcept {
    return equation_.required_varyings();
  }

private:

  [[no_unique_address]] Equation equation_;

}; // class Variant

template<class Equation>
Variant(Equation) -> Variant<Equation>;

/// Variant of equations or none.
template<class... Equations>
using OptionalVariant = Variant<None, Equations...>;

/// Variant type.
template<class V>
concept variant = specialization_of<V, Variant>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace sph
} // namespace tit
