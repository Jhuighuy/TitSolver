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
#if __ARM_NEON

#include <array>
#include <functional>

#include <arm_neon.h>

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
    std::array<float64_t, num_rows> _col;
    float64x2_t _reg;
  };

  constexpr Vec(float64_t qx, float64_t qy) noexcept {
    if consteval {
      _col = {qx, qy};
    } else {
      _reg = vsetq_lane_f64(qx, _reg, 0);
      _reg = vsetq_lane_f64(qy, _reg, 1);
    }
  }

  constexpr Vec(float64_t q = 0.0) noexcept {
    if consteval {
      _col = {q, q};
    } else {
      _reg = vdupq_n_f64(q);
    }
  }
  constexpr auto operator=(float64_t q) noexcept -> Vec& {
    if consteval {
      _col.fill(q);
    } else {
      _reg = vdupq_n_f64(q);
    }
    return *this;
  }

  constexpr auto operator[](size_t i) noexcept -> float64_t& {
    TIT_ASSERT(i < num_rows, "Row index is out of range.");
    return _col[i];
  }
  constexpr auto operator[](size_t i) const noexcept -> float64_t {
    TIT_ASSERT(i < num_rows, "Row index is out of range.");
    return _col[i];
  }

}; // class Vec<float64_t, 2>

TIT_VEC_SIMD_FUNC_VV(operator+, 2, float64_t, a, float64_t, b, {
  Vec<float64_t, 2> r;
  r._reg = vaddq_f64(a._reg, b._reg);
  return r;
})
TIT_VEC_SIMD_FUNC_VV(operator+=, 2, float64_t, &a, float64_t, b, {
  a._reg = vaddq_f64(a._reg, b._reg);
  return a;
})

TIT_VEC_SIMD_FUNC_V(operator-, 2, float64_t, a, {
  Vec<float64_t, 2> r;
  r._reg = vnegq_f64(a._reg);
  return r;
})
TIT_VEC_SIMD_FUNC_VV(operator-, 2, float64_t, a, float64_t, b, {
  Vec<float64_t, 2> r;
  r._reg = vsubq_f64(a._reg, b._reg);
  return r;
})
TIT_VEC_SIMD_FUNC_VV(operator-=, 2, float64_t, &a, float64_t, b, {
  a._reg = vsubq_f64(a._reg, b._reg);
  return a;
})

TIT_VEC_SIMD_FUNC_SV(operator*, 2, Num, a, float64_t, b, {
  Vec<float64_t, 2> r;
  r._reg = vmulq_f64(vdupq_n_f64(static_cast<float64_t>(a)), b._reg);
  return r;
})
TIT_VEC_SIMD_FUNC_VS(operator*, 2, float64_t, a, Num, b, {
  Vec<float64_t, 2> r;
  r._reg = vmulq_f64(a._reg, vdupq_n_f64(static_cast<float64_t>(b)));
  return r;
})
TIT_VEC_SIMD_FUNC_VV(operator*, 2, float64_t, a, float64_t, b, {
  Vec<float64_t, 2> r;
  r._reg = vmulq_f64(a._reg, b._reg);
  return r;
})
TIT_VEC_SIMD_FUNC_VS(operator*=, 2, float64_t, &a, Num, b, {
  a._reg = vmulq_f64(a._reg, vdupq_n_f64(static_cast<float64_t>(b)));
  return a;
})
TIT_VEC_SIMD_FUNC_VV(operator*=, 2, float64_t, &a, float64_t, b, {
  a._reg = vmulq_f64(a._reg, b._reg);
  return a;
})

TIT_VEC_SIMD_FUNC_VS(operator/, 2, float64_t, a, Num, b, {
  Vec<float64_t, 2> r;
  r._reg = vdivq_f64(a._reg, vdupq_n_f64(static_cast<float64_t>(b)));
  return r;
})
TIT_VEC_SIMD_FUNC_VV(operator/, 2, float64_t, a, float64_t, b, {
  Vec<float64_t, 2> r;
  r._reg = vdivq_f64(a._reg, b._reg);
  return r;
})
TIT_VEC_SIMD_FUNC_VS(operator/=, 2, float64_t, &a, Num, b, {
  a._reg = vdivq_f64(a._reg, vdupq_n_f64(static_cast<float64_t>(b)));
  return a;
})
TIT_VEC_SIMD_FUNC_VV(operator/=, 2, float64_t, &a, float64_t, b, {
  a._reg = vdivq_f64(a._reg, b._reg);
  return a;
})

TIT_VEC_SIMD_FUNC_V(sum, 2, float64_t, a, {
  return vaddvq_f64(a._reg); //
})

namespace {
  // vmvnq_u64 does not exist for some reason.
  inline auto vmvnq_u64(uint64x2_t a_reg) noexcept -> uint64x2_t {
    return vreinterpretq_u64_u32(vmvnq_u32(vreinterpretq_u32_u64(a_reg)));
  }
} // namespace

// Helper to compare two NEON registers based on the compare functor.
template<common_cmp_op Op>
auto _cmp_to_mask(VecCmp<Op, 2, float64_t> cmp) noexcept -> uint64x2_t {
  const auto& [_, x, y] = cmp;
  if constexpr (std::is_same_v<Op, std::equal_to<>>) {
    return vceqq_f64(x._reg, y._reg);
  } else if constexpr (std::is_same_v<Op, std::not_equal_to<>>) {
    return vmvnq_u64(vceqq_f64(x._reg, y._reg));
  } else if constexpr (std::is_same_v<Op, std::less<>>) {
    return vcltq_f64(x._reg, y._reg);
  } else if constexpr (std::is_same_v<Op, std::less_equal<>>) {
    return vcleq_f64(x._reg, y._reg);
  } else if constexpr (std::is_same_v<Op, std::greater<>>) {
    return vcgtq_f64(x._reg, y._reg);
  } else if constexpr (std::is_same_v<Op, std::greater_equal<>>) {
    return vcgeq_f64(x._reg, y._reg);
  }
}

// clang-format off
TIT_VEC_SIMD_MERGE(2, Op, float64_t, float64_t, cmp,
                   float64_t, a, {
  Vec<float64_t, 2> r;
  const auto mask = _cmp_to_mask(cmp);
  r._reg = vreinterpretq_f64_u64(
      vandq_u64(vreinterpretq_u64_f64(a._reg), mask));
  return r;
})
TIT_VEC_SIMD_MERGE_2(2, Op, float64_t, float64_t, cmp,
                     float64_t, a, float64_t, b, {
  Vec<float64_t, 2> r;
  const auto mask = _cmp_to_mask(cmp);
  r._reg = vreinterpretq_f64_u64(
        vorrq_u64(vandq_u64(vreinterpretq_u64_f64(a._reg), mask),
                  vandq_u64(vreinterpretq_u64_f64(b._reg), vmvnq_u64(mask))));
  return r;
})
// clang-format on

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit

#endif // __ARM_NEON
