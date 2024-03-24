/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once
#ifdef __ARM_NEON

#include <arm_neon.h>

#include "tit/core/basic_types.hpp"
#include "tit/core/simd/fwd.hpp"
#include "tit/core/utils.hpp"

namespace tit::simd {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

template<>
class alignas(16) Reg<float64_t, 2> final {
public:

  float64x2_t reg; // NOLINT(*-non-private-member-variables-in-classes)

  TIT_FORCE_INLINE Reg() = default;

  TIT_FORCE_INLINE Reg(float64x2_t r) noexcept : reg{r} {}

  TIT_FORCE_INLINE Reg(float64_t qx, float64_t qy) noexcept : reg{qx, qy} {}

  TIT_FORCE_INLINE explicit Reg(zeroinit_t /*z*/) noexcept : Reg(0.0) {}

  TIT_FORCE_INLINE explicit Reg(float64_t q) noexcept : reg{vdupq_n_f64(q)} {}

}; // class Reg<float64_t, 2>

template<>
class alignas(16) RegMask<float64_t, 2> final {
public:

  uint64x2_t reg; // NOLINT(*-non-private-member-variables-in-classes)

  TIT_FORCE_INLINE RegMask() = default;

  TIT_FORCE_INLINE RegMask(uint64x2_t r) noexcept : reg{r} {}

  TIT_FORCE_INLINE RegMask(uint64_t qx, uint64_t qy) noexcept : reg{qx, qy} {}

  TIT_FORCE_INLINE explicit RegMask(zeroinit_t /*z*/) noexcept : RegMask(0.0) {}

  TIT_FORCE_INLINE explicit RegMask(uint64_t q) noexcept
      : reg{vdupq_n_u64(q)} {}

}; // class RegMask<float64_t, 2>

inline namespace float64x2 {

using Reg_ = Reg<float64_t, 2>;
using RegMask_ = RegMask<float64_t, 2>;

TIT_FORCE_INLINE auto vmvnq_u64(uint64x2_t reg) noexcept -> uint64x2_t {
  return vreinterpretq_u64_u32(vmvnq_u32(vreinterpretq_u32_u64(reg)));
}

TIT_FORCE_INLINE auto operator+(Reg_ a, Reg_ b) noexcept -> Reg_ {
  return vaddq_f64(a.reg, b.reg);
}

TIT_FORCE_INLINE auto operator-(Reg_ a) noexcept -> Reg_ {
  return vnegq_f64(a.reg);
}

TIT_FORCE_INLINE auto operator-(Reg_ a, Reg_ b) noexcept -> Reg_ {
  return vsubq_f64(a.reg, b.reg);
}

TIT_FORCE_INLINE auto operator*(Reg_ a, Reg_ b) noexcept -> Reg_ {
  return vmulq_f64(a.reg, b.reg);
}

TIT_FORCE_INLINE auto operator/(Reg_ a, Reg_ b) noexcept -> Reg_ {
  return vdivq_f64(a.reg, b.reg);
}

#if __ARM_FEATURE_FMA

TIT_FORCE_INLINE auto fma(Reg_ a, Reg_ b, Reg_ c) noexcept -> Reg_ {
  return vfmaq_f64(c.reg, a.reg, b.reg);
}

TIT_FORCE_INLINE auto fnma(Reg_ a, Reg_ b, Reg_ c) noexcept -> Reg_ {
  return vfmsq_f64(c.reg, a.reg, b.reg);
}

#endif // if __ARM_FEATURE_FMA

TIT_FORCE_INLINE auto abs(Reg_ a) noexcept -> Reg_ {
  return vabsq_f64(a.reg);
}

TIT_FORCE_INLINE auto abs_delta(Reg_ a, Reg_ b) noexcept -> Reg_ {
  return vabdq_f64(a.reg, b.reg);
}

TIT_FORCE_INLINE auto min(Reg_ a, Reg_ b) noexcept -> Reg_ {
  return vminq_f64(a.reg, b.reg);
}

TIT_FORCE_INLINE auto max(Reg_ a, Reg_ b) noexcept -> Reg_ {
  return vmaxq_f64(a.reg, b.reg);
}

TIT_FORCE_INLINE auto floor(Reg_ a) noexcept -> Reg_ {
  return vrndmq_f64(a.reg);
}

TIT_FORCE_INLINE auto round(Reg_ a) noexcept -> Reg_ {
  return vrndnq_f64(a.reg);
}

TIT_FORCE_INLINE auto ceil(Reg_ a) noexcept -> Reg_ {
  return vrndpq_f64(a.reg);
}

TIT_FORCE_INLINE auto operator==(Reg_ a, Reg_ b) noexcept -> RegMask_ {
  return vceqq_f64(a.reg, b.reg);
}

TIT_FORCE_INLINE auto operator!=(Reg_ a, Reg_ b) noexcept -> RegMask_ {
  return vmvnq_u64(vceqq_f64(a.reg, b.reg));
}

TIT_FORCE_INLINE auto operator<(Reg_ a, Reg_ b) noexcept -> RegMask_ {
  return vcltq_f64(a.reg, b.reg);
}

TIT_FORCE_INLINE auto operator<=(Reg_ a, Reg_ b) noexcept -> RegMask_ {
  return vcleq_f64(a.reg, b.reg);
}

TIT_FORCE_INLINE auto operator>(Reg_ a, Reg_ b) noexcept -> RegMask_ {
  return vcgtq_f64(a.reg, b.reg);
}

TIT_FORCE_INLINE auto operator>=(Reg_ a, Reg_ b) noexcept -> RegMask_ {
  return vcgeq_f64(a.reg, b.reg);
}

TIT_FORCE_INLINE auto blend_zero(RegMask_ m, Reg_ a) noexcept -> Reg_ {
  return vreinterpretq_f64_u64( //
      vandq_u64(vreinterpretq_u64_f64(a.reg), m.reg));
}

TIT_FORCE_INLINE auto blend(RegMask_ m, Reg_ a, Reg_ b) noexcept -> Reg_ {
  return vreinterpretq_f64_u64(
      vorrq_u64(vandq_u64(vreinterpretq_u64_f64(a.reg), m.reg),
                vbicq_u64(vreinterpretq_u64_f64(b.reg), m.reg)));
}

TIT_FORCE_INLINE auto sum(Reg_ a) noexcept -> float64_t {
  return vaddvq_f64(a.reg);
}

TIT_FORCE_INLINE auto min_value(Reg_ a) noexcept -> float64_t {
  return vminvq_f64(a.reg);
}

TIT_FORCE_INLINE auto max_value(Reg_ a) noexcept -> float64_t {
  return vmaxvq_f64(a.reg);
}

TIT_FORCE_INLINE auto any(RegMask_ m) noexcept -> bool {
  uint64_t const none_mask = 0;
  return (vgetq_lane_u64(m.reg, 0) | vgetq_lane_u64(m.reg, 1)) != none_mask;
}

TIT_FORCE_INLINE auto all(RegMask_ m) noexcept -> bool {
  uint64_t const all_mask = ~uint64_t{0};
  return (vgetq_lane_u64(m.reg, 0) & vgetq_lane_u64(m.reg, 1)) == all_mask;
}

} // namespace float64x2

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::simd

#endif // __ARM_NEON
