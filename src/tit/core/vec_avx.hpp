/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
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
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once
#ifdef __SSE__

#include <array>
#include <functional>

#include <x86intrin.h>

#include "tit/core/assert.hpp"
#include "tit/core/types.hpp"
#include "tit/core/vec.hpp" // IWYU pragma: keep

namespace tit {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

template<>
class alignas(16) Vec<float64_t, 2> final {
public:

  static constexpr auto num_rows = size_t{2};

  union {
    std::array<float64_t, num_rows> row_;
    __m128d reg_;
  };

  constexpr Vec(float64_t qx, float64_t qy) noexcept {
    if consteval {
      row_ = {qx, qy};
    } else {
      reg_ = _mm_set_pd(qx, qy);
    }
  }

  constexpr Vec(float64_t q = 0.0) noexcept {
    if consteval {
      row_ = {q, q};
    } else {
      reg_ = _mm_set1_pd(q);
    }
  }
  constexpr auto operator=(float64_t q) noexcept -> Vec& {
    if consteval {
      row_.fill(q);
    } else {
      reg_ = _mm_set1_pd(q);
    }
    return *this;
  }

  constexpr auto operator[](size_t i) noexcept -> float64_t& {
    TIT_ASSERT(i < num_rows, "Row index is out of range.");
    return row_[i];
  }
  constexpr auto operator[](size_t i) const noexcept -> float64_t {
    TIT_ASSERT(i < num_rows, "Row index is out of range.");
    return row_[i];
  }

}; // class Vec<float64_t, 4>

TIT_VEC_SIMD_FUNC_VV(operator+, 2, float64_t, a, float64_t, b, {
  Vec<float64_t, 2> r;
  r.reg_ = _mm_add_pd(a.reg_, b.reg_);
  return r;
})
TIT_VEC_SIMD_FUNC_VV(operator+=, 2, float64_t, &a, float64_t, b, {
  a.reg_ = _mm_add_pd(a.reg_, b.reg_);
  return a;
})

TIT_VEC_SIMD_FUNC_V(operator-, 2, float64_t, a, {
  Vec<float64_t, 2> r;
  r.reg_ = _mm_xor_pd(_mm_set1_pd(-0.0), a.reg_);
  return r;
})
TIT_VEC_SIMD_FUNC_VV(operator-, 2, float64_t, a, float64_t, b, {
  Vec<float64_t, 2> r;
  r.reg_ = _mm_sub_pd(a.reg_, b.reg_);
  return r;
})
TIT_VEC_SIMD_FUNC_VV(operator-=, 2, float64_t, &a, float64_t, b, {
  a.reg_ = _mm_sub_pd(a.reg_, b.reg_);
  return a;
})

TIT_VEC_SIMD_FUNC_SV(operator*, 2, Num, a, float64_t, b, {
  Vec<float64_t, 2> r;
  r.reg_ = _mm_mul_pd(_mm_set1_pd(static_cast<float64_t>(a)), b.reg_);
  return r;
})
TIT_VEC_SIMD_FUNC_VS(operator*, 2, float64_t, a, Num, b, {
  Vec<float64_t, 2> r;
  r.reg_ = _mm_mul_pd(a.reg_, _mm_set1_pd(static_cast<float64_t>(b)));
  return r;
})
TIT_VEC_SIMD_FUNC_VV(operator*, 2, float64_t, a, float64_t, b, {
  Vec<float64_t, 2> r;
  r.reg_ = _mm_mul_pd(a.reg_, b.reg_);
  return r;
})
TIT_VEC_SIMD_FUNC_VS(operator*=, 2, float64_t, &a, Num, b, {
  a.reg_ = _mm_mul_pd(a.reg_, _mm_set1_pd(static_cast<float64_t>(b)));
  return a;
})
TIT_VEC_SIMD_FUNC_VV(operator*=, 2, float64_t, &a, float64_t, b, {
  a.reg_ = _mm_mul_pd(a.reg_, b.reg_);
  return a;
})

TIT_VEC_SIMD_FUNC_VS(operator/, 2, float64_t, a, Num, b, {
  Vec<float64_t, 2> r;
  r.reg_ = _mm_div_pd(a.reg_, _mm_set1_pd(static_cast<float64_t>(b)));
  return r;
})
TIT_VEC_SIMD_FUNC_VV(operator/, 2, float64_t, a, float64_t, b, {
  Vec<float64_t, 2> r;
  r.reg_ = _mm_div_pd(a.reg_, b.reg_);
  return r;
})
TIT_VEC_SIMD_FUNC_VS(operator/=, 2, float64_t, &a, Num, b, {
  a.reg_ = _mm_div_pd(a.reg_, _mm_set1_pd(static_cast<float64_t>(b)));
  return a;
})
TIT_VEC_SIMD_FUNC_VV(operator/=, 2, float64_t, &a, float64_t, b, {
  a.reg_ = _mm_div_pd(a.reg_, b.reg_);
  return a;
})

TIT_VEC_SIMD_FUNC_V(sum, 2, float64_t, a, {
  const auto reverse = _mm_unpackhi_pd(a.reg_, a.reg_);
  return _mm_cvtsd_f64(_mm_add_sd(a.reg_, reverse));
})

TIT_VEC_SIMD_FUNC_VV(dot, 2, float64_t, a, float64_t, b, {
  return _mm_cvtsd_f64(_mm_dp_pd(a.reg_, b.reg_, 0b00110001));
})

// Helper to compare two SSE registers based on the compare functor.
template<common_cmp_op Op>
auto _cmp_to_mask(VecCmp<Op, 2, float64_t> cmp) noexcept -> __m128d {
  const auto& [_, x, y] = cmp;
  if constexpr (std::is_same_v<Op, std::equal_to<>>) {
    return _mm_cmp_pd(x.reg_, y.reg_, _CMP_EQ_OQ);
  } else if constexpr (std::is_same_v<Op, std::not_equal_to<>>) {
    return _mm_cmp_pd(x.reg_, y.reg_, _CMP_NEQ_OQ);
  } else if constexpr (std::is_same_v<Op, std::less<>>) {
    return _mm_cmp_pd(x.reg_, y.reg_, _CMP_LT_OQ);
  } else if constexpr (std::is_same_v<Op, std::less_equal<>>) {
    return _mm_cmp_pd(x.reg_, y.reg_, _CMP_LE_OQ);
  } else if constexpr (std::is_same_v<Op, std::greater<>>) {
    return _mm_cmp_pd(x.reg_, y.reg_, _CMP_GT_OQ);
  } else if constexpr (std::is_same_v<Op, std::greater_equal<>>) {
    return _mm_cmp_pd(x.reg_, y.reg_, _CMP_GE_OQ);
  }
}

// clang-format off
TIT_VEC_SIMD_MERGE(2, Op, float64_t, float64_t, cmp,
                   float64_t, a, {
  Vec<float64_t, 2> r;
  const auto mask = _cmp_to_mask(cmp);
  r.reg_ = _mm_and_pd(mask, a.reg_);
  return r;
})
TIT_VEC_SIMD_MERGE_2(2, Op, float64_t, float64_t, cmp,
                     float64_t, a, float64_t, b, {
  Vec<float64_t, 2> r;
  const auto mask = _cmp_to_mask(cmp);
  // Falsy value comes first.
  r.reg_ = _mm_blend_pd(b.reg_, a.reg_, mask);
  return r;
})
// clang-format on

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#ifdef __AVX__

template<>
class alignas(32) Vec<float64_t, 4> final {
public:

  static constexpr auto num_rows = size_t{4};

  union {
    std::array<float64_t, num_rows> row_;
    __m256d reg_;
  };

  constexpr Vec(float64_t qx, float64_t qy, //
                float64_t qz, float64_t qw) noexcept {
    if consteval {
      row_ = {qx, qy, qz, qw};
    } else {
      reg_ = _mm256_set_pd(qx, qy, qz, qw);
    }
  }

  constexpr Vec(float64_t q = 0.0) noexcept {
    if consteval {
      row_ = {q, q, q, q};
    } else {
      reg_ = _mm256_set1_pd(q);
    }
  }
  constexpr auto operator=(float64_t q) noexcept -> Vec& {
    if consteval {
      row_.fill(q);
    } else {
      reg_ = _mm256_set1_pd(q);
    }
    return *this;
  }

  constexpr auto operator[](size_t i) noexcept -> float64_t& {
    TIT_ASSERT(i < num_rows, "Row index is out of range.");
    return row_[i];
  }
  constexpr auto operator[](size_t i) const noexcept -> float64_t {
    TIT_ASSERT(i < num_rows, "Row index is out of range.");
    return row_[i];
  }

}; // class Vec<float64_t, 4>

TIT_VEC_SIMD_FUNC_VV(operator+, 4, float64_t, a, float64_t, b, {
  Vec<float64_t, 4> r;
  r.reg_ = _mm256_add_pd(a.reg_, b.reg_);
  return r;
})
TIT_VEC_SIMD_FUNC_VV(operator+=, 4, float64_t, &a, float64_t, b, {
  a.reg_ = _mm256_add_pd(a.reg_, b.reg_);
  return a;
})

TIT_VEC_SIMD_FUNC_V(operator-, 4, float64_t, a, {
  Vec<float64_t, 4> r;
  r.reg_ = _mm256_xor_pd(_mm256_set1_pd(-0.0), a.reg_);
  return r;
})
TIT_VEC_SIMD_FUNC_VV(operator-, 4, float64_t, a, float64_t, b, {
  Vec<float64_t, 4> r;
  r.reg_ = _mm256_sub_pd(a.reg_, b.reg_);
  return r;
})
TIT_VEC_SIMD_FUNC_VV(operator-=, 4, float64_t, &a, float64_t, b, {
  a.reg_ = _mm256_sub_pd(a.reg_, b.reg_);
  return a;
})

TIT_VEC_SIMD_FUNC_SV(operator*, 4, Num, a, float64_t, b, {
  Vec<float64_t, 4> r;
  r.reg_ = _mm256_mul_pd(_mm256_set1_pd(static_cast<float64_t>(a)), b.reg_);
  return r;
})
TIT_VEC_SIMD_FUNC_VS(operator*, 4, float64_t, a, Num, b, {
  Vec<float64_t, 4> r;
  r.reg_ = _mm256_mul_pd(a.reg_, _mm256_set1_pd(static_cast<float64_t>(b)));
  return r;
})
TIT_VEC_SIMD_FUNC_VV(operator*, 4, float64_t, a, float64_t, b, {
  Vec<float64_t, 4> r;
  r.reg_ = _mm256_mul_pd(a.reg_, b.reg_);
  return r;
})
TIT_VEC_SIMD_FUNC_VS(operator*=, 4, float64_t, &a, Num, b, {
  a.reg_ = _mm256_mul_pd(a.reg_, _mm256_set1_pd(static_cast<float64_t>(b)));
  return a;
})
TIT_VEC_SIMD_FUNC_VV(operator*=, 4, float64_t, &a, float64_t, b, {
  a.reg_ = _mm256_mul_pd(a.reg_, b.reg_);
  return a;
})

TIT_VEC_SIMD_FUNC_VS(operator/, 4, float64_t, a, Num, b, {
  Vec<float64_t, 4> r;
  r.reg_ = _mm256_div_pd(a.reg_, _mm256_set1_pd(static_cast<float64_t>(b)));
  return r;
})
TIT_VEC_SIMD_FUNC_VV(operator/, 4, float64_t, a, float64_t, b, {
  Vec<float64_t, 4> r;
  r.reg_ = _mm256_div_pd(a.reg_, b.reg_);
  return r;
})
TIT_VEC_SIMD_FUNC_VS(operator/=, 4, float64_t, &a, Num, b, {
  a.reg_ = _mm256_div_pd(a.reg_, _mm256_set1_pd(static_cast<float64_t>(b)));
  return a;
})
TIT_VEC_SIMD_FUNC_VV(operator/=, 4, float64_t, &a, float64_t, b, {
  a.reg_ = _mm256_div_pd(a.reg_, b.reg_);
  return a;
})

TIT_VEC_SIMD_FUNC_V(sum, 4, float64_t, a, {
  const auto a_low = _mm256_castpd256_pd128(a.reg_);
  const auto a_high = _mm256_extractf128_pd(a.reg_, 1);
  const auto partial = _mm_add_pd(a_low, a_high);
  const auto partial_reverse = _mm_unpackhi_pd(partial, partial);
  return _mm_cvtsd_f64(_mm_add_sd(partial, partial_reverse));
})

// Helper to compare two AVX registers based on the compare functor.
template<common_cmp_op Op>
auto _cmp_to_mask(VecCmp<Op, 4, float64_t> cmp) noexcept -> __m256d {
  const auto& [_, x, y] = cmp;
  if constexpr (std::is_same_v<Op, std::equal_to<>>) {
    return _mm256_cmp_pd(x.reg_, y.reg_, _CMP_EQ_OQ);
  } else if constexpr (std::is_same_v<Op, std::not_equal_to<>>) {
    return _mm256_cmp_pd(x.reg_, y.reg_, _CMP_NEQ_OQ);
  } else if constexpr (std::is_same_v<Op, std::less<>>) {
    return _mm256_cmp_pd(x.reg_, y.reg_, _CMP_LT_OQ);
  } else if constexpr (std::is_same_v<Op, std::less_equal<>>) {
    return _mm256_cmp_pd(x.reg_, y.reg_, _CMP_LE_OQ);
  } else if constexpr (std::is_same_v<Op, std::greater<>>) {
    return _mm256_cmp_pd(x.reg_, y.reg_, _CMP_GT_OQ);
  } else if constexpr (std::is_same_v<Op, std::greater_equal<>>) {
    return _mm256_cmp_pd(x.reg_, y.reg_, _CMP_GE_OQ);
  }
}

// clang-format off
TIT_VEC_SIMD_MERGE(4, Op, float64_t, float64_t, cmp,
                   float64_t, a, {
  Vec<float64_t, 4> r;
  const auto mask = _cmp_to_mask(cmp);
  r.reg_ = _mm256_and_pd(mask, a.reg_);
  return r;
})
TIT_VEC_SIMD_MERGE_2(4, Op, float64_t, float64_t, cmp,
                     float64_t, a, float64_t, b, {
  Vec<float64_t, 4> r;
  const auto mask = _cmp_to_mask(cmp);
  // Falsy value comes first.
  r.reg_ = _mm256_blend_pd(b.reg_, a.reg_, mask);
  return r;
})
// clang-format on

#endif // __AVX__

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#ifdef __AVX512F__

// TODO: implement me!
template<>
class alignas(64) Vec<float64_t, 8> final {
public:

  static constexpr auto num_rows = size_t{8};

}; // Vec<float64_t, 8>

#endif // __AVX512F__

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit

#endif // __SSE__
