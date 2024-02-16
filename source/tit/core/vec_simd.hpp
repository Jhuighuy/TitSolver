/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <array>
#include <concepts> // IWYU pragma: keep
#include <utility>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/math.hpp"
#include "tit/core/simd.hpp"
#include "tit/core/utils.hpp" // IWYU pragma: keep
#include "tit/core/vec.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Algebraic vector (blocked version).
template<class Num, size_t Dim>
  requires simd::use_regs<Num, Dim>
class Vec<Num, Dim> final {
public:

  /// Number of rows.
  static constexpr auto num_rows = Dim;

  /// SIMD register size.
  static constexpr auto reg_size = simd::reg_size_v<Num, Dim>;

  /// Number of SIMD register.
  static constexpr auto num_regs = ceil_divide(num_rows, reg_size);

  /// Padding.
  static constexpr auto padding = reg_size * num_regs - num_rows;

  /// SIMD register type.
  using Reg = Vec<Num, simd::reg_size_v<Num, Dim>>;

private:

  std::array<Reg, num_regs> regs_;

public:

  /// Construct a vector with elements.
  template<class... Args>
    requires (sizeof...(Args) == Dim) &&
             (std::constructible_from<Num, Args> && ...)
  constexpr explicit Vec(Args... qi) noexcept
      : regs_{[&]<size_t... rs>(std::index_sequence<rs...>) {
          auto const qi_padded = std::array<Num, num_rows + padding>{qi...};
          auto const pack_reg = [&]<size_t ri>(std::index_sequence<ri>) {
            return [&]<size_t... ss>(std::index_sequence<ss...>) {
              return Reg{qi_padded[ri * reg_size + ss]...};
            }(std::make_index_sequence<reg_size>{});
          };
          return std::array{pack_reg(std::index_sequence<rs>{})...};
        }(std::make_index_sequence<num_regs>{})} {}

  /// Fill-initialize the vector.
  constexpr explicit Vec(Num q = Num{}) noexcept {
    regs_.fill(Reg(q));
  }
  /// Fill-assign the vector.
  constexpr auto operator=(Num q) noexcept -> Vec& {
    regs_.fill(Reg(q));
    return *this;
  }

  /// Vector register at index.
  /// @{
  constexpr auto reg(size_t i) noexcept -> Reg& {
    TIT_ASSERT(i < num_regs, "Register index is out of range.");
    return regs_[i];
  }
  constexpr auto reg(size_t i) const noexcept -> Reg {
    TIT_ASSERT(i < num_regs, "Register index is out of range.");
    return regs_[i];
  }
  /// @}

  /// Vector component at index.
  /// @{
  constexpr auto operator[](size_t i) noexcept -> Num& {
    TIT_ASSERT(i < num_rows, "Row index is out of range.");
    return regs_[i / reg_size][i % reg_size];
  }
  constexpr auto operator[](size_t i) const noexcept -> Num {
    TIT_ASSERT(i < num_rows, "Row index is out of range.");
    return regs_[i / reg_size][i % reg_size];
  }
  /// @}

}; // class Vec<Num, Dim>

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

template<class NumA, class NumB, size_t Dim>
  requires simd::regs_match<Dim, NumA, NumB>
constexpr auto operator+(Vec<NumA, Dim> a, Vec<NumB, Dim> b) noexcept {
  Vec<add_result_t<NumA, NumB>, Dim> r;
  for (size_t i = 0; i < r.num_regs; ++i) r.reg(i) = a.reg(i) + b.reg(i);
  return r;
}

template<class NumA, class NumB, size_t Dim>
  requires simd::regs_match<Dim, NumA, NumB>
constexpr auto operator+=(Vec<NumA, Dim>& a, Vec<NumB, Dim> b) noexcept
    -> auto& {
  for (size_t i = 0; i < a.num_regs; ++i) a.reg(i) += b.reg(i);
  return a;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

template<class Num, size_t Dim>
  requires simd::use_regs<Num, Dim>
constexpr auto operator-(Vec<Num, Dim> a) noexcept {
  Vec<negate_result_t<Num>, Dim> r;
  for (size_t i = 0; i < r.num_regs; ++i) r.reg(i) = -a.reg(i);
  return r;
}

template<class NumA, class NumB, size_t Dim>
  requires simd::regs_match<Dim, NumA, NumB>
constexpr auto operator-(Vec<NumA, Dim> a, Vec<NumB, Dim> b) noexcept {
  Vec<sub_result_t<NumA, NumB>, Dim> r;
  for (size_t i = 0; i < r.num_regs; ++i) r.reg(i) = a.reg(i) - b.reg(i);
  return r;
}

template<class NumA, class NumB, size_t Dim>
  requires simd::regs_match<Dim, NumA, NumB>
constexpr auto operator-=(Vec<NumA, Dim>& a, Vec<NumB, Dim> b) noexcept
    -> auto& {
  for (size_t i = 0; i < a.num_regs; ++i) a.reg(i) -= b.reg(i);
  return a;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

template<class NumA, class NumB, size_t Dim>
  requires simd::regs_match<Dim, NumA, NumB>
constexpr auto operator*(NumA a, Vec<NumB, Dim> b) noexcept {
  Vec<mul_result_t<NumA, NumB>, Dim> r;
  auto const a_reg = typename Vec<NumA, Dim>::Reg(a);
  for (size_t i = 0; i < r.num_regs; ++i) r.reg(i) = a_reg * b.reg(i);
  return r;
}
template<class NumA, class NumB, size_t Dim>
  requires simd::regs_match<Dim, NumA, NumB>
constexpr auto operator*(Vec<NumA, Dim> a, NumB b) noexcept {
  Vec<mul_result_t<NumA, NumB>, Dim> r;
  auto const b_reg = typename Vec<NumB, Dim>::Reg(b);
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

template<class NumA, class NumB, size_t Dim>
  requires simd::regs_match<Dim, NumA, NumB>
constexpr auto operator*=(Vec<NumA, Dim>& a, NumB b) noexcept -> auto& {
  auto const b_reg = typename Vec<NumB, Dim>::Reg(b);
  for (size_t i = 0; i < a.num_regs; ++i) a.reg(i) *= b_reg;
  return a;
}
template<class NumA, class NumB, size_t Dim>
  requires simd::regs_match<Dim, NumA, NumB>
constexpr auto operator*=(Vec<NumA, Dim>& a, Vec<NumB, Dim> b) noexcept
    -> auto& {
  for (size_t i = 0; i < a.num_regs; ++i) a.reg(i) *= b.reg(i);
  return a;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

template<class NumA, class NumB, size_t Dim>
  requires simd::regs_match<Dim, NumA, NumB>
constexpr auto operator/(Vec<NumA, Dim> a, NumB b) noexcept {
  Vec<div_result_t<NumA, NumB>, Dim> r;
  auto const b_reg = typename Vec<NumB, Dim>::Reg(b);
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

template<class NumA, class NumB, size_t Dim>
  requires simd::regs_match<Dim, NumA, NumB>
constexpr auto operator/=(Vec<NumA, Dim>& a, NumB b) noexcept -> auto& {
  auto const b_reg = typename Vec<NumB, Dim>::Reg(b);
  for (size_t i = 0; i < a.num_regs; ++i) a.reg(i) /= b_reg;
  return a;
}
template<class NumA, class NumB, size_t Dim>
  requires simd::regs_match<Dim, NumA, NumB>
constexpr auto operator/=(Vec<NumA, Dim>& a, Vec<NumB, Dim> b) noexcept
    -> auto& {
  for (size_t i = 0; i < a.num_regs; ++i) a.reg(i) /= b.reg(i);
  return a;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

template<class To, class From, size_t Dim>
  requires simd::regs_match<Dim, From, To>
constexpr auto static_vec_cast(Vec<From, Dim> a) noexcept {
  Vec<To, Dim> r;
  for (size_t i = 0; i < r.num_regs; ++i) {
    r.reg(i) = static_vec_cast<To>(a.reg(i));
  }
  return r;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// TODO: implement with SIMD intrinsics (where possible).
template<class Num, size_t Dim>
  requires simd::use_regs<Num, Dim>
constexpr auto floor(Vec<Num, Dim> a) noexcept {
  Vec<Num, Dim> r;
  for (size_t i = 0; i < r.num_regs; ++i) r.reg(i) = floor(a.reg(i));
  return r;
}

// TODO: implement with SIMD intrinsics (where possible).
template<class Num, size_t Dim>
  requires simd::use_regs<Num, Dim>
constexpr auto round(Vec<Num, Dim> a) noexcept {
  Vec<Num, Dim> r;
  for (size_t i = 0; i < r.num_regs; ++i) r.reg(i) = round(a.reg(i));
  return r;
}

// TODO: implement with SIMD intrinsics (where possible).
template<class Num, size_t Dim>
  requires simd::use_regs<Num, Dim>
constexpr auto ceil(Vec<Num, Dim> a) noexcept {
  Vec<Num, Dim> r;
  for (size_t i = 0; i < r.num_regs; ++i) r.reg(i) = ceil(a.reg(i));
  return r;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// TODO: implement sum with non-zero padding.
template<class Num, size_t Dim>
  requires simd::use_regs<Num, Dim> && (Vec<Num, Dim>::padding == 0)
constexpr auto sum(Vec<Num, Dim> a) noexcept {
  auto r_reg(a.reg(0));
  for (size_t i = 1; i < a.num_regs; ++i) r_reg += a.reg(i);
  return sum(r_reg);
}

// TODO: implement dot product for SIMD.

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

template<class Op, class NumX, class NumY, class NumA, size_t Dim>
  requires simd::regs_match<Dim, NumX, NumY, NumA>
constexpr auto merge(VecCmp<Op, Dim, NumX, NumY> cmp,
                     Vec<NumA, Dim> a) noexcept {
  Vec<NumA, Dim> r;
  auto const& [op, x, y] = cmp;
  for (size_t i = 0; i < r.num_regs; ++i) {
    r.reg(i) = merge(VecCmp{op, x.reg(i), y.reg(i)}, a.reg(i));
  }
  return r;
}
template<class Op, class NumX, class NumY, class NumA, class NumB, size_t Dim>
  requires simd::regs_match<Dim, NumX, NumY, NumA, NumB>
constexpr auto merge(VecCmp<Op, Dim, NumX, NumY> cmp, //
                     Vec<NumA, Dim> a, Vec<NumB, Dim> b) noexcept {
  Vec<sub_result_t<NumA, NumB>, Dim> r;
  auto const& [op, x, y] = cmp;
  for (size_t i = 0; i < r.num_regs; ++i) {
    r.reg(i) = merge(VecCmp{op, x.reg(i), y.reg(i)}, a.reg(i), b.reg(i));
  }
  return r;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Generate a constexpr-aware overload for a unary vector function.
#define TIT_VEC_SIMD_FUNC_V(func, Dim, Num, a, ...)                            \
  /* NOLINTNEXTLINE(*-use-trailing-return-type) */                             \
  constexpr auto func(Vec<Num, Dim> a) noexcept {                              \
    if consteval {                                                             \
      return _unwrap(func<Num, Dim>(_unwrap(a)));                              \
    } else { /* NOLINT(*-else-after-return) */                                 \
      __VA_ARGS__                                                              \
    }                                                                          \
  }

// Generate a constexpr-aware overload for a binary scalar-vector function.
// clang-format off
#define TIT_VEC_SIMD_FUNC_SV(func, Dim, NumA, a, NumB, b, ...)                 \
  template<std::convertible_to<NumB> NumA>                                     \
  /* NOLINTNEXTLINE(*-use-trailing-return-type) */                             \
  constexpr auto func(NumA a, Vec<NumB, Dim> b) noexcept {                     \
    if consteval {                                                             \
      return _unwrap(func<NumA, NumB, Dim>(_unwrap(a), _unwrap(b)));           \
    } else { /* NOLINT(*-else-after-return) */                                 \
      __VA_ARGS__                                                              \
    }                                                                          \
  }
// clang-format on

// Generate a constexpr-aware overload for a binary vector-scalar function.
// clang-format off
#define TIT_VEC_SIMD_FUNC_VS(func, Dim, NumA, a, NumB, b, ...)                 \
  template<std::convertible_to<NumA> NumB>                                     \
  /* NOLINTNEXTLINE(*-use-trailing-return-type) */                             \
  constexpr auto func(Vec<NumA, Dim> a, NumB b) noexcept {                     \
    if consteval {                                                             \
      return _unwrap(func<NumA, NumB, Dim>(_unwrap(a), _unwrap(b)));           \
    } else { /* NOLINT(*-else-after-return) */                                 \
      __VA_ARGS__                                                              \
    }                                                                          \
  }
// clang-format on

// Generate a constexpr-aware overload for a binary vector-vector function.
#define TIT_VEC_SIMD_FUNC_VV(func, Dim, NumA, a, NumB, b, ...)                 \
  /* NOLINTNEXTLINE(*-use-trailing-return-type) */                             \
  constexpr auto func(Vec<NumA, Dim> a, Vec<NumB, Dim> b) noexcept {           \
    if consteval {                                                             \
      return _unwrap(func<NumA, NumB, Dim>(_unwrap(a), _unwrap(b)));           \
    } else { /* NOLINT(*-else-after-return) */                                 \
      __VA_ARGS__                                                              \
    }                                                                          \
  }

// Generate a constexpr-aware overload for a vector merge function.
// clang-format off
#define TIT_VEC_SIMD_MERGE(Dim, Op, NumX, NumY, cmp, NumA, a, ...)             \
  template<common_cmp_op Op>                                                   \
  constexpr auto merge(VecCmp<Op, Dim, NumX, NumY> cmp,                        \
                       Vec<NumA, Dim> a) noexcept {                            \
    if consteval {                                                             \
      return _unwrap(                                                          \
          merge<Op, NumX, NumY, NumA, Dim>(_unwrap(cmp), _unwrap(a)));         \
    } else { /* NOLINT(*-else-after-return) */                                 \
      __VA_ARGS__                                                              \
    }                                                                          \
  }
// clang-format on

// Generate a constexpr-aware overload for a two vector merge function.
// clang-format off
#define TIT_VEC_SIMD_MERGE_2(Dim, Op, NumX, NumY, cmp, NumA, a, NumB, b, ...)  \
  template<common_cmp_op Op>                                                   \
  constexpr auto merge(VecCmp<Op, Dim, NumX, NumY> cmp,                        \
                       Vec<NumA, Dim> a, Vec<NumB, Dim> b) noexcept {          \
    if consteval {                                                             \
      return _unwrap(                                                          \
          merge<Op, NumX, NumY, NumA, NumB, Dim>(                              \
              _unwrap(cmp), _unwrap(a), _unwrap(b)));                          \
    } else { /* NOLINT(*-else-after-return) */                                 \
      __VA_ARGS__                                                              \
    }                                                                          \
  }
// clang-format on

#ifndef __INTELLISENSE__ // IntelliSense goes crazy when it encounters
                         // intrinsics. Especially NEON.
// IWYU pragma: begin_exports
#if defined(__SSE__)
#include "tit/core/vec_avx.hpp"
#elif defined(__ARM_NEON)
#include "tit/core/vec_neon.hpp"
#endif
// IWYU pragma: end_exports
#endif

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
