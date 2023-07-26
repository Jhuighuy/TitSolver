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

#include <algorithm>
#include <array>
#include <concepts>
#include <utility>

#include "tit/core/assert.hpp"
#include "tit/core/config.hpp"
#include "tit/core/math.hpp"
#include "tit/core/types.hpp"
#include "tit/core/vec.hpp" // IWYU pragma: keep

namespace tit {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

// Set up the default register size.
// Assuming 128-bit SIMD registers by default.
#if defined(__AVX__)
#define TIT_SIMD_REGISTER_SIZE (4 * sizeof(double))
#elif defined(__AVX512F__)
#define TIT_SIMD_REGISTER_SIZE (8 * sizeof(double))
#elif defined(__ARM_NEON) || defined(__SSE__)
#define TIT_SIMD_REGISTER_SIZE (2 * sizeof(double))
#else
// Assuming 128-bit SIMD registers by default.
#define TIT_SIMD_REGISTER_SIZE (2 * sizeof(double))
#endif

namespace simd {

  static_assert(is_power_of_two(TIT_SIMD_REGISTER_SIZE));

  /** Maximum SIMD register size. */
  template<class Num>
  inline constexpr auto max_reg_size_v =
      std::max<size_t>(1, TIT_SIMD_REGISTER_SIZE / sizeof(Num));

  /** @brief Should SIMD registors be used for the amount scalars?
   ** Registers are used if either number of dimensions is greater than register
   ** size for the scalar type (e.g., 3*double on NEON CPU), or it is less than
   ** register size for the scalar type (e.g. 3*double on AVX CPU) and number of
   ** dimensions is not power of two (in this case fractions of registers are
   ** used, e.g. 2*double on AVX CPU). */
  template<class Num, size_t Dim>
  concept use_regs = (Dim > max_reg_size_v<Num>) ||
                     (Dim < max_reg_size_v<Num> && !is_power_of_two(Dim));

  /** SIMD register size for the specified amount of scalars. */
  template<class Num, size_t Dim>
    requires use_regs<Num, Dim>
  inline constexpr auto reg_size_v =
      std::min(max_reg_size_v<Num>, //
               (is_power_of_two(Dim) ? Dim : exp2(log2(Dim) + 1)));

  /** Do SIMD registors match for the specified types? */
  template<size_t Dim, class Num, class... RestNums>
  concept regs_match =
      use_regs<Num, Dim> && (use_regs<RestNums, Dim> && ...) &&
      ((reg_size_v<Num, Dim> == reg_size_v<RestNums, Dim>) &&...);

} // namespace simd

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** Algebraic vector (blockified version).
\******************************************************************************/
template<class Num, size_t Dim>
  requires simd::use_regs<Num, Dim>
class Vec<Num, Dim> final {
public:

  /** Number of scalars. */
  static constexpr auto num_scalars = Dim;

  /** SIMD register size. */
  static constexpr auto reg_size = simd::reg_size_v<Num, Dim>;

  /** Number of SIMD register. */
  static constexpr auto num_regs = ceil_divide(num_scalars, reg_size);

  /** Padding. */
  static constexpr auto padding = reg_size * num_regs - num_scalars;

  /** SIMD register type. */
  using Reg = Vec<Num, simd::reg_size_v<Num, Dim>>;

private:

  std::array<Reg, num_regs> _regs;

  template<class... Args>
  static constexpr auto _pack_regs(Args... qi) noexcept {
    const auto qi_padded = std::array<Num, num_scalars + padding>{qi...};
    const auto pack_reg = [&]<size_t ri>(std::index_sequence<ri>) {
      return [&]<size_t... ss>(std::index_sequence<ss...>) {
        return Reg{qi_padded[ri * reg_size + ss]...};
      }(std::make_index_sequence<reg_size>{});
    };
    return [&]<size_t... rs>(std::index_sequence<rs...>) {
      return std::array{pack_reg(std::index_sequence<rs>{})...};
    }(std::make_index_sequence<num_regs>{});
  }

public:

  /** Construct a vector with components. */
  template<class... Args>
    requires (sizeof...(Args) == Dim)
  constexpr explicit Vec(Args... qi) noexcept : _regs{_pack_regs(qi...)} {}

  /** Fill-initialize the vector. */
  constexpr Vec(Num q = Num{}) noexcept {
    _regs.fill(Reg(q));
  }

  /** Fill-assign the vector. */
  constexpr auto operator=(Num q) noexcept -> Vec& {
    _regs.fill(Reg(q));
    return *this;
  }

  /** Vector register at index. */
  /** @{ */
  constexpr auto reg(size_t i) noexcept -> Reg& {
    TIT_ASSERT(i < num_regs, "Register index is out of range.");
    return _regs[i];
  }
  constexpr auto reg(size_t i) const noexcept -> Reg {
    TIT_ASSERT(i < num_regs, "Register index is out of range.");
    return _regs[i];
  }
  /** @} */

  /** Vector component at index. */
  /** @{ */
  constexpr auto operator[](size_t i) noexcept -> Num& {
    TIT_ASSERT(i < num_scalars, "Component index is out of range.");
    return _regs[i / reg_size][i % reg_size];
  }
  constexpr auto operator[](size_t i) const noexcept -> Num {
    TIT_ASSERT(i < num_scalars, "Component index is out of range.");
    return _regs[i / reg_size][i % reg_size];
  }
  /** @} */

}; // class Vec<Num, Dim>

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Vector addition. */
template<class NumA, class NumB, size_t Dim>
  requires simd::regs_match<Dim, NumA, NumB>
constexpr auto operator+(Vec<NumA, Dim> a, Vec<NumB, Dim> b) noexcept {
  Vec<add_result_t<NumA, NumB>, Dim> r;
  for (size_t i = 0; i < r.num_regs; ++i) r.reg(i) = a.reg(i) + b.reg(i);
  return r;
}

/** Vector addition assignment. */
template<class NumA, class NumB, size_t Dim>
  requires simd::regs_match<Dim, NumA, NumB>
constexpr auto& operator+=(Vec<NumA, Dim>& a, Vec<NumB, Dim> b) noexcept {
  for (size_t i = 0; i < a.num_regs; ++i) a.reg(i) += b.reg(i);
  return a;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Vector negation. */
template<class Num, size_t Dim>
  requires simd::use_regs<Num, Dim>
constexpr auto operator-(Vec<Num, Dim> a) noexcept {
  Vec<negate_result_t<Num>, Dim> r;
  for (size_t i = 0; i < r.num_regs; ++i) r.reg(i) = -a.reg(i);
  return r;
}

/** Vector subtraction. */
template<class NumA, class NumB, size_t Dim>
  requires simd::regs_match<Dim, NumA, NumB>
constexpr auto operator-(Vec<NumA, Dim> a, Vec<NumB, Dim> b) noexcept {
  Vec<sub_result_t<NumA, NumB>, Dim> r;
  for (size_t i = 0; i < r.num_regs; ++i) r.reg(i) = a.reg(i) - b.reg(i);
  return r;
}

/** Vector subtraction assignment. */
template<class NumA, class NumB, size_t Dim>
  requires simd::regs_match<Dim, NumA, NumB>
constexpr auto& operator-=(Vec<NumA, Dim>& a, Vec<NumB, Dim> b) noexcept {
  for (size_t i = 0; i < a.num_regs; ++i) a.reg(i) -= b.reg(i);
  return a;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Vector multiplication. */
/** @{ */
template<class NumA, class NumB, size_t Dim>
  requires simd::regs_match<Dim, NumA, NumB>
constexpr auto operator*(NumA a, Vec<NumB, Dim> b) noexcept {
  Vec<mul_result_t<NumA, NumB>, Dim> r;
  const auto a_reg = typename Vec<NumA, Dim>::Reg(a);
  for (size_t i = 0; i < r.num_regs; ++i) r.reg(i) = a_reg * b.reg(i);
  return r;
}
template<class NumA, class NumB, size_t Dim>
  requires simd::regs_match<Dim, NumA, NumB>
constexpr auto operator*(Vec<NumA, Dim> a, NumB b) noexcept {
  Vec<mul_result_t<NumA, NumB>, Dim> r;
  const auto b_reg = typename Vec<NumB, Dim>::Reg(b);
  for (size_t i = 0; i < r.num_regs; ++i) r.reg(i) = a.reg(i) * b_reg;
  return r;
}
template<class NumA, class NumB, size_t Dim>
  requires simd::regs_match<Dim, NumA, NumB>
constexpr auto operator*(Vec<NumA, Dim> a, Vec<NumB, Dim> b) noexcept {
  Vec<mul_result_t<NumA, NumB>, Dim> r;
  for (size_t i = 0; i < r.num_regs; ++i) r.reg(i) = a.reg(i) * b.reg(i);
  return r;
}
/** @} */

/** Vector multiplication assignment. */
/** @{ */
template<class NumA, class NumB, size_t Dim>
  requires simd::regs_match<Dim, NumA, NumB>
constexpr auto& operator*=(Vec<NumA, Dim>& a, NumB b) noexcept {
  const auto b_reg = typename Vec<NumB, Dim>::Reg(b);
  for (size_t i = 0; i < a.num_regs; ++i) a.reg(i) *= b_reg;
  return a;
}
template<class NumA, class NumB, size_t Dim>
  requires simd::regs_match<Dim, NumA, NumB>
constexpr auto& operator*=(Vec<NumA, Dim>& a, Vec<NumB, Dim> b) noexcept {
  for (size_t i = 0; i < a.num_regs; ++i) a.reg(i) *= b.reg(i);
  return a;
}
/** @} */

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Vector division. */
/** @{ */
template<class NumA, class NumB, size_t Dim>
  requires simd::regs_match<Dim, NumA, NumB>
constexpr auto operator/(Vec<NumA, Dim> a, NumB b) noexcept {
  Vec<div_result_t<NumA, NumB>, Dim> r;
  const auto b_reg = typename Vec<NumB, Dim>::Reg(b);
  for (size_t i = 0; i < r.num_regs; ++i) r.reg(i) = a.reg(i) / b_reg;
  return r;
}
template<class NumA, class NumB, size_t Dim>
  requires simd::regs_match<Dim, NumA, NumB>
constexpr auto operator/(Vec<NumA, Dim> a, Vec<NumB, Dim> b) noexcept {
  Vec<div_result_t<NumA, NumB>, Dim> r;
  for (size_t i = 0; i < r.num_regs; ++i) r.reg(i) = a.reg(i) / b.reg(i);
  return r;
}
/** @} */

/** Vector division assignment. */
/** @{ */
template<class NumA, class NumB, size_t Dim>
  requires simd::regs_match<Dim, NumA, NumB>
constexpr auto& operator/=(Vec<NumA, Dim>& a, NumB b) noexcept {
  const auto b_reg = typename Vec<NumB, Dim>::Reg(b);
  for (size_t i = 0; i < a.num_regs; ++i) a.reg(i) /= b_reg;
  return a;
}
template<class NumA, class NumB, size_t Dim>
  requires simd::regs_match<Dim, NumA, NumB>
constexpr auto& operator/=(Vec<NumA, Dim>& a, Vec<NumB, Dim> b) noexcept {
  for (size_t i = 0; i < a.num_regs; ++i) a.reg(i) /= b.reg(i);
  return a;
}
/** @} */

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Sum of the vector components. */
// TODO: implement sum with non-zero padding.
template<class Num, size_t Dim>
  requires simd::use_regs<Num, Dim> && (Vec<Num, Dim>::padding == 0)
constexpr auto sum(Vec<Num, Dim> a) noexcept {
  auto r_reg(a.reg(0));
  for (size_t i = 1; i < a.num_regs; ++i) r_reg += a.reg(i);
  return sum(r_reg);
}

// TODO: implement dot product for SIMD.

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Merge vector with zero vector based on comparison result. */
template<class Op, class NumX, class NumY, class NumA, size_t Dim>
  requires simd::regs_match<Dim, NumX, NumY, NumA>
constexpr auto merge(VecCmp<Op, Dim, NumX, NumY> cmp,
                     Vec<NumA, Dim> a) noexcept {
  Vec<NumA, Dim> r;
  const auto& [op, x, y] = cmp;
  for (size_t i = 0; i < r.num_regs; ++i) {
    r.reg(i) = merge(VecCmp{op, x.reg(i), y.reg(i)}, a.reg(i));
  }
  return r;
}
/** Merge two vectors bases on comparison result. */
template<class Op, class NumX, class NumY, class NumA, class NumB, size_t Dim>
  requires simd::regs_match<Dim, NumX, NumY, NumA, NumB>
constexpr auto merge(VecCmp<Op, Dim, NumX, NumY> cmp, //
                     Vec<NumA, Dim> a, Vec<NumB, Dim> b) noexcept {
  Vec<sub_result_t<NumA, NumB>, Dim> r;
  const auto& [op, x, y] = cmp;
  for (size_t i = 0; i < r.num_regs; ++i) {
    r.reg(i) = merge(VecCmp{op, x.reg(i), y.reg(i)}, a.reg(i), b.reg(i));
  }
  return r;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit

#if TIT_ENABLE_INTRISICS

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

namespace tit {
consteval auto _unwrap(auto value) noexcept {
  return value;
}
consteval auto& _unwrap(auto* value) noexcept {
  return *value;
}
} // namespace tit

// Generate a constexpr-aware overload for a unary vector function.
#define TIT_VEC_SIMD_FUNC_V(func, Dim, Num, a, ...)                            \
  inline constexpr auto func(Vec<Num, Dim> a) noexcept {                       \
    if consteval {                                                             \
      return func<Num, Dim>(_unwrap(a));                                       \
    } else __VA_ARGS__                                                         \
  }

// Generate a constexpr-aware overload for a binary scalar-vector function.
#define TIT_VEC_SIMD_FUNC_SV(func, Dim, NumA, a, NumB, b, ...)                 \
  template<std::convertible_to<NumB> NumA>                                     \
  constexpr auto func(NumA a, Vec<NumB, Dim> b) noexcept {                     \
    if consteval {                                                             \
      return func<NumA, NumB, Dim>(_unwrap(a), _unwrap(b));                    \
    } else __VA_ARGS__                                                         \
  }

// Generate a constexpr-aware overload for a binary vector-scalar function.
#define TIT_VEC_SIMD_FUNC_VS(func, Dim, NumA, a, NumB, b, ...)                 \
  template<std::convertible_to<NumA> NumB>                                     \
  constexpr auto func(Vec<NumA, Dim> a, NumB b) noexcept {                     \
    if consteval {                                                             \
      return func<NumA, NumB, Dim>(_unwrap(a), _unwrap(b));                    \
    } else __VA_ARGS__                                                         \
  }

// Generate a constexpr-aware overload for a binary vector-vector function.
#define TIT_VEC_SIMD_FUNC_VV(func, Dim, NumA, a, NumB, b, ...)                 \
  constexpr auto func(Vec<NumA, Dim> a, Vec<NumB, Dim> b) noexcept {           \
    if consteval {                                                             \
      return func<NumA, NumB, Dim>(_unwrap(a), _unwrap(b));                    \
    } else __VA_ARGS__                                                         \
  }

// Generate a constexpr-aware overload for a vector merge function.
#define TIT_VEC_SIMD_MERGE(Dim, Op, NumX, NumY, cmp, NumA, a, ...)             \
  template<common_cmp_op Op>                                                   \
  constexpr auto merge(VecCmp<Op, Dim, NumX, NumY> cmp,                        \
                       Vec<NumA, Dim> a) noexcept {                            \
    if consteval {                                                             \
      return merge<Op, NumX, NumY, NumA, Dim>(_unwrap(cmp), _unwrap(a));       \
    } else __VA_ARGS__                                                         \
  }

// Generate a constexpr-aware overload for a two vector merge function.
#define TIT_VEC_SIMD_MERGE_2(Dim, Op, NumX, NumY, cmp, NumA, a, NumB, b, ...)  \
  template<common_cmp_op Op>                                                   \
  constexpr auto merge(VecCmp<Op, Dim, NumX, NumY> cmp, Vec<NumA, Dim> a,      \
                       Vec<NumB, Dim> b) noexcept {                            \
    if consteval {                                                             \
      return merge<Op, NumX, NumY, NumA, NumB, Dim>(_unwrap(cmp), _unwrap(a),  \
                                                    _unwrap(b));               \
    } else __VA_ARGS__                                                         \
  }

// IWYU pragma: begin_exports
#if defined(__SSE__)
#include "tit/core/vec_avx.hpp"
#elif defined(__ARM_NEON)
#include "tit/core/vec_neon.hpp"
#endif
// IWYU pragma: end_exports

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#endif // TIT_ENABLE_INTRISICS
