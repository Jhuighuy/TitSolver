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
#include "tit/core/simd/reg.hpp"
#include "tit/core/uint_utils.hpp"

namespace tit::simd {

// NOLINTBEGIN(*-include-cleaner)

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

template<>
class alignas(16) Reg<float64_t, 2> final {
public:

  __m128d data; // NOLINT(*-non-private-member-variables-in-classes)

  [[gnu::always_inline]]
  Reg() noexcept
      : data{_mm_setzero_pd()} {}

  [[gnu::always_inline]]
  Reg(__m128d r) noexcept
      : data{r} {}

  [[gnu::always_inline]]
  explicit Reg(float64_t q) noexcept
      : data{_mm_set1_pd(q)} {}

}; // class Reg<float64_t, 2>

template<>
class alignas(16) RegMask<float64_t, 2> final {
public:

  __m128d data; // NOLINT(*-non-private-member-variables-in-classes)

  [[gnu::always_inline]]
  RegMask() noexcept
      : data{_mm_setzero_pd()} {}

  [[gnu::always_inline]]
  RegMask(__m128d r) noexcept
      : data{r} {}

  [[gnu::always_inline]]
  explicit RegMask(uint64_t q) noexcept
      : data{_mm_castsi128_pd(_mm_set1_epi64(_mm_cvtsi64_m64(to_signed(q))))} {}

}; // class RegMask<float64_t, 2>

inline namespace float64x2 {

using Reg_ = Reg<float64_t, 2>;
using RegMask_ = RegMask<float64_t, 2>;

[[gnu::always_inline]]
inline auto operator+(Reg_ a, Reg_ b) noexcept -> Reg_ {
  return _mm_add_pd(a.data, b.data);
}

[[gnu::always_inline]]
inline auto operator-(Reg_ a) noexcept -> Reg_ {
  auto const sign_mask = _mm_set1_pd(-0.0);
  return _mm_xor_pd(sign_mask, a.data);
}

[[gnu::always_inline]]
inline auto operator-(Reg_ a, Reg_ b) noexcept -> Reg_ {
  return _mm_sub_pd(a.data, b.data);
}

[[gnu::always_inline]]
inline auto operator*(Reg_ a, Reg_ b) noexcept -> Reg_ {
  return _mm_mul_pd(a.data, b.data);
}

[[gnu::always_inline]]
inline auto operator/(Reg_ a, Reg_ b) noexcept -> Reg_ {
  return _mm_div_pd(a.data, b.data);
}

#ifdef __FMA__

[[gnu::always_inline]]
inline auto fma(Reg_ a, Reg_ b, Reg_ c) noexcept -> Reg_ {
  return _mm_fmadd_pd(a.data, b.data, c.data);
}

[[gnu::always_inline]]
inline auto fnma(Reg_ a, Reg_ b, Reg_ c) noexcept -> Reg_ {
  return _mm_fnmadd_pd(a.data, b.data, c.data);
}

#endif // ifdef __FMA__

[[gnu::always_inline]]
inline auto abs(Reg_ a) noexcept -> Reg_ {
  auto const sign_mask = _mm_set1_pd(-0.0);
  return _mm_andnot_pd(sign_mask, a.data);
}

[[gnu::always_inline]]
inline auto min(Reg_ a, Reg_ b) noexcept -> Reg_ {
  return _mm_min_pd(a.data, b.data);
}

[[gnu::always_inline]]
inline auto max(Reg_ a, Reg_ b) noexcept -> Reg_ {
  return _mm_max_pd(a.data, b.data);
}

[[gnu::always_inline]]
inline auto floor(Reg_ a) noexcept -> Reg_ {
  return _mm_round_pd(a.data, _MM_FROUND_TO_NEG_INF);
}

[[gnu::always_inline]]
inline auto round(Reg_ a) noexcept -> Reg_ {
  return _mm_round_pd(a.data, _MM_FROUND_TO_NEAREST_INT);
}

[[gnu::always_inline]]
inline auto ceil(Reg_ a) noexcept -> Reg_ {
  return _mm_round_pd(a.data, _MM_FROUND_TO_POS_INF);
}

[[gnu::always_inline]]
inline auto operator==(Reg_ a, Reg_ b) noexcept -> RegMask_ {
  return _mm_cmp_pd(a.data, b.data, _CMP_EQ_OQ);
}

[[gnu::always_inline]]
inline auto operator!=(Reg_ a, Reg_ b) noexcept -> RegMask_ {
  return _mm_cmp_pd(a.data, b.data, _CMP_NEQ_OQ);
}

[[gnu::always_inline]]
inline auto operator<(Reg_ a, Reg_ b) noexcept -> RegMask_ {
  return _mm_cmp_pd(a.data, b.data, _CMP_LT_OQ);
}

[[gnu::always_inline]]
inline auto operator<=(Reg_ a, Reg_ b) noexcept -> RegMask_ {
  return _mm_cmp_pd(a.data, b.data, _CMP_LE_OQ);
}

[[gnu::always_inline]]
inline auto operator>(Reg_ a, Reg_ b) noexcept -> RegMask_ {
  return _mm_cmp_pd(a.data, b.data, _CMP_GT_OQ);
}

[[gnu::always_inline]]
inline auto operator>=(Reg_ a, Reg_ b) noexcept -> RegMask_ {
  return _mm_cmp_pd(a.data, b.data, _CMP_GE_OQ);
}

[[gnu::always_inline]]
inline auto filter(RegMask_ m, Reg_ a) noexcept -> Reg_ {
  return _mm_and_pd(a.data, m.data);
}

[[gnu::always_inline]]
inline auto blend(RegMask_ m, Reg_ a, Reg_ b) noexcept -> Reg_ {
  return _mm_blendv_pd(b.data, a.data, m.data);
}

[[gnu::always_inline]]
inline auto sum(Reg_ a) noexcept -> float64_t {
  auto const a_hi = _mm_unpackhi_pd(a.data, a.data);
  return _mm_cvtsd_f64(_mm_add_sd(a.data, a_hi));
}

[[gnu::always_inline]]
inline auto min_value(Reg_ a) noexcept -> float64_t {
  auto const a_hi = _mm_unpackhi_pd(a.data, a.data);
  return _mm_cvtsd_f64(_mm_min_sd(a.data, a_hi));
}

[[gnu::always_inline]]
inline auto max_value(Reg_ a) noexcept -> float64_t {
  auto const a_hi = _mm_unpackhi_pd(a.data, a.data);
  return _mm_cvtsd_f64(_mm_max_sd(a.data, a_hi));
}

} // namespace float64x2

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#ifdef __AVX__

template<>
class alignas(32) Reg<float64_t, 4> final {
public:

