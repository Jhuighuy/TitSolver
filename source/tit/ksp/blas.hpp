/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <utility>

#include "tit/core/basic_types.hpp"

namespace tit::ksp {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Set of the operations for given vector type.
template<class Vector>
class VectorOperations {
public:

  /// Compute a dot product of @p x and @p y.
  constexpr static auto Dot(const Vector& x, const Vector& y) = delete;

  /// Compute a norm of @p x.
  constexpr static auto Norm2(const Vector& x) = delete;

  /// Compute @p x = @p y.
  constexpr static void Set(Vector& x, const Vector& y) = delete;

  /// Fill the @p x with value @p a.
  constexpr static void Fill(Vector& x, auto a) = delete;

  /// Randomly fill the @p x.
  constexpr static void RandFill(Vector& x) = delete;

  /// Compute @p x *= @p a.
  constexpr static void ScaleAssign(Vector& x, auto a);

  /// Compute @p x += @p a * @p y.
  constexpr static void AddAssign(Vector& x, const Vector& y, auto a);

}; // class VectorOperations

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// A type of a dot product of the two vectors.
template<class Vector>
using DotType = decltype(VectorOperations<Vector>::Dot(std::declval<Vector>(),
                                                       std::declval<Vector>()));

namespace blas {
/// Vector concept that supports the level 1 BLAS operations.
template<class Vector>
concept vector =
    std::swappable<Vector> &&
    requires(Vector& target, const Vector& source, bool copy_contents) {
      { target.Assign(source) };
      { target.Assign(source, copy_contents) };
    } && requires { typename DotType<Vector>; } && requires(const Vector& x) {
      { VectorOperations<Vector>::Norm2(x) } -> std::convertible_to<real_t>;
    } && requires(Vector& x, const Vector& y) {
      VectorOperations<Vector>::Set(x, y);
    } && requires(Vector& x, DotType<Vector> a) {
      VectorOperations<Vector>::Fill(x, a);
      VectorOperations<Vector>::RandFill(x);
    } && requires(Vector& x, const Vector& y, DotType<Vector> a) {
      VectorOperations<Vector>::ScaleAssign(x, a);
      VectorOperations<Vector>::AddAssign(x, y, a);
    };
} // namespace blas

namespace Blas {

/// Compute a dot product of @p x and @p y.
template<blas::vector Vector>
constexpr auto Dot(const Vector& x, const Vector& y) {
  return VectorOperations<Vector>::Dot(x, y);
}

/// Compute a norm of @p x.
template<blas::vector Vector>
constexpr auto Norm2(const Vector& x) -> real_t {
  return VectorOperations<Vector>::Norm2(x);
}

/// Compute @p x = @p y.
template<blas::vector Vector>
constexpr void Set(Vector& x, const Vector& y) {
  VectorOperations<Vector>::Set(x, y);
}

/// Fill the @p x with value @p a.
template<blas::vector Vector>
constexpr void Fill(Vector& x, auto a) {
  VectorOperations<Vector>::Fill(x, a);
}

/// Randomly fill the @p x.
template<blas::vector Vector>
constexpr void RandFill(Vector& x) {
  VectorOperations<Vector>::RandFill(x);
}

/// Compute @p x *= @p a.
template<blas::vector Vector>
constexpr void ScaleAssign(Vector& x, auto a) {
  VectorOperations<Vector>::ScaleAssign(x, a);
}

/// Compute @p x = @p a * @p y.
template<blas::vector Vector>
constexpr void Scale(Vector& x, const Vector& y, auto a) {
  VectorOperations<Vector>::Scale(x, y, a);
}

/// Compute @p x += @p a * @p y.
template<blas::vector Vector>
constexpr void AddAssign(Vector& x, const Vector& y, auto a) {
  VectorOperations<Vector>::AddAssign(x, y, a);
}

template<blas::vector Vector>
constexpr void AddAssign(Vector& x, const Vector& y) {
  VectorOperations<Vector>::AddAssign(x, y);
}

template<blas::vector Vector>
constexpr void Add(Vector& x, const Vector& y, const Vector& z) {
  VectorOperations<Vector>::Add(x, y, z);
}
template<blas::vector Vector>
constexpr void Add(Vector& x, const Vector& y, const Vector& z, auto b) {
  VectorOperations<Vector>::Add(x, y, z, b);
}
template<blas::vector Vector>
constexpr void Add(Vector& x,
                   const Vector& y,
                   auto a,
                   const Vector& z,
                   auto b) {
  VectorOperations<Vector>::Add(x, y, a, z, b);
}

template<blas::vector Vector>
constexpr void SubAssign(Vector& x, const Vector& y, auto a) {
  VectorOperations<Vector>::SubAssign(x, y, a);
}
template<blas::vector Vector>
constexpr void SubAssign(Vector& x, const Vector& y) {
  VectorOperations<Vector>::SubAssign(x, y);
}

template<blas::vector Vector>
constexpr void Sub(Vector& x, const Vector& y, const Vector& z) {
  VectorOperations<Vector>::Sub(x, y, z);
}
template<blas::vector Vector>
constexpr void Sub(Vector& x, const Vector& y, const Vector& z, auto b) {
  VectorOperations<Vector>::Sub(x, y, z, b);
}
template<blas::vector Vector>
constexpr void Sub(Vector& x,
                   const Vector& y,
                   auto a,
                   const Vector& z,
                   auto b) {
  VectorOperations<Vector>::Sub(x, y, a, z, b);
}

} // namespace Blas

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace blas {
/// Operator-like concept.
template<class Operator, class InVector, class OutVector = InVector>
concept op =
    requires(Operator& A, OutVector& y, const InVector& x) { A(y, x); };
} // namespace blas

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::ksp
