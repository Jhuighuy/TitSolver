/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// IWYU pragma: private, include "tit/core/vec.hpp"
#pragma once

#include <array>
#include <concepts>
#include <utility>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/simd.hpp"
#include "tit/core/utils.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace impl {

// Register specification for generic vector masks.
template<class Num, size_t Dim>
struct VecMaskSIMD {
  using Mask = bool;
  using RegMask = bool;
  static constexpr auto RegSize = 1;
  static constexpr auto RegCount = Dim;
};

// Register specification for SIMD vector masks.
template<simd::supported_type Num, size_t Dim>
struct VecMaskSIMD<Num, Dim> {
  using Mask = simd::Mask<Num>;
  using RegMask = simd::deduce_reg_mask_t<Num, Dim>;
  static constexpr auto RegSize = simd::deduce_size_v<Num, Dim>;
  static constexpr auto RegCount = simd::deduce_count_v<Num, Dim>;
};

} // namespace impl

/// Column vector element-wise boolean mask with SIMD support.
template<class Num, size_t Dim>
class VecMask final {
public:

  /// Type of the underlying element mask that is used.
  using Mask = impl::VecMaskSIMD<Num, Dim>::Mask;

  /// Type of the underlying register that is used.
  using RegMask = impl::VecMaskSIMD<Num, Dim>::RegMask;

  /// Size of the underlying register that is used.
  static constexpr auto RegSize = impl::VecMaskSIMD<Num, Dim>::RegSize;

  /// Amount of the underlying registers stored.
  static constexpr auto RegCount = impl::VecMaskSIMD<Num, Dim>::RegCount;

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  // NOLINTBEGIN(*-type-union-access)

  /// Fill-initialize the vector mask with false values.
  constexpr VecMask() {
    TIT_IF_SIMD_AVALIABLE(Num) {
      regs_ = {}, regs_.fill(RegMask{}); // mark as active union member.
      return;
    }
    col_.fill(Mask{});
  }

  /// Fill-initialize the vector mask with the boolean @p b.
  constexpr explicit(Dim > 1) VecMask(bool b) {
    TIT_IF_SIMD_AVALIABLE(Num) {
      regs_ = {}, regs_.fill(RegMask(b)); // mark as active union member.
      return;
    }
    col_.fill(Mask(b));
  }

  /// Construct a vector mask with elements @p bs.
  template<class... Args>
    requires (Dim > 1) && (sizeof...(Args) == Dim) &&
             (std::constructible_from<bool, Args &&> && ...)
  constexpr VecMask(Args&&... bs) : col_{Mask{std::forward<Args>(bs)}...} {}

  /// Vector mask element at index.
  constexpr auto operator[](this auto&& self, size_t i) noexcept -> auto&& {
    TIT_ASSERT(i < Dim, "Row index is out of range!");
    return TIT_FORWARD_LIKE(self, self.col_[i]);
  }

  /// Underlying register at index.
  auto reg(this auto&& self, size_t i) noexcept -> auto&& {
    TIT_ASSERT(i < RegCount, "Register index is out of range!");
    return TIT_FORWARD_LIKE(self, self.regs_[i]);
  }

