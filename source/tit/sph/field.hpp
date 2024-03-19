/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <type_traits>
#include <utility>

#include "tit/core/basic_types.hpp"
#include "tit/core/mat.hpp"
#include "tit/core/math.hpp"
#include "tit/core/meta.hpp"
#include "tit/core/misc.hpp"
#include "tit/core/vec.hpp"

namespace tit {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

template<class PV>
concept has_fields_ =
    requires { std::remove_cvref_t<PV>::fields; } && //
    meta::is_set_v<decltype(auto(std::remove_cvref_t<PV>::fields))>;

template<class PV>
concept has_constants_ =
    requires { std::remove_cvref_t<PV>::constants; } && //
    meta::is_set_v<decltype(auto(std::remove_cvref_t<PV>::constants))>;

/** Check particle fields presence. */
/** @{ */
template<has_fields_ PV, meta::type... Fields>
consteval auto has(meta::Set<Fields...> fields) -> bool {
  return std::remove_cvref_t<PV>::fields.includes(fields);
}
template<has_fields_ PV, meta::type... Fields>
consteval auto has(Fields... fields) -> bool {
  return has<PV>(meta::Set{fields...});
}
template<has_fields_ PV, meta::type... Fields>
consteval auto has() -> bool {
  return has<PV>(Fields{}...);
}
/** @} */

/** Check particle constant presence. */
/** @{ */
template<has_fields_ PV, meta::type... Consts>
consteval auto has_const(meta::Set<Consts...> consts) -> bool {
  if constexpr (!has_constants_<PV>) return false;
  else {
    return has<PV>(consts) &&
           std::remove_cvref_t<PV>::constants.includes(consts);
  }
}
template<has_fields_ PV, meta::type... Consts>
consteval auto has_const(Consts... consts) -> bool {
  return has_const<PV>(meta::Set{consts...});
}
template<has_fields_ PV, meta::type... Consts>
consteval auto has_const() -> bool {
  return has_const<PV>(Consts{}...);
}
/** @} */

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Abstract class for fields specification. */
template<class field_t>
class Field {
public:

  /** Field value for the specified particle view. */
  template<has_fields_ PV>
    requires (has<PV, field_t>())
  constexpr auto operator[](PV&& a) const noexcept -> decltype(auto) {
    return std::forward<PV>(a)[field_t{}];
  }

  /** Field value for the specified particle view or default value. */
  template<has_fields_ PV>
  constexpr auto get(PV&& a, auto default_) const noexcept {
    if constexpr (has<PV, field_t>()) return std::forward<PV>(a)[field_t{}];
    else return default_;
  }

  /** Field value delta for the specified particle view. */
  template<has_fields_ PV>
    requires (has<PV, field_t>())
  constexpr auto operator[](PV&& a, PV&& b) const noexcept {
    return std::forward<PV>(a)[field_t{}] - std::forward<PV>(b)[field_t{}];
  }

  /** Average of the field values over the specified particle views. */
  // TODO: here we should check for all `PVs` types to be the same and avoid
  // averaging if the current field is const.
  template<has_fields_... PVs>
    requires (... && has<PVs, field_t>())
  constexpr auto avg(PVs&&... ai) const noexcept {
    // Namespace prefix is a must here to avoid recursion.
    return tit::avg(std::forward<PVs>(ai)[field_t{}]...);
  }

  /** Weighted average of the field values over the specified particle views. */
  template<has_fields_... PVs>
    requires (... && has<PVs, field_t>())
  constexpr auto wavg(PVs&&... ai) const noexcept {
    // Namespace prefix is a must here to avoid recursion.
    return tit::avg(std::forward<PVs>(ai)[field_t{}]...);
  }

  /** Harmonic average of the field values over the specified particle views. */
  template<has_fields_... PVs>
    requires (... && has<PVs, field_t>())
  constexpr auto havg(PVs&&... ai) const noexcept {
    // Namespace prefix is a must here to avoid recursion.
    return tit::havg(std::forward<PVs>(ai)[field_t{}]...);
  }

}; // class Field

/** Declare a particle field. */
#define TIT_DEFINE_FIELD(type, name, ...)                                      \
  class name##_t final : public Field<name##_t> {                              \
  public:                                                                      \
                                                                               \
    /** Field name. */                                                         \
    static constexpr const char* field_name = #name;                           \
                                                                               \
    /** Field type. */                                                         \
    template<class Real, size_t Dim>                                           \
    using field_value_type = type;                                             \
                                                                               \
  }; /* class name##_t */                                                      \
  inline constexpr name##_t name __VA_OPT__(, __VA_ARGS__);