  __m256d data; // NOLINT(*-non-private-member-variables-in-classes)

  [[gnu::always_inline]]
  Reg() noexcept
      : data{_mm256_setzero_pd()} {}

  [[gnu::always_inline]]
  Reg(__m256d r) noexcept
      : data{r} {}

  [[gnu::always_inline]]
  explicit Reg(float64_t q) noexcept
      : data{_mm256_set1_pd(q)} {}

}; // class Reg<float64_t, 4>

template<>
class alignas(32) RegMask<float64_t, 4> final {
public:

  __m256d data; // NOLINT(*-non-private-member-variables-in-classes)

  [[gnu::always_inline]]
  RegMask() noexcept
      : data{_mm256_setzero_pd()} {}

  [[gnu::always_inline]]
  RegMask(__m256d r) noexcept
      : data{r} {}

  [[gnu::always_inline]]
  explicit RegMask(uint64_t q) noexcept
      : data{_mm256_castsi256_pd(_mm256_set1_epi64x(to_signed(q)))} {}

}; // class RegMask<float64_t, 4>

inline namespace float64x4 {

using Reg_ = Reg<float64_t, 4>;
using RegMask_ = RegMask<float64_t, 4>;

[[gnu::always_inline]]
inline auto operator+(Reg_ a, Reg_ b) noexcept -> Reg_ {
  return _mm256_add_pd(a.data, b.data);
}

[[gnu::always_inline]]
inline auto operator-(Reg_ a) noexcept -> Reg_ {
  auto const sign_mask = _mm256_set1_pd(-0.0);
  return _mm256_xor_pd(sign_mask, a.data);
}

[[gnu::always_inline]]
inline auto operator-(Reg_ a, Reg_ b) noexcept -> Reg_ {
  return _mm256_sub_pd(a.data, b.data);
}

[[gnu::always_inline]]
inline auto operator*(Reg_ a, Reg_ b) noexcept -> Reg_ {
  return _mm256_mul_pd(a.data, b.data);
}

[[gnu::always_inline]]
inline auto operator/(Reg_ a, Reg_ b) noexcept -> Reg_ {
  return _mm256_div_pd(a.data, b.data);
}

#ifdef __FMA__

[[gnu::always_inline]]
inline auto fma(Reg_ a, Reg_ b, Reg_ c) noexcept -> Reg_ {
  return _mm256_fmadd_pd(a.data, b.data, c.data);
}

[[gnu::always_inline]]
inline auto fnma(Reg_ a, Reg_ b, Reg_ c) noexcept -> Reg_ {
  return _mm256_fnmadd_pd(a.data, b.data, c.data);
}

#endif // ifdef __FMA__

[[gnu::always_inline]]
inline auto abs(Reg_ a) noexcept -> Reg_ {
  auto const sign_mask = _mm256_set1_pd(-0.0);
  return _mm256_andnot_pd(sign_mask, a.data);
}

[[gnu::always_inline]]
inline auto min(Reg_ a, Reg_ b) noexcept -> Reg_ {
  return _mm256_min_pd(a.data, b.data);
}

[[gnu::always_inline]]
inline auto max(Reg_ a, Reg_ b) noexcept -> Reg_ {
  return _mm256_max_pd(a.data, b.data);
}

[[gnu::always_inline]]
inline auto floor(Reg_ a) noexcept -> Reg_ {
  return _mm256_round_pd(a.data, _MM_FROUND_TO_NEG_INF);
}

[[gnu::always_inline]]
inline auto round(Reg_ a) noexcept -> Reg_ {
  return _mm256_round_pd(a.data, _MM_FROUND_TO_NEAREST_INT);
}

[[gnu::always_inline]]
inline auto ceil(Reg_ a) noexcept -> Reg_ {
  return _mm256_round_pd(a.data, _MM_FROUND_TO_POS_INF);
}

[[gnu::always_inline]]
inline auto operator==(Reg_ a, Reg_ b) noexcept -> RegMask_ {
  return _mm256_cmp_pd(a.data, b.data, _CMP_EQ_OQ);
}

[[gnu::always_inline]]
inline auto operator!=(Reg_ a, Reg_ b) noexcept -> RegMask_ {
  return _mm256_cmp_pd(a.data, b.data, _CMP_NEQ_OQ);
}

[[gnu::always_inline]]
inline auto operator<(Reg_ a, Reg_ b) noexcept -> RegMask_ {
  return _mm256_cmp_pd(a.data, b.data, _CMP_LT_OQ);
}

[[gnu::always_inline]]
inline auto operator<=(Reg_ a, Reg_ b) noexcept -> RegMask_ {
  return _mm256_cmp_pd(a.data, b.data, _CMP_LE_OQ);
}

[[gnu::always_inline]]
inline auto operator>(Reg_ a, Reg_ b) noexcept -> RegMask_ {
  return _mm256_cmp_pd(a.data, b.data, _CMP_GT_OQ);
}

[[gnu::always_inline]]
inline auto operator>=(Reg_ a, Reg_ b) noexcept -> RegMask_ {
  return _mm256_cmp_pd(a.data, b.data, _CMP_GE_OQ);
}

[[gnu::always_inline]]
inline auto filter(RegMask_ m, Reg_ a) noexcept -> Reg_ {
  return _mm256_and_pd(a.data, m.data);
}

[[gnu::always_inline]]
inline auto blend(RegMask_ m, Reg_ a, Reg_ b) noexcept -> Reg_ {
  return _mm256_blendv_pd(b.data, a.data, m.data);
}

[[gnu::always_inline]]
inline auto split(Reg_ a) noexcept {
  using HalfReg = Reg<float64_t, 2>;
  return std::tuple{HalfReg{_mm256_castpd256_pd128(a.data)},
                    HalfReg{_mm256_extractf128_pd(a.data, 1)}};
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

  __m512d data; // NOLINT(*-non-private-member-variables-in-classes)

  [[gnu::always_inline]]
  Reg() noexcept
      : data{_mm512_setzero_pd()} {}

  [[gnu::always_inline]]
  Reg(__m512d r) noexcept
      : data{r} {}

  [[gnu::always_inline]]
  explicit Reg(float64_t q) noexcept
      : data{_mm512_set1_pd(q)} {}

}; // class Reg<float64_t, 8>

template<>
class alignas(64) RegMask<float64_t, 8> final {
public:

  __m512d data; // NOLINT(*-non-private-member-variables-in-classes)

  [[gnu::always_inline]]
  RegMask() noexcept
      : data{_mm512_setzero_pd()} {}

  [[gnu::always_inline]]
  RegMask(__m512d mask) noexcept
      : data{mask} {}

  [[gnu::always_inline]]
  explicit RegMask(uint64_t q) noexcept
      : data{_mm512_castsi512_pd(_mm512_set1_epi64(to_signed(q)))} {}

}; // class RegMask<float64_t, 8>

inline namespace float64x8 {

using Reg_ = Reg<float64_t, 8>;
using RegMask_ = RegMask<float64_t, 8>;

[[gnu::always_inline]]
inline auto operator+(Reg_ a, Reg_ b) noexcept -> Reg_ {
  return _mm512_add_pd(a.data, b.data);
}

[[gnu::always_inline]]
inline auto operator-(Reg_ a) noexcept -> Reg_ {
  auto const sign_mask = _mm512_set1_pd(-0.0);
  return _mm512_xor_pd(sign_mask, a.data);
}

[[gnu::always_inline]]
inline auto operator-(Reg_ a, Reg_ b) noexcept -> Reg_ {
  return _mm512_sub_pd(a.data, b.data);
}

[[gnu::always_inline]]
inline auto operator*(Reg_ a, Reg_ b) noexcept -> Reg_ {
  return _mm512_mul_pd(a.data, b.data);
}

[[gnu::always_inline]]
inline auto operator/(Reg_ a, Reg_ b) noexcept -> Reg_ {
  return _mm512_div_pd(a.data, b.data);
}

[[gnu::always_inline]]
inline auto fma(Reg_ a, Reg_ b, Reg_ c) noexcept -> Reg_ {
  return _mm512_fmadd_pd(a.data, b.data, c.data);
}

[[gnu::always_inline]]
inline auto fnma(Reg_ a, Reg_ b, Reg_ c) noexcept -> Reg_ {
  return _mm512_fnmadd_pd(a.data, b.data, c.data);
}

[[gnu::always_inline]]
inline auto abs(Reg_ a) noexcept -> Reg_ {
  auto const sign_mask = _mm512_set1_pd(-0.0);
  return _mm512_andnot_pd(sign_mask, a.data);
}

[[gnu::always_inline]]
inline auto min(Reg_ a, Reg_ b) noexcept -> Reg_ {
  return _mm512_min_pd(a.data, b.data);
}

[[gnu::always_inline]]
inline auto max(Reg_ a, Reg_ b) noexcept -> Reg_ {
  return _mm512_max_pd(a.data, b.data);
}

[[gnu::always_inline]]
inline auto floor(Reg_ a) noexcept -> Reg_ {
  return _mm512_round_pd(a.data, _MM_FROUND_TO_NEG_INF);
}

[[gnu::always_inline]]
inline auto round(Reg_ a) noexcept -> Reg_ {
  return _mm512_round_pd(a.data, _MM_FROUND_TO_NEAREST_INT);
}

[[gnu::always_inline]]
inline auto ceil(Reg_ a) noexcept -> Reg_ {
  return _mm512_round_pd(a.data, _MM_FROUND_TO_POS_INF);
}

[[gnu::always_inline]]
inline auto operator==(Reg_ a, Reg_ b) noexcept -> RegMask_ {
  return _mm512_cmp_pd_mask(a.data, b.data, _CMP_EQ_OQ);
}

[[gnu::always_inline]]
inline auto operator!=(Reg_ a, Reg_ b) noexcept -> RegMask_ {
  return _mm512_cmp_pd_mask(a.data, b.data, _CMP_NEQ_OQ);
}

[[gnu::always_inline]]
inline auto operator<(Reg_ a, Reg_ b) noexcept -> RegMask_ {
  return _mm512_cmp_pd_mask(a.data, b.data, _CMP_LT_OQ);
}

[[gnu::always_inline]]
inline auto operator<=(Reg_ a, Reg_ b) noexcept -> RegMask_ {
  return _mm512_cmp_pd_mask(a.data, b.data, _CMP_LE_OQ);
}

[[gnu::always_inline]]
inline auto operator>(Reg_ a, Reg_ b) noexcept -> RegMask_ {
  return _mm512_cmp_pd_mask(a.data, b.data, _CMP_GT_OQ);
}

[[gnu::always_inline]]
inline auto operator>=(Reg_ a, Reg_ b) noexcept -> RegMask_ {
  return _mm512_cmp_pd_mask(a.data, b.data, _CMP_GE_OQ);
}

[[gnu::always_inline]]
inline auto filter(RegMask_ m, Reg_ a) noexcept -> Reg_ {
  return _mm512_and_pd(a.data, m.data);
}

[[gnu::always_inline]]
inline auto blend(RegMask_ m, Reg_ a, Reg_ b) noexcept -> Reg_ {
  return _mm512_mask_blend_pd(b.data, a.data, m.data);
}

[[gnu::always_inline]]
inline auto split(Reg_ a) noexcept {
  using HalfReg = Reg<float64_t, 4>;
  return std::tuple{HalfReg{_mm512_castpd512_pd256(a.data)},
                    HalfReg{_mm512_extractf64x4_pd(a.data, 1)}};
}

} // namespace float64x8

#endif // __AVX512F__
#endif

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// NOLINTEND(*-include-cleaner)

} // namespace tit::simd

#endif // __SSE__
