/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once
#if defined(__ARM_NEON) && (!TIT_IWYU)

#include <arm_neon.h>

#include <array>
#include <functional>

#include "tit/core/assert.hpp"
#include "tit/core/config.hpp"
#include "tit/core/types.hpp"
#include "tit/core/vec.hpp" // IWYU pragma: keep

namespace tit {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

template<>
class alignas(16) Vec<float64_t, 2> final {
public:

  static constexpr auto num_rows = 2UZ;

  // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
  union {
    std::array<float64_t, num_rows> col_;
    float64x2_t reg_;
  };
  // NOLINTEND(misc-non-private-member-variables-in-classes)

  constexpr Vec(float64_t qx, float64_t qy) noexcept {
    if consteval {
      col_ = {qx, qy};
    } else {
      const auto qs = std::array{qx, qy};
      reg_ = vld1q_f64(qs.data());
    }
  }

  constexpr explicit Vec(float64_t q = 0.0) noexcept {
    if consteval {
      col_ = {q, q};
    } else {
      reg_ = vdupq_n_f64(q);
    }
  }
  constexpr auto operator=(float64_t q) noexcept -> Vec& {
    if consteval {
      col_.fill(q);
    } else {
      reg_ = vdupq_n_f64(q);
    }
    return *this;
  }

  constexpr auto operator[](size_t i) noexcept -> float64_t& {
    TIT_ASSERT(i < num_rows, "Row index is out of range.");
    return col_[i];
  }
  constexpr auto operator[](size_t i) const noexcept -> float64_t {
    TIT_ASSERT(i < num_rows, "Row index is out of range.");
    return col_[i];
  }

}; // class Vec<float64_t, 2>

TIT_VEC_SIMD_FUNC_VV(operator+, 2, float64_t, a, float64_t, b, {
  Vec<float64_t, 2> r;
  r.reg_ = vaddq_f64(a.reg_, b.reg_);
  return r;
})
TIT_VEC_SIMD_FUNC_VV(&operator+=, 2, float64_t, &a, float64_t, b, {
  a.reg_ = vaddq_f64(a.reg_, b.reg_);
  return a;
})

TIT_VEC_SIMD_FUNC_V(operator-, 2, float64_t, a, {
  Vec<float64_t, 2> r;
  r.reg_ = vnegq_f64(a.reg_);
  return r;
})
TIT_VEC_SIMD_FUNC_VV(operator-, 2, float64_t, a, float64_t, b, {
  Vec<float64_t, 2> r;
  r.reg_ = vsubq_f64(a.reg_, b.reg_);
  return r;
})
TIT_VEC_SIMD_FUNC_VV(&operator-=, 2, float64_t, &a, float64_t, b, {
  a.reg_ = vsubq_f64(a.reg_, b.reg_);
  return a;
})

TIT_VEC_SIMD_FUNC_SV(operator*, 2, Num, a, float64_t, b, {
  Vec<float64_t, 2> r;
  r.reg_ = vmulq_f64(vdupq_n_f64(static_cast<float64_t>(a)), b.reg_);
  return r;
})
TIT_VEC_SIMD_FUNC_VS(operator*, 2, float64_t, a, Num, b, {
  Vec<float64_t, 2> r;
  r.reg_ = vmulq_f64(a.reg_, vdupq_n_f64(static_cast<float64_t>(b)));
  return r;
})
TIT_VEC_SIMD_FUNC_VV(operator*, 2, float64_t, a, float64_t, b, {
  Vec<float64_t, 2> r;
  r.reg_ = vmulq_f64(a.reg_, b.reg_);
  return r;
})
TIT_VEC_SIMD_FUNC_VS(&operator*=, 2, float64_t, &a, Num, b, {
  a.reg_ = vmulq_f64(a.reg_, vdupq_n_f64(static_cast<float64_t>(b)));
  return a;
})
TIT_VEC_SIMD_FUNC_VV(&operator*=, 2, float64_t, &a, float64_t, b, {
  a.reg_ = vmulq_f64(a.reg_, b.reg_);
  return a;
})

TIT_VEC_SIMD_FUNC_VS(operator/, 2, float64_t, a, Num, b, {
  Vec<float64_t, 2> r;
  r.reg_ = vdivq_f64(a.reg_, vdupq_n_f64(static_cast<float64_t>(b)));
  return r;
})
TIT_VEC_SIMD_FUNC_VV(operator/, 2, float64_t, a, float64_t, b, {
  Vec<float64_t, 2> r;
  r.reg_ = vdivq_f64(a.reg_, b.reg_);
  return r;
})
TIT_VEC_SIMD_FUNC_VS(&operator/=, 2, float64_t, &a, Num, b, {
  a.reg_ = vdivq_f64(a.reg_, vdupq_n_f64(static_cast<float64_t>(b)));
  return a;
})
TIT_VEC_SIMD_FUNC_VV(&operator/=, 2, float64_t, &a, float64_t, b, {
  a.reg_ = vdivq_f64(a.reg_, b.reg_);
  return a;
})

TIT_VEC_SIMD_FUNC_V(floor, 2, float64_t, a, {
  Vec<float64_t, 2> r;
  r.reg_ = vrndmq_f64(a.reg_);
  return r;
})
TIT_VEC_SIMD_FUNC_V(round, 2, float64_t, a, {
  Vec<float64_t, 2> r;
  r.reg_ = vrndnq_f64(a.reg_);
  return r;
})
TIT_VEC_SIMD_FUNC_V(ceil, 2, float64_t, a, {
  Vec<float64_t, 2> r;
  r.reg_ = vrndpq_f64(a.reg_);
  return r;
})

TIT_VEC_SIMD_FUNC_V(sum, 2, float64_t, a, {
  return vaddvq_f64(a.reg_); //
})

// vmvnq_u64 does not exist for some reason.
inline auto vmvnq_u64(uint64x2_t a_reg) noexcept -> uint64x2_t {
  return vreinterpretq_u64_u32(vmvnq_u32(vreinterpretq_u32_u64(a_reg)));
}

// Helper to compare two NEON registers based on the compare functor.
template<common_cmp_op Op>
auto _cmp_to_mask(VecCmp<Op, 2, float64_t> cmp) noexcept -> uint64x2_t {
  const auto& [_, x, y] = cmp;
  if constexpr (std::is_same_v<Op, std::equal_to<>>) {
    return vceqq_f64(x.reg_, y.reg_);
  } else if constexpr (std::is_same_v<Op, std::not_equal_to<>>) {
    return vmvnq_u64(vceqq_f64(x.reg_, y.reg_));
  } else if constexpr (std::is_same_v<Op, std::less<>>) {
    return vcltq_f64(x.reg_, y.reg_);
  } else if constexpr (std::is_same_v<Op, std::less_equal<>>) {
    return vcleq_f64(x.reg_, y.reg_);
  } else if constexpr (std::is_same_v<Op, std::greater<>>) {
    return vcgtq_f64(x.reg_, y.reg_);
  } else if constexpr (std::is_same_v<Op, std::greater_equal<>>) {
    return vcgeq_f64(x.reg_, y.reg_);
  }
}

TIT_VEC_SIMD_MERGE(2, Op, float64_t, float64_t, cmp, float64_t, a, {
  Vec<float64_t, 2> r;
  const auto mask = _cmp_to_mask(cmp);
  r.reg_ = vreinterpretq_f64_u64( //
      vandq_u64(vreinterpretq_u64_f64(a.reg_), mask));
  return r;
})
// clang-format off
TIT_VEC_SIMD_MERGE_2(2, Op, float64_t, float64_t, cmp,
                     float64_t, a, float64_t, b, {
  Vec<float64_t, 2> r;
  const auto mask = _cmp_to_mask(cmp);
  r.reg_ = vreinterpretq_f64_u64(
        vorrq_u64(vandq_u64(vreinterpretq_u64_f64(a.reg_), mask),
                  vandq_u64(vreinterpretq_u64_f64(b.reg_), vmvnq_u64(mask))));
  return r;
})
// clang-format on

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit

#endif // __ARM_NEON