  // NOLINTEND(*-type-union-access)

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Check if all elements are true.
  constexpr explicit(false) operator bool() const noexcept {
    return all(*this);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Vector mask element-wise logical negation operation.
  friend constexpr auto operator!(const VecMask& m) -> VecMask {
    VecMask r;
    TIT_IF_SIMD_AVALIABLE(Num) {
      for (size_t i = 0; i < RegCount; ++i) r.reg(i) = !m.reg(i);
      return r;
    }
    for (size_t i = 0; i < Dim; ++i) r[i] = !m[i];
    return r;
  }

  /// Vector mask element-wise conjunction operation.
  friend constexpr auto operator&&(const VecMask& m, const VecMask& n)
      -> VecMask {
    VecMask r;
    TIT_IF_SIMD_AVALIABLE(Num) {
      for (size_t i = 0; i < RegCount; ++i) r.reg(i) = m.reg(i) && n.reg(i);
      return r;
    }
    for (size_t i = 0; i < Dim; ++i) r[i] = m[i] && n[i];
    return r;
  }

  /// Vector mask element-wise disjunction operation.
  friend constexpr auto operator||(const VecMask& m, const VecMask& n)
      -> VecMask {
    VecMask r;
    TIT_IF_SIMD_AVALIABLE(Num) {
      for (size_t i = 0; i < RegCount; ++i) r.reg(i) = m.reg(i) || n.reg(i);
      return r;
    }
    for (size_t i = 0; i < Dim; ++i) r[i] = m[i] || n[i];
    return r;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Vector mask element-wise "equal to" comparison operation.
  friend constexpr auto operator==(const VecMask& m, const VecMask& n)
      -> VecMask {
    VecMask r;
    TIT_IF_SIMD_AVALIABLE(Num) {
      for (size_t i = 0; i < RegCount; ++i) r.reg(i) = m.reg(i) == n.reg(i);
      return r;
    }
    for (size_t i = 0; i < Dim; ++i) r[i] = m[i] == n[i];
    return r;
  }

  /// Vector element-wise "not equal to" comparison operation.
  friend constexpr auto operator!=(const VecMask& m, const VecMask& n)
      -> VecMask {
    VecMask r;
    TIT_IF_SIMD_AVALIABLE(Num) {
      for (size_t i = 0; i < RegCount; ++i) r.reg(i) = m.reg(i) != n.reg(i);
      return r;
    }
    for (size_t i = 0; i < Dim; ++i) r[i] = m[i] != n[i];
    return r;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  union {
    std::array<Mask, Dim> col_{}; // mark as active union member.
    std::array<RegMask, RegCount> regs_;
  };

}; // class VecMask<SIMD>

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Check if any vector mask element is set to true.
template<class Num, size_t Dim>
constexpr auto any(const VecMask<Num, Dim>& m) -> bool {
  TIT_IF_SIMD_AVALIABLE(Num) {
    constexpr auto RegSize = VecMask<Num, Dim>::RegSize;
    if constexpr (constexpr auto NFullRegs = Dim / RegSize; NFullRegs == 0) {
      return simd::any(simd::take_n(Dim, m.reg(0)));
    } else {
      auto r_reg = m.reg(0);
      for (size_t i = 1; i < NFullRegs; ++i) {
        r_reg = r_reg || m.reg(i);
      }
      if constexpr (constexpr auto Rem = Dim % RegSize; Rem != 0) {
        r_reg = r_reg || simd::take_n(Rem, m.reg(NFullRegs));
      }
      return simd::any(r_reg);
    }
  }
  for (size_t i = 0; i < Dim; ++i) {
    if (m[i]) return true;
  }
  return false;
}

/// Check if all vector mask elements are set to true.
template<class Num, size_t Dim>
constexpr auto all(const VecMask<Num, Dim>& m) -> bool {
  TIT_IF_SIMD_AVALIABLE(Num) {
    using RegMask = typename VecMask<Num, Dim>::RegMask;
    constexpr auto RegSize = VecMask<Num, Dim>::RegSize;
    if constexpr (constexpr auto NFullRegs = Dim / RegSize; NFullRegs == 0) {
      return simd::all(simd::merge_n(Dim, m.reg(0), RegMask(true)));
    } else {
      auto r_reg = m.reg(0);
      for (size_t i = 1; i < NFullRegs; ++i) {
        r_reg = r_reg && m.reg(i);
      }
      if constexpr (constexpr auto Rem = Dim % RegSize; Rem != 0) {
        r_reg = r_reg && simd::merge_n(Rem, m.reg(NFullRegs), RegMask(true));
      }
      return simd::all(r_reg);
    }
  }
  for (size_t i = 0; i < Dim; ++i) {
    if (!m[i]) return false;
  }
  return true;
}

/// Find the first true value in the vector mask.
/// Return `-1` if all values are false.
template<class Num, size_t Dim>
constexpr auto find_true(const VecMask<Num, Dim>& m) -> ssize_t {
  TIT_IF_SIMD_AVALIABLE(Num) {
    constexpr auto RegSize = VecMask<Num, Dim>::RegSize;
    if constexpr (constexpr auto NFullRegs = Dim / RegSize; NFullRegs == 0) {
      return simd::find_true(simd::take_n(Dim, m.reg(0)));
    } else {
      for (size_t i = 0; i < NFullRegs; ++i) {
        const auto true_index = simd::find_true(m.reg(i));
        if (true_index != -1) return true_index + i * RegSize;
      }
      if constexpr (constexpr auto Rem = Dim % RegSize; Rem != 0) {
        const auto true_index =
            simd::find_true(simd::take_n(Rem, m.reg(NFullRegs)));
        if (true_index != -1) return true_index + NFullRegs * RegSize;
      }
      return -1;
    }
  }
  for (size_t i = 0; i < Dim; ++i) {
    if (m[i]) return static_cast<ssize_t>(i);
  }
  return -1;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
