// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Part of the Tit Solver project, under the MIT License.
// See /LICENSE.md for license information. SPDX-License-Identifier: MIT
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#pragma once
#ifdef __SSE__

#ifdef __AVX__
#include <tuple>
#endif

#include <immintrin.h> // IWYU pragma: keep

#include "tit/core/basic_types.hpp"
#include "tit/core/simd/fwd.hpp"
#include "tit/core/uint_utils.hpp"
#include "tit/core/utils.hpp"

namespace tit::simd {

// NOLINTBEGIN(*-include-cleaner)

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

template<>
class alignas(16) Reg<float64_t, 2> final {
public:

  __m128d reg; // NOLINT(*-non-private-member-variables-in-classes)

  TIT_FORCE_INLINE Reg() = default;

  TIT_FORCE_INLINE Reg(__m128d r) noexcept : reg{r} {}

  TIT_FORCE_INLINE Reg(float64_t qx, float64_t qy) noexcept
      : reg{_mm_setr_pd(qx, qy)} {}

  TIT_FORCE_INLINE explicit Reg(zeroinit_t /*z*/) noexcept
      : reg{_mm_setzero_pd()} {}

  TIT_FORCE_INLINE explicit Reg(float64_t q) noexcept : reg{_mm_set1_pd(q)} {}

}; // class Reg<float64_t, 2>

template<>
class alignas(16) RegMask<float64_t, 2> final {
public:

  __m128d reg; // NOLINT(*-non-private-member-variables-in-classes)

  TIT_FORCE_INLINE RegMask() = default;

  TIT_FORCE_INLINE RegMask(__m128d r) noexcept : reg{r} {}

  TIT_FORCE_INLINE RegMask(uint64_t qx, uint64_t qy) noexcept
      : reg{_mm_castsi128_pd(_mm_setr_epi64( //
            _mm_cvtsi64_m64(to_signed(qx)), _mm_cvtsi64_m64(to_signed(qy))))} {}

  TIT_FORCE_INLINE explicit RegMask(zeroinit_t /*z*/) noexcept
      : reg{_mm_setzero_pd()} {}

