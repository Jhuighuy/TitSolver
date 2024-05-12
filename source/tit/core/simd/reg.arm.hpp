/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#ifndef __ARM_NEON
#error This header may only be included when NEON is available!
#endif

#include <arm_neon.h>

#include "tit/core/basic_types.hpp"
#include "tit/core/simd/reg.hpp"

[[gnu::always_inline]]
inline auto vmvnq_u64(uint64x2_t reg) noexcept -> uint64x2_t {
  return vreinterpretq_u64_u32(vmvnq_u32(vreinterpretq_u32_u64(reg)));
}

namespace tit::simd {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

template<>
class alignas(16) RegMask<float64_t, 2> final {
public:

  uint64x2_t data; // NOLINT(*-non-private-member-variables-in-classes)

  [[gnu::always_inline]]
  RegMask() noexcept
      : RegMask(0.0) {}

  [[gnu::always_inline]]
  RegMask(uint64x2_t r) noexcept
      : data{r} {}

  [[gnu::always_inline]]
  explicit RegMask(uint64_t q) noexcept
      : data{vdupq_n_u64(q)} {}

}; // class RegMask<float64_t, 2>

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

[[gnu::always_inline]]
inline auto any(RegMask<float64_t, 2> const& m) noexcept -> bool {
  return (vgetq_lane_u64(m.data, 0) | vgetq_lane_u64(m.data, 1)) != 0;
}

[[gnu::always_inline]]
inline auto all(RegMask<float64_t, 2> const& m) noexcept -> bool {
  return ~(vgetq_lane_u64(m.data, 0) & vgetq_lane_u64(m.data, 1)) == 0;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

template<>
class alignas(16) Reg<float64_t, 2> final {
public:

  using RegMask = tit::simd::RegMask<float64_t, 2>;

  float64x2_t data; // NOLINT(*-non-private-member-variables-in-classes)

  [[gnu::always_inline]]
  Reg() noexcept
      : Reg(0.0) {}

  [[gnu::always_inline]]
  Reg(float64x2_t r) noexcept
      : data{r} {}

  [[gnu::always_inline]]
  explicit Reg(float64_t q) noexcept
      : data{vdupq_n_f64(q)} {}

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  [[gnu::always_inline]]
  friend auto operator+(Reg const& a, Reg const& b) noexcept -> Reg {
    return vaddq_f64(a.data, b.data);
  }

  [[gnu::always_inline]]
  friend auto operator-(Reg const& a) noexcept -> Reg {
    return vnegq_f64(a.data);
  }

  [[gnu::always_inline]]
  friend auto operator-(Reg const& a, Reg const& b) noexcept -> Reg {
    return vsubq_f64(a.data, b.data);
  }

  [[gnu::always_inline]]
  friend auto operator*(Reg const& a, Reg const& b) noexcept -> Reg {
    return vmulq_f64(a.data, b.data);
  }

  [[gnu::always_inline]]
  friend auto operator/(Reg const& a, Reg const& b) noexcept -> Reg {
    return vdivq_f64(a.data, b.data);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  [[gnu::always_inline]]
  friend auto operator==(Reg const& a, Reg const& b) noexcept -> RegMask {
    return vceqq_f64(a.data, b.data);
  }

  [[gnu::always_inline]]
  friend auto operator!=(Reg const& a, Reg const& b) noexcept -> RegMask {
    return vmvnq_u64(vceqq_f64(a.data, b.data));
  }

  [[gnu::always_inline]]
  friend auto operator<(Reg const& a, Reg const& b) noexcept -> RegMask {
    return vcltq_f64(a.data, b.data);
  }

  [[gnu::always_inline]]
  friend auto operator<=(Reg const& a, Reg const& b) noexcept -> RegMask {
    return vcleq_f64(a.data, b.data);
  }

  [[gnu::always_inline]]
  friend auto operator>(Reg const& a, Reg const& b) noexcept -> RegMask {
    return vcgtq_f64(a.data, b.data);
  }

  [[gnu::always_inline]]
  friend auto operator>=(Reg const& a, Reg const& b) noexcept -> RegMask {
    return vcgeq_f64(a.data, b.data);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

}; // class Reg<float64_t, 2>

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

[[gnu::always_inline]]
inline auto min(Reg<float64_t, 2> const& a,
                Reg<float64_t, 2> const& b) noexcept -> Reg<float64_t, 2> {
  return vminq_f64(a.data, b.data);
}

[[gnu::always_inline]]
inline auto max(Reg<float64_t, 2> const& a,
                Reg<float64_t, 2> const& b) noexcept -> Reg<float64_t, 2> {
  return vmaxq_f64(a.data, b.data);
}

[[gnu::always_inline]]
inline auto filter(RegMask<float64_t, 2> const& m,
                   Reg<float64_t, 2> const& a) noexcept -> Reg<float64_t, 2> {
  return vreinterpretq_f64_u64(
      vandq_u64(vreinterpretq_u64_f64(a.data), m.data));
}

[[gnu::always_inline]]
inline auto blend(RegMask<float64_t, 2> const& m,
                  Reg<float64_t, 2> const& a,
                  Reg<float64_t, 2> const& b) noexcept -> Reg<float64_t, 2> {
  return vreinterpretq_f64_u64(
      vorrq_u64(vandq_u64(vreinterpretq_u64_f64(a.data), m.data),
                vbicq_u64(vreinterpretq_u64_f64(b.data), m.data)));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

[[gnu::always_inline]]
inline auto floor(Reg<float64_t, 2> const& a) noexcept -> Reg<float64_t, 2> {
  return vrndmq_f64(a.data);
}

[[gnu::always_inline]]
inline auto round(Reg<float64_t, 2> const& a) noexcept -> Reg<float64_t, 2> {
  return vrndnq_f64(a.data);
}

[[gnu::always_inline]]
inline auto ceil(Reg<float64_t, 2> const& a) noexcept -> Reg<float64_t, 2> {
  return vrndpq_f64(a.data);
}

#if __ARM_FEATURE_FMA

[[gnu::always_inline]]
inline auto fma(Reg<float64_t, 2> const& a,
                Reg<float64_t, 2> const& b,
                Reg<float64_t, 2> const& c) noexcept -> Reg<float64_t, 2> {
  return vfmaq_f64(c.data, a.data, b.data);
}

[[gnu::always_inline]]
inline auto fnma(Reg<float64_t, 2> const& a,
                 Reg<float64_t, 2> const& b,
                 Reg<float64_t, 2> const& c) noexcept -> Reg<float64_t, 2> {
  return vfmsq_f64(c.data, a.data, b.data);
}

#endif // if __ARM_FEATURE_FMA

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

[[gnu::always_inline]]
inline auto sum(Reg<float64_t, 2> const& a) noexcept -> float64_t {
  return vaddvq_f64(a.data);
}

[[gnu::always_inline]]
inline auto min_value(Reg<float64_t, 2> const& a) noexcept -> float64_t {
  return vminvq_f64(a.data);
}

[[gnu::always_inline]]
inline auto max_value(Reg<float64_t, 2> const& a) noexcept -> float64_t {
  return vmaxvq_f64(a.data);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::simd
