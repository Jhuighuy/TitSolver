/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <optional>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

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
  template<class Self, class PV>
  constexpr auto operator[](this const Self& self, PV&& a) noexcept
      -> decltype(auto) {
    return std::forward<PV>(a)[self];
  }

  /// Field value delta for the specified particle view.
  template<class Self, class PVa, class PVb>
  constexpr auto operator[](this const Self& self, PVa&& a, PVb&& b) noexcept {
    return std::forward<PVa>(a)[self] - std::forward<PVb>(b)[self];
  }

  /// Average of the field values over the specified particle views.
  template<class Self, class... PVs>
  constexpr auto avg(this const Self& self, PVs&&... ai) {
    return tit::avg(std::forward<PVs>(ai)[self]...);
  }

  /// Harmonic average of the field values over the specified particle views.
  template<class Self, class... PVs>
  constexpr auto havg(this const Self& self, PVs&&... ai) {
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

template<field... Fields>
inline constexpr bool is_field_set_v<TypeSubset<Fields...>> = true;

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
/// Particle velocity gradient.
TIT_DEFINE_MATRIX_FIELD(grad_v)

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

/// Particle normal vector.
TIT_DEFINE_VECTOR_FIELD(N)
/// Particle renormalization matrix.
TIT_DEFINE_MATRIX_FIELD(L)

/// Particle free surface flag.
TIT_DEFINE_SCALAR_FIELD(FS)

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

struct None {};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

constexpr auto get_required_uniforms(const None& /*impl*/) noexcept {
  return TypeSubset{/*empty*/};
}
constexpr auto get_required_varyings(const None& /*impl*/) noexcept {
  return TypeSubset{/*empty*/};
}

template<class Impl>
constexpr auto get_required_uniforms(const Impl& impl) noexcept {
  return impl.required_uniforms();
}
template<class Impl>
constexpr auto get_required_varyings(const Impl& impls) noexcept {
  return impls.required_varyings();
}

template<class... Impls>
constexpr auto get_required_uniforms(
    const std::tuple<Impls...>& impls) noexcept {
  return std::apply(
      [](const auto&... impl) {
        return (get_required_uniforms(impl) | ... | TypeSet{/*empty*/});
      },
      impls);
}
template<class... Impls>
constexpr auto get_required_varyings(
    const std::tuple<Impls...>& impls) noexcept {
  return std::apply(
      [](const auto&... impl) {
        return (get_required_varyings(impl) | ... | TypeSet{/*empty*/});
      },
      impls);
}

template<class Impl>
constexpr auto get_required_uniforms(const std::optional<Impl>& impl) noexcept {
  decltype(TypeSubset{get_required_uniforms(std::declval<Impl>())}) result{};
  if (impl.has_value()) result = result | get_required_uniforms(*impl);
  return result;
}
template<class Impl>
constexpr auto get_required_varyings(const std::optional<Impl>& impl) noexcept {
  decltype(TypeSubset{get_required_varyings(std::declval<Impl>())}) result{};
  if (impl.has_value()) result = result | get_required_varyings(*impl);
  return result;
}

template<class... Impls>
constexpr auto get_required_uniforms(
    const std::variant<Impls...>& impls) noexcept {
  decltype(TypeSubset{
      (get_required_uniforms(std::declval<Impls>()) | ...)}) result{};
  std::visit(
      [&result](const auto& impl) {
        result = result | get_required_uniforms(impl);
      },
      impls);
  return result;
}
template<class... Impls>
constexpr auto get_required_varyings(
    const std::variant<Impls...>& impls) noexcept {
  decltype(TypeSubset{
      (get_required_varyings(std::declval<Impls>()) | ...)}) result{};
  std::visit(
      [&result](const auto& impl) {
        result = result | get_required_varyings(impl);
      },
      impls);
  return result;
}

template<class Impl>
constexpr auto get_required_uniforms(const std::vector<Impl>& impls) noexcept {
  decltype(TypeSubset{get_required_uniforms(std::declval<Impl>())}) result{};
  for (const auto& impl : impls) result = result | get_required_uniforms(impl);
  return result;
}
template<class Impl>
constexpr auto get_required_varyings(const std::vector<Impl>& impls) noexcept {
  decltype(TypeSubset{get_required_varyings(std::declval<Impl>())}) result{};
  for (const auto& impl : impls) result = result | get_required_varyings(impl);
  return result;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#define TIT_ENABLED(eq) /*if*/ constexpr(different_from<decltype(eq), None>)

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