  TIT_FORCE_INLINE explicit RegMask(uint64_t q) noexcept
      : reg{_mm_castsi128_pd(_mm_set1_epi64(_mm_cvtsi64_m64(to_signed(q))))} {}

}; // class RegMask<float64_t, 2>

inline namespace float64x2 {

using Reg_ = Reg<float64_t, 2>;
using RegMask_ = RegMask<float64_t, 2>;

TIT_FORCE_INLINE auto operator+(Reg_ a, Reg_ b) noexcept -> Reg_ {
  return _mm_add_pd(a.reg, b.reg);
}

TIT_FORCE_INLINE auto operator-(Reg_ a) noexcept -> Reg_ {
  auto const sign_mask = _mm_set1_pd(-0.0);
  return _mm_xor_pd(sign_mask, a.reg);
}

TIT_FORCE_INLINE auto operator-(Reg_ a, Reg_ b) noexcept -> Reg_ {
  return _mm_sub_pd(a.reg, b.reg);
}

TIT_FORCE_INLINE auto operator*(Reg_ a, Reg_ b) noexcept -> Reg_ {
  return _mm_mul_pd(a.reg, b.reg);
}

TIT_FORCE_INLINE auto operator/(Reg_ a, Reg_ b) noexcept -> Reg_ {
  return _mm_div_pd(a.reg, b.reg);
}

#ifdef __FMA__

TIT_FORCE_INLINE auto fma(Reg_ a, Reg_ b, Reg_ c) noexcept -> Reg_ {
  return _mm_fmadd_pd(a.reg, b.reg, c.reg);
}

TIT_FORCE_INLINE auto fnma(Reg_ a, Reg_ b, Reg_ c) noexcept -> Reg_ {
  return _mm_fnmadd_pd(a.reg, b.reg, c.reg);
}

#endif // ifdef __FMA__

TIT_FORCE_INLINE auto abs(Reg_ a) noexcept -> Reg_ {
  auto const sign_mask = _mm_set1_pd(-0.0);
  return _mm_andnot_pd(sign_mask, a.reg);
}

TIT_FORCE_INLINE auto min(Reg_ a, Reg_ b) noexcept -> Reg_ {
  return _mm_min_pd(a.reg, b.reg);
}

TIT_FORCE_INLINE auto max(Reg_ a, Reg_ b) noexcept -> Reg_ {
  return _mm_max_pd(a.reg, b.reg);
}

TIT_FORCE_INLINE auto floor(Reg_ a) noexcept -> Reg_ {
  return _mm_round_pd(a.reg, _MM_FROUND_TO_NEG_INF);
}

TIT_FORCE_INLINE auto round(Reg_ a) noexcept -> Reg_ {
  return _mm_round_pd(a.reg, _MM_FROUND_TO_NEAREST_INT);
}

TIT_FORCE_INLINE auto ceil(Reg_ a) noexcept -> Reg_ {
  return _mm_round_pd(a.reg, _MM_FROUND_TO_POS_INF);
}

TIT_FORCE_INLINE auto operator==(Reg_ a, Reg_ b) noexcept -> RegMask_ {
  return _mm_cmp_pd(a.reg, b.reg, _CMP_EQ_OQ);
}

TIT_FORCE_INLINE auto operator!=(Reg_ a, Reg_ b) noexcept -> RegMask_ {
  return _mm_cmp_pd(a.reg, b.reg, _CMP_NEQ_OQ);
}

TIT_FORCE_INLINE auto operator<(Reg_ a, Reg_ b) noexcept -> RegMask_ {
  return _mm_cmp_pd(a.reg, b.reg, _CMP_LT_OQ);
}

TIT_FORCE_INLINE auto operator<=(Reg_ a, Reg_ b) noexcept -> RegMask_ {
  return _mm_cmp_pd(a.reg, b.reg, _CMP_LE_OQ);
}

TIT_FORCE_INLINE auto operator>(Reg_ a, Reg_ b) noexcept -> RegMask_ {
  return _mm_cmp_pd(a.reg, b.reg, _CMP_GT_OQ);
}

TIT_FORCE_INLINE auto operator>=(Reg_ a, Reg_ b) noexcept -> RegMask_ {
  return _mm_cmp_pd(a.reg, b.reg, _CMP_GE_OQ);
}

TIT_FORCE_INLINE auto blend_zero(RegMask_ m, Reg_ a) noexcept -> Reg_ {
  return _mm_and_pd(a.reg, m.reg);
}

TIT_FORCE_INLINE auto blend(RegMask_ m, Reg_ a, Reg_ b) noexcept -> Reg_ {
  return _mm_blendv_pd(b.reg, a.reg, m.reg);
}

TIT_FORCE_INLINE auto sum(Reg_ a) noexcept -> float64_t {
  auto const a_hi = _mm_unpackhi_pd(a.reg, a.reg);
  return _mm_cvtsd_f64(_mm_add_sd(a.reg, a_hi));
}

TIT_FORCE_INLINE auto min_value(Reg_ a) noexcept -> float64_t {
  auto const a_hi = _mm_unpackhi_pd(a.reg, a.reg);
  return _mm_cvtsd_f64(_mm_min_sd(a.reg, a_hi));
}

TIT_FORCE_INLINE auto max_value(Reg_ a) noexcept -> float64_t {
  auto const a_hi = _mm_unpackhi_pd(a.reg, a.reg);
  return _mm_cvtsd_f64(_mm_max_sd(a.reg, a_hi));
}

} // namespace float64x2

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#ifdef __AVX__

template<>
class alignas(32) Reg<float64_t, 4> final {
public:

  __m256d reg; // NOLINT(*-non-private-member-variables-in-classes)

  TIT_FORCE_INLINE Reg() = default;

  TIT_FORCE_INLINE Reg(__m256d r) noexcept : reg{r} {}

  TIT_FORCE_INLINE Reg(float64_t qx, float64_t qy, //
                       float64_t qz, float64_t qw) noexcept
      : reg{_mm256_setr_pd(qx, qy, qz, qw)} {}

  TIT_FORCE_INLINE explicit Reg(zeroinit_t /*z*/) noexcept
      : reg{_mm256_setzero_pd()} {}

  TIT_FORCE_INLINE explicit Reg(float64_t q) noexcept
      : reg{_mm256_set1_pd(q)} {}

}; // class Reg<float64_t, 4>

template<>
class alignas(32) RegMask<float64_t, 4> final {
public:

  __m256d reg; // NOLINT(*-non-private-member-variables-in-classes)

  TIT_FORCE_INLINE RegMask() = default;

  TIT_FORCE_INLINE RegMask(__m256d r) noexcept : reg{r} {}

  TIT_FORCE_INLINE RegMask(uint64_t qx, uint64_t qy, //
                           uint64_t qz, uint64_t qw) noexcept
      : reg{_mm256_castsi256_pd(_mm256_setr_epi64x(
            to_signed(qx), to_signed(qy), to_signed(qz), to_signed(qw)))} {}

  TIT_FORCE_INLINE explicit RegMask(zeroinit_t /*z*/) noexcept
      : reg{_mm256_setzero_pd()} {}

  TIT_FORCE_INLINE explicit RegMask(uint64_t q) noexcept
      : reg{_mm256_castsi256_pd(_mm256_set1_epi64x(to_signed(q)))} {}

}; // class RegMask<float64_t, 4>

inline namespace float64x4 {

using Reg_ = Reg<float64_t, 4>;
using RegMask_ = RegMask<float64_t, 4>;

TIT_FORCE_INLINE auto operator+(Reg_ a, Reg_ b) noexcept -> Reg_ {
  return _mm256_add_pd(a.reg, b.reg);
}

TIT_FORCE_INLINE auto operator-(Reg_ a) noexcept -> Reg_ {
  auto const sign_mask = _mm256_set1_pd(-0.0);
  return _mm256_xor_pd(sign_mask, a.reg);
}

TIT_FORCE_INLINE auto operator-(Reg_ a, Reg_ b) noexcept -> Reg_ {
  return _mm256_sub_pd(a.reg, b.reg);
}

TIT_FORCE_INLINE auto operator*(Reg_ a, Reg_ b) noexcept -> Reg_ {
  return _mm256_mul_pd(a.reg, b.reg);
}

TIT_FORCE_INLINE auto operator/(Reg_ a, Reg_ b) noexcept -> Reg_ {
  return _mm256_div_pd(a.reg, b.reg);
}

#ifdef __FMA__

TIT_FORCE_INLINE auto fma(Reg_ a, Reg_ b, Reg_ c) noexcept -> Reg_ {
  return _mm256_fmadd_pd(a.reg, b.reg, c.reg);
}

TIT_FORCE_INLINE auto fnma(Reg_ a, Reg_ b, Reg_ c) noexcept -> Reg_ {
  return _mm256_fnmadd_pd(a.reg, b.reg, c.reg);
}

#endif // ifdef __FMA__

TIT_FORCE_INLINE auto abs(Reg_ a) noexcept -> Reg_ {
  auto const sign_mask = _mm256_set1_pd(-0.0);
  return _mm256_andnot_pd(sign_mask, a.reg);
}

TIT_FORCE_INLINE auto min(Reg_ a, Reg_ b) noexcept -> Reg_ {
  return _mm256_min_pd(a.reg, b.reg);
}

TIT_FORCE_INLINE auto max(Reg_ a, Reg_ b) noexcept -> Reg_ {
  return _mm256_max_pd(a.reg, b.reg);
}

TIT_FORCE_INLINE auto floor(Reg_ a) noexcept -> Reg_ {
  return _mm256_round_pd(a.reg, _MM_FROUND_TO_NEG_INF);
}

TIT_FORCE_INLINE auto round(Reg_ a) noexcept -> Reg_ {
  return _mm256_round_pd(a.reg, _MM_FROUND_TO_NEAREST_INT);
}

TIT_FORCE_INLINE auto ceil(Reg_ a) noexcept -> Reg_ {
  return _mm256_round_pd(a.reg, _MM_FROUND_TO_POS_INF);
}

TIT_FORCE_INLINE auto operator==(Reg_ a, Reg_ b) noexcept -> RegMask_ {
  return _mm256_cmp_pd(a.reg, b.reg, _CMP_EQ_OQ);
}

TIT_FORCE_INLINE auto operator!=(Reg_ a, Reg_ b) noexcept -> RegMask_ {
  return _mm256_cmp_pd(a.reg, b.reg, _CMP_NEQ_OQ);
}

TIT_FORCE_INLINE auto operator<(Reg_ a, Reg_ b) noexcept -> RegMask_ {
  return _mm256_cmp_pd(a.reg, b.reg, _CMP_LT_OQ);
}

TIT_FORCE_INLINE auto operator<=(Reg_ a, Reg_ b) noexcept -> RegMask_ {
  return _mm256_cmp_pd(a.reg, b.reg, _CMP_LE_OQ);
}

TIT_FORCE_INLINE auto operator>(Reg_ a, Reg_ b) noexcept -> RegMask_ {
  return _mm256_cmp_pd(a.reg, b.reg, _CMP_GT_OQ);
}

TIT_FORCE_INLINE auto operator>=(Reg_ a, Reg_ b) noexcept -> RegMask_ {
  return _mm256_cmp_pd(a.reg, b.reg, _CMP_GE_OQ);
}

TIT_FORCE_INLINE auto blend_zero(RegMask_ m, Reg_ a) noexcept -> Reg_ {
  return _mm256_and_pd(a.reg, m.reg);
}

TIT_FORCE_INLINE auto blend(RegMask_ m, Reg_ a, Reg_ b) noexcept -> Reg_ {
  return _mm256_blendv_pd(b.reg, a.reg, m.reg);
}

TIT_FORCE_INLINE auto split(Reg_ a) noexcept {
  using HalfReg = Reg<float64_t, 2>;
  return std::tuple{HalfReg{_mm256_castpd256_pd128(a.reg)},
                    HalfReg{_mm256_extractf128_pd(a.reg, 1)}};
}

} // namespace float64x4

#endif // __AVX__

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Here I explicitly disable AVX-512 until I can test it on a machine that
// supports it. Seems like even GitHub Action runners don't support it.
// Although, the code should be mostly alive and well.
#if 0
#ifdef __AVX512F__

template<>
class alignas(64) Reg<float64_t, 8> final {
public:

  __m512d reg; // NOLINT(*-non-private-member-variables-in-classes)

  TIT_FORCE_INLINE Reg() = default;

  TIT_FORCE_INLINE Reg(__m512d r) noexcept : reg{r} {}

  TIT_FORCE_INLINE Reg(float64_t q0, float64_t q1, //
                       float64_t q2, float64_t q3, //
                       float64_t q4, float64_t q5, //
                       float64_t q6, float64_t q7) noexcept
      : reg{_mm512_setr_pd(q0, q1, q2, q3, q4, q5, q6, q7)} {}

  TIT_FORCE_INLINE explicit Reg(zeroinit_t /*z*/) noexcept
      : reg{_mm512_setzero_pd()} {}

  TIT_FORCE_INLINE explicit Reg(float64_t q) noexcept
      : reg{_mm512_set1_pd(q)} {}

}; // class Reg<float64_t, 8>

template<>
class alignas(64) RegMask<float64_t, 8> final {
public:

  __m512d reg; // NOLINT(*-non-private-member-variables-in-classes)

  TIT_FORCE_INLINE RegMask() = default;

  TIT_FORCE_INLINE RegMask(__m512d mask) noexcept : reg{mask} {}

  TIT_FORCE_INLINE RegMask(uint64_t q0, uint64_t q1, //
                           uint64_t q2, uint64_t q3, //
                           uint64_t q4, uint64_t q5, //
                           uint64_t q6, uint64_t q7) noexcept
      : reg{_mm512_castsi512_pd(_mm512_setr_epi64(
            to_signed(q0), to_signed(q1), to_signed(q2), to_signed(q3),
            to_signed(q4), to_signed(q5), to_signed(q6), to_signed(q7)))} {}

  TIT_FORCE_INLINE explicit RegMask(zeroinit_t /*z*/) noexcept
      : reg{_mm512_setzero_pd()} {}

  TIT_FORCE_INLINE explicit RegMask(uint64_t q) noexcept
      : reg{_mm512_castsi512_pd(_mm512_set1_epi64(to_signed(q)))} {}

}; // class RegMask<float64_t, 8>

inline namespace float64x8 {

using Reg_ = Reg<float64_t, 8>;
using RegMask_ = RegMask<float64_t, 8>;

TIT_FORCE_INLINE auto operator+(Reg_ a, Reg_ b) noexcept -> Reg_ {
  return _mm512_add_pd(a.reg, b.reg);
}

TIT_FORCE_INLINE auto operator-(Reg_ a) noexcept -> Reg_ {
  auto const sign_mask = _mm512_set1_pd(-0.0);
  return _mm512_xor_pd(sign_mask, a.reg);
}

TIT_FORCE_INLINE auto operator-(Reg_ a, Reg_ b) noexcept -> Reg_ {
  return _mm512_sub_pd(a.reg, b.reg);
}

TIT_FORCE_INLINE auto operator*(Reg_ a, Reg_ b) noexcept -> Reg_ {
  return _mm512_mul_pd(a.reg, b.reg);
}

TIT_FORCE_INLINE auto operator/(Reg_ a, Reg_ b) noexcept -> Reg_ {
  return _mm512_div_pd(a.reg, b.reg);
}

TIT_FORCE_INLINE auto fma(Reg_ a, Reg_ b, Reg_ c) noexcept -> Reg_ {
  return _mm512_fmadd_pd(a.reg, b.reg, c.reg);
}

TIT_FORCE_INLINE auto fnma(Reg_ a, Reg_ b, Reg_ c) noexcept -> Reg_ {
  return _mm512_fnmadd_pd(a.reg, b.reg, c.reg);
}

TIT_FORCE_INLINE auto abs(Reg_ a) noexcept -> Reg_ {
  auto const sign_mask = _mm512_set1_pd(-0.0);
  return _mm512_andnot_pd(sign_mask, a.reg);
}

TIT_FORCE_INLINE auto min(Reg_ a, Reg_ b) noexcept -> Reg_ {
  return _mm512_min_pd(a.reg, b.reg);
}

TIT_FORCE_INLINE auto max(Reg_ a, Reg_ b) noexcept -> Reg_ {
  return _mm512_max_pd(a.reg, b.reg);
}

TIT_FORCE_INLINE auto floor(Reg_ a) noexcept -> Reg_ {
  return _mm512_round_pd(a.reg, _MM_FROUND_TO_NEG_INF);
}

TIT_FORCE_INLINE auto round(Reg_ a) noexcept -> Reg_ {
  return _mm512_round_pd(a.reg, _MM_FROUND_TO_NEAREST_INT);
}

TIT_FORCE_INLINE auto ceil(Reg_ a) noexcept -> Reg_ {
  return _mm512_round_pd(a.reg, _MM_FROUND_TO_POS_INF);
}

TIT_FORCE_INLINE auto operator==(Reg_ a, Reg_ b) noexcept -> RegMask_ {
  return _mm512_cmp_pd_mask(a.reg, b.reg, _CMP_EQ_OQ);
}

TIT_FORCE_INLINE auto operator!=(Reg_ a, Reg_ b) noexcept -> RegMask_ {
  return _mm512_cmp_pd_mask(a.reg, b.reg, _CMP_NEQ_OQ);
}

TIT_FORCE_INLINE auto operator<(Reg_ a, Reg_ b) noexcept -> RegMask_ {
  return _mm512_cmp_pd_mask(a.reg, b.reg, _CMP_LT_OQ);
}

TIT_FORCE_INLINE auto operator<=(Reg_ a, Reg_ b) noexcept -> RegMask_ {
  return _mm512_cmp_pd_mask(a.reg, b.reg, _CMP_LE_OQ);
}

TIT_FORCE_INLINE auto operator>(Reg_ a, Reg_ b) noexcept -> RegMask_ {
  return _mm512_cmp_pd_mask(a.reg, b.reg, _CMP_GT_OQ);
}

TIT_FORCE_INLINE auto operator>=(Reg_ a, Reg_ b) noexcept -> RegMask_ {
  return _mm512_cmp_pd_mask(a.reg, b.reg, _CMP_GE_OQ);
}

TIT_FORCE_INLINE auto blend_zero(RegMask_ m, Reg_ a) noexcept -> Reg_ {
  return _mm512_and_pd(a.reg, m.reg);
}

TIT_FORCE_INLINE auto blend(RegMask_ m, Reg_ a, Reg_ b) noexcept -> Reg_ {
  return _mm512_mask_blend_pd(b.reg, a.reg, m.reg);
}

TIT_FORCE_INLINE auto split(Reg_ a) noexcept {
  using HalfReg = Reg<float64_t, 4>;
  return std::tuple{HalfReg{_mm512_castpd512_pd256(a.reg)},
                    HalfReg{_mm512_extractf64x4_pd(a.reg, 1)}};
}

} // namespace float64x8

#endif // __AVX512F__
#endif

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// NOLINTEND(*-include-cleaner)

} // namespace tit::simd

#endif // __SSE__
