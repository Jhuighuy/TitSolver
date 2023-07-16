/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*\
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
\*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#pragma once

#include "tit/utils/meta.hpp"
#include "tit/utils/misc.hpp"
#include "tit/utils/vec.hpp"

namespace tit {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

template<class PV>
concept _has_fields =
    requires { std::remove_cvref_t<PV>::fields; } && //
    meta::is_set_v<decltype(auto(std::remove_cvref_t<PV>::fields))>;

template<class PV>
concept _has_constants =
    requires { std::remove_cvref_t<PV>::constants; } && //
    meta::is_set_v<decltype(auto(std::remove_cvref_t<PV>::constants))>;

template<class PV>
concept _has_variables =
    requires { std::remove_cvref_t<PV>::variables; } && //
    meta::is_set_v<decltype(auto(std::remove_cvref_t<PV>::variables))>;

/** Check particle fields presense. */
/** @{ */
template<_has_fields PV, meta::type... Fields>
consteval bool has(meta::Set<Fields...> fields) {
  return std::remove_cvref_t<PV>::fields.includes(fields);
}
template<_has_fields PV, meta::type... Fields>
consteval bool has(Fields... fields) {
  return has<PV>(meta::Set{fields...});
}
template<_has_fields PV, meta::type... Fields>
consteval bool has() {
  return has<PV>(Fields{}...);
}
/** @} */

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Declare a particle field. */
#define TIT_DEFINE_FIELD(type, name, ...)                           \
  namespace particle_fields {                                       \
    class name##_t {                                                \
    public:                                                         \
                                                                    \
      /** Field name. */                                            \
      static constexpr std::string field_name = #name;              \
                                                                    \
      /** Field type. */                                            \
      template<class Real, dim_t Dim>                               \
      using field_value_type = type;                                \
                                                                    \
      /** Field value for the specified particle view. */           \
      template<_has_fields PV>                                      \
        requires (has<PV, name##_t>())                              \
      static constexpr decltype(auto) operator[](PV&& a) noexcept { \
        return a[name##_t{}];                                       \
      }                                                             \
      /** Field value delta for the specified particle view. */     \
      template<_has_fields PV>                                      \
        requires (has<PV, name##_t>())                              \
      static constexpr auto operator[](PV&& a, PV&& b) noexcept {   \
        return a[name##_t{}] - b[name##_t{}];                       \
      }                                                             \
                                                                    \
    }; /* class name##_t */                                         \
    inline constexpr name##_t name __VA_OPT__(, __VA_ARGS__);       \
  } // namespace particle_fields

/** Declare a scalar particle field. */
#define TIT_DEFINE_SCALAR_FIELD(name, ...) \
  TIT_DEFINE_FIELD(Real, name __VA_OPT__(, __VA_ARGS__))

/** Declare a scalar particle field. */
#define TIT_DEFINE_VECTOR_FIELD(name, ...) \
  TIT_DEFINE_FIELD(TIT_PASS(Vec<Real, Dim>), name __VA_OPT__(, __VA_ARGS__))

/** Field name. */
template<meta::type Field>
inline constexpr auto field_name_v = std::remove_cvref_t<Field>::field_name;

/** Field type. */
template<meta::type Field, class Real, dim_t Dim>
using field_value_type_t =
    typename std::remove_cvref_t<Field>::field_value_type<Real, Dim>;

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Is particle fixed? For the fixed particles,
 ** no variables are updated during the simulation. */
TIT_DEFINE_FIELD(bool, fixed);

/** Particle position. */
TIT_DEFINE_VECTOR_FIELD(r);

/** Particle velocity. */
TIT_DEFINE_VECTOR_FIELD(v, dr_dt);
/** Particle velocity (XSPH model). */
TIT_DEFINE_VECTOR_FIELD(v_xsph, dr_dt_xsph);

/** Particle velocity divergence. */
TIT_DEFINE_SCALAR_FIELD(div_v);
/** Particle velocity curl (always 3D). */
TIT_DEFINE_FIELD(TIT_PASS(Vec<Real, 3>), curl_v);

/** Particle acceleration. */
TIT_DEFINE_VECTOR_FIELD(a, dv_dt);

/** Particle mass. */
TIT_DEFINE_SCALAR_FIELD(m);
/** Particle density. */
TIT_DEFINE_SCALAR_FIELD(rho);

/** Particle width. */
TIT_DEFINE_SCALAR_FIELD(h);
/** Particle "Omega" variable (Grad-H model) */
TIT_DEFINE_SCALAR_FIELD(Omega);

/** Particle pressure. */
TIT_DEFINE_SCALAR_FIELD(p);
/** Particle sound speed. */
TIT_DEFINE_SCALAR_FIELD(cs);

/** Particle thermal energy. */
TIT_DEFINE_SCALAR_FIELD(eps);
/** Particle thermal energy time derivative. */
TIT_DEFINE_SCALAR_FIELD(deps_dt);

/** Particle artificial viscosity switch. */
TIT_DEFINE_SCALAR_FIELD(alpha);
/** Particle artificial viscosity switch time derivative. */
TIT_DEFINE_SCALAR_FIELD(dalpha_dt);

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit