/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <functional>
#include <memory>
#include <stdexcept>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"

#include "tit/ksp/blas.hpp"

namespace tit::ksp {

struct Object {
  constexpr Object() = default;
  constexpr Object(const Object&) = default;
  constexpr Object(Object&&) = default;
  constexpr auto operator=(const Object&) -> Object& = default;
  constexpr auto operator=(Object&&) -> Object& = default;
  virtual ~Object() = default;
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Abstract operator 𝒚 ← 𝓐(𝒙).
template<blas::vector InVector, blas::vector OutVector = InVector>
class Operator : public Object {
public:

  /// Compute an operator-vector product, 𝒚 ← 𝓐(𝒙).
  ///
  /// @param y Output vector, 𝒚.
  /// @param x Input vector, 𝒙.
  constexpr virtual void MatVec(OutVector& y, const InVector& x) const = 0;

  /// Compute a chained operator-vector product, 𝒛 ← 𝓐(𝒚 ← 𝓑(𝒙)).
  ///
  /// @param z Output vector, 𝒛.
  /// @param y Intermediate vector, 𝒚.
  /// @param x Input vector, 𝒙.
  template<blas::vector InOutVector = InVector>
  constexpr void MatVec(OutVector& z,
                        InOutVector& y,
                        const Operator<InVector, InOutVector>& otherOp,
                        const InVector& x) const {
    otherOp.MatVec(y, x);
    MatVec(z, y);
  }

  /// Compute a residual, 𝒓 ← 𝒃 - 𝓐(𝒙).
  ///
  /// @param r Residual vector, 𝒓.
  /// @param b Input vector, 𝒃.
  /// @param x Input vector, 𝒙.
  constexpr void Residual(OutVector& r,
                          const OutVector& b,
                          const InVector& x) const {
    MatVec(r, x);
    Blas::Sub(r, b, r);
  }

  /// Compute a residual norm, ‖𝒃 - 𝓐𝒙‖.
  ///
  /// @param b Input vector, 𝒃.
  /// @param x Input vector, 𝒙.
  constexpr auto ResidualNorm(const OutVector& b,
                              const InVector& x) const -> real_t {
    OutVector r;
    r.Assign(b, false);
    Residual(r, b, x);
    return r.Norm2();
  }

  /// Compute an conjugate operator-vector product, 𝒙 ← 𝓐*(𝒚).
  ///
  /// @param x Output vector, 𝒙.
  /// @param y Input vector, 𝒚.
  constexpr virtual void ConjMatVec(InVector& /*x*/,
                                    const OutVector& /*y*/) const {
    throw std::runtime_error("`Operator::ConjMatVec` was not overridden");
  }

}; // class Operator

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Operator implementation with external function pointers.
template<blas::vector InVector, blas::vector OutVector = InVector>
class FunctionalOperator final : public Operator<InVector, OutVector> {
private:

  std::function<void(OutVector&, const InVector&)> MatVecFunc_;
  std::function<void(InVector&, const OutVector&)> ConjMatVecFunc_;

public:

  /// Construct the functional operator.
  ///
  /// @param matVecFunc Operator-vector product function, 𝒚 ← 𝓐(𝒙).
  /// @param conjMatVecFunc Conjugate operator-vector product, 𝒙 ← 𝓐*(𝒚).
  /// @{
  template<blas::op<InVector, OutVector> MatVecFunc>
  constexpr explicit FunctionalOperator(MatVecFunc&& matVecFunc)
      : MatVecFunc_{std::forward<MatVecFunc>(matVecFunc)} {
    TIT_ASSERT(!!MatVecFunc_, "Invalid function!");
  }
  template<blas::op<InVector, OutVector> MatVecFunc,
           blas::op<OutVector, InVector> ConjMatVecFunc>
  constexpr explicit FunctionalOperator(MatVecFunc&& matVecFunc,
                                        ConjMatVecFunc&& conjMatVecFunc)
      : MatVecFunc_{std::forward<MatVecFunc>(matVecFunc)},
        ConjMatVecFunc_{std::forward<ConjMatVecFunc>(conjMatVecFunc)} {
    TIT_ASSERT(MatVecFunc_ && ConjMatVecFunc_, "Invalid function!");
  }
  /// @}

private:

  void MatVec(OutVector& y, const InVector& x) const override {
    MatVecFunc_(y, x);
  }

  void ConjMatVec(InVector& x, const OutVector& y) const override {
    if (!ConjMatVecFunc_) {
      throw std::runtime_error("`FunctionalOperator::ConjMatVec`"
                               " conjugate product function was not set.");
    }
    ConjMatVecFunc_(x, y);
  }

}; // class FunctionalOperator

/// Make the functional operator.
///
/// @param matVecFunc Operator-vector product function, 𝒚 ← 𝓐(𝒙).
/// @param conjMatVecFunc Conjugate operator-vector product, 𝒙 ← 𝓐*(𝒚).
/// @{
template<blas::vector InVector,
         blas::vector OutVector = InVector,
         blas::op<InVector, OutVector> MatVecFunc>
constexpr auto MakeOperator(MatVecFunc&& matVecFunc) {
  return std::make_unique<FunctionalOperator<InVector, OutVector>>(
      std::forward<MatVecFunc>(matVecFunc));
}
template<blas::vector InVector,
         blas::vector OutVector = InVector,
         blas::op<InVector, OutVector> MatVecFunc,
         blas::op<OutVector, InVector> ConjMatVecFunc>
constexpr auto MakeOperator(MatVecFunc&& matVecFunc,
                            ConjMatVecFunc&& conjMatVecFunc) {
  return std::make_unique<FunctionalOperator<InVector, OutVector>>(
      std::forward<MatVecFunc>(matVecFunc),
      std::forward<ConjMatVecFunc>(conjMatVecFunc));
}
/// @}

/// Make the self-adjoint functional operator.
template<blas::vector Vector, blas::op<Vector> MatVecFunc>
constexpr auto MakeSymmetricOperator(MatVecFunc&& matVecFunc) {
  return std::make_unique<FunctionalOperator<Vector>>(
      matVecFunc,
      std::forward<MatVecFunc>(matVecFunc));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::ksp
