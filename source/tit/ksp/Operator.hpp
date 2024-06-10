/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <cmath>
#include <functional>
#include <memory>
#include <stdexcept>

#include "tit/ksp/Vector.hpp"
#include "tit/ksp/stormBase.hpp"

namespace tit::ksp {

struct Object {
  virtual ~Object() = default;
};

/// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- ///
/// @brief Abstract operator 𝒚 ← 𝓐(𝒙).
/// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- ///
template<VectorLike InVector, VectorLike OutVector = InVector>
class Operator : public Object {
public:

  /// @brief Compute an operator-vector product, 𝒚 ← 𝓐(𝒙).
  ///
  /// @param yVec Output vector, 𝒚.
  /// @param xVec Input vector, 𝒙.
  virtual void MatVec(OutVector& yVec, const InVector& xVec) const = 0;

  /// @brief Compute a chained
  ///   operator-vector product, 𝒛 ← 𝓐(𝒚 ← 𝓑(𝒙)).
  ///
  /// @param zVec Output vector, 𝒛.
  /// @param yVec Intermediate vector, 𝒚.
  /// @param xVec Input vector, 𝒙.
  template<VectorLike InOutVector = InVector>
  void MatVec(OutVector& zVec,
              InOutVector& yVec,
              const Operator<InVector, InOutVector>& otherOp,
              const InVector& xVec) const {
    otherOp.MatVec(yVec, xVec);
    MatVec(zVec, yVec);
  }

  /// @brief Compute a residual, 𝒓 ← 𝒃 - 𝓐(𝒙).
  ///
  /// @param rVec Residual vector, 𝒓.
  /// @param bVec Input vector, 𝒃.
  /// @param xVec Input vector, 𝒙.
  void Residual(OutVector& rVec,
                const OutVector& bVec,
                const InVector& xVec) const {
    MatVec(rVec, xVec);
    Blas::Sub(rVec, bVec, rVec);
  }

  /// @brief Compute a residual norm, ‖𝒃 - 𝓐𝒙‖.
  ///
  /// @param bVec Input vector, 𝒃.
  /// @param xVec Input vector, 𝒙.
  real_t ResidualNorm(const OutVector& bVec, const InVector& xVec) const {
    OutVector rVec;
    rVec.Assign(bVec, false);
    Residual(rVec, bVec, xVec);
    return rVec.Norm2();
  }

  /// @brief Compute an conjugate operator-vector product, 𝒙 ← 𝓐*(𝒚).
  ///
  /// @param xVec Output vector, 𝒙.
  /// @param yVec Input vector, 𝒚.
  virtual void ConjMatVec(InVector& xVec, const OutVector& yVec) const {
    throw std::runtime_error("`Operator::ConjMatVec` was not overridden");
  }

}; // class Operator

/// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- ///
/// @brief Operator implementation with external function pointers.
/// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- ///
template<VectorLike InVector, VectorLike OutVector = InVector>
class FunctionalOperator final : public Operator<InVector, OutVector> {
private:

  std::function<void(OutVector&, const InVector&)> MatVecFunc_;
  std::function<void(InVector&, const OutVector&)> ConjMatVecFunc_;

public:

  /// @brief Construct the functional operator.
  ///
  /// @param matVecFunc Operator-vector product function, 𝒚 ← 𝓐(𝒙).
  /// @param conjMatVecFunc Conjugate operator-vector product, 𝒙 ← 𝓐*(𝒚).
  /// @{
  template<operator_like<InVector, OutVector> MatVecFunc>
  explicit FunctionalOperator(MatVecFunc&& matVecFunc)
      : MatVecFunc_{std::forward<MatVecFunc>(matVecFunc)} {
    StormAssert(MatVecFunc_);
  }
  template<operator_like<InVector, OutVector> MatVecFunc,
           operator_like<OutVector, InVector> ConjMatVecFunc>
  explicit FunctionalOperator(MatVecFunc&& matVecFunc,
                              ConjMatVecFunc&& conjMatVecFunc)
      : MatVecFunc_{std::forward<MatVecFunc>(matVecFunc)},
        ConjMatVecFunc_{std::forward<ConjMatVecFunc>(conjMatVecFunc)} {
    StormAssert(MatVecFunc_ && ConjMatVecFunc_);
  }
  /// @}

private:

  void MatVec(OutVector& yVec, const InVector& xVec) const override {
    MatVecFunc_(yVec, xVec);
  }

  void ConjMatVec(InVector& xVec, const OutVector& yVec) const override {
    if (!ConjMatVecFunc_) {
      throw std::runtime_error("`FunctionalOperator::ConjMatVec`"
                               " conjugate product function was not set.");
    }
    ConjMatVecFunc_(xVec, yVec);
  }

}; // class FunctionalOperator

/// ----------------------------------------------------------------- ///
/// @brief Make the functional operator.
///
/// @param matVecFunc Operator-vector product function, 𝒚 ← 𝓐(𝒙).
/// @param conjMatVecFunc Conjugate operator-vector product, 𝒙 ← 𝓐*(𝒚).
/// ----------------------------------------------------------------- ///
/// @{
template<VectorLike InVector,
         VectorLike OutVector = InVector,
         operator_like<InVector, OutVector> MatVecFunc>
auto MakeOperator(MatVecFunc&& matVecFunc) {
  return std::make_unique<FunctionalOperator<InVector, OutVector>>(
      std::forward<MatVecFunc>(matVecFunc));
}
template<VectorLike InVector,
         VectorLike OutVector = InVector,
         operator_like<InVector, OutVector> MatVecFunc,
         operator_like<OutVector, InVector> ConjMatVecFunc>
auto MakeOperator(MatVecFunc&& matVecFunc, ConjMatVecFunc&& conjMatVecFunc) {
  return std::make_unique<FunctionalOperator<InVector, OutVector>>(
      std::forward<MatVecFunc>(matVecFunc),
      std::forward<ConjMatVecFunc>(conjMatVecFunc));
}
/// @}

/// ----------------------------------------------------------------- ///
/// @brief Make the self-adjoint functional operator.
/// ----------------------------------------------------------------- ///
template<VectorLike Vector, operator_like<Vector> MatVecFunc>
auto MakeSymmetricOperator(MatVecFunc&& matVecFunc) {
  return std::make_unique<FunctionalOperator<Vector>>(
      matVecFunc,
      std::forward<MatVecFunc>(matVecFunc));
}

} // namespace tit::ksp