/** Declare a scalar particle field. */
#define TIT_DEFINE_SCALAR_FIELD(name, ...)                                     \
  TIT_DEFINE_FIELD(Real, name __VA_OPT__(, __VA_ARGS__))

/** Declare a vector particle field. */
#define TIT_DEFINE_VECTOR_FIELD(name, ...)                                     \
  TIT_DEFINE_FIELD(TIT_PASS(Vec<Real, Dim>), name __VA_OPT__(, __VA_ARGS__))

/** Declare a matrix particle field. */
#define TIT_DEFINE_MATRIX_FIELD(name, ...)                                     \
  TIT_DEFINE_FIELD(TIT_PASS(Mat<Real, Dim>), name __VA_OPT__(, __VA_ARGS__))

/** Field name. */
template<meta::type Field>
inline constexpr auto field_name_v = std::remove_cvref_t<Field>::field_name;

/** Field type. */
template<meta::type Field, class Real, size_t Dim>
using field_value_type_t =
    typename std::remove_cvref_t<Field>::template field_value_type<Real, Dim>;

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

enum class ParState {
  /** Particle is far from subdomain boundary. */
  INNER,
  /** Particle is near subdomain boundary, and is in halo of some other
   ** subdomain. Fields of such particles are send to the corresponding
   ** processes during synchronization. */
  NEAR_HALO,
  /** Particle is on the subdomain boundary. Fields of such particles are
   ** received from the corresponding processes during synchronization. */
  HALO
};
struct ParInfo {
  size_t part;
  size_t global_index;
  ParState state;
};
template<class Stream>
constexpr auto operator<<(Stream& stream, ParInfo p) -> Stream& {
  stream << p.part;
  return stream;
}

/** Is particle fixed? For the fixed particles,
 ** no variables are updated during the simulation. */
TIT_DEFINE_FIELD(bool, fixed)
TIT_DEFINE_FIELD(ParInfo, parinfo)

/** Particle position. */
TIT_DEFINE_VECTOR_FIELD(r)

/** Particle velocity. */
TIT_DEFINE_VECTOR_FIELD(v)
/** Particle velocity (XSPH model). */
TIT_DEFINE_VECTOR_FIELD(v_xsph)
/** Particle acceleration. */
TIT_DEFINE_VECTOR_FIELD(dv_dt)
/** Particle velocity gradient. */
TIT_DEFINE_MATRIX_FIELD(grad_v)
/** Particle velocity divergence. */
TIT_DEFINE_SCALAR_FIELD(div_v)
/** Particle velocity curl (always 3D). */
TIT_DEFINE_FIELD(TIT_PASS(Vec<Real, 3>), curl_v)

/** Particle mass. */
TIT_DEFINE_SCALAR_FIELD(m)
/** Particle density. */
TIT_DEFINE_SCALAR_FIELD(rho)
/** Particle density gradient. */
TIT_DEFINE_VECTOR_FIELD(grad_rho)
/** Particle density time derivative. */
TIT_DEFINE_SCALAR_FIELD(drho_dt)

/** Particle width. */
TIT_DEFINE_SCALAR_FIELD(h)
/** Particle "Omega" variable (Grad-H model) */
TIT_DEFINE_SCALAR_FIELD(Omega)

/** Particle pressure. */
TIT_DEFINE_SCALAR_FIELD(p)
/** Particle sound speed. */
TIT_DEFINE_SCALAR_FIELD(cs)

/** Particle thermal energy. */
TIT_DEFINE_SCALAR_FIELD(u)
/** Particle thermal energy time derivative. */
TIT_DEFINE_SCALAR_FIELD(du_dt)

/** Particle molecular viscosity. */
TIT_DEFINE_SCALAR_FIELD(mu)
/** Particle molecular turbulent viscosity. */
TIT_DEFINE_SCALAR_FIELD(mu_T)
/** Particle second viscosity. */
TIT_DEFINE_SCALAR_FIELD(lambda)

/** Particle artificial viscosity switch. */
TIT_DEFINE_SCALAR_FIELD(alpha)
/** Particle artificial viscosity switch time derivative. */
TIT_DEFINE_SCALAR_FIELD(dalpha_dt)

/** Kernel renormalization coefficient (Shepard filter) */
TIT_DEFINE_SCALAR_FIELD(S)
/** Kernel gradient renormalization matrix. */
TIT_DEFINE_MATRIX_FIELD(L)

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

namespace sph::fsi {
/** Reference particle position. */
TIT_DEFINE_VECTOR_FIELD(r_0)
/** Piola-Kirchhoff stress tensor. */
TIT_DEFINE_MATRIX_FIELD(P)
} // namespace sph::fsi

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit
