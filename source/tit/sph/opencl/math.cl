/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include "common.cl"

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Math
//

#ifndef Num
#error "`Num` type is not defined!"
#endif

// Make a numeric literal.
#define NUM(x) ((Num) (x))

// Tiny number.
#define TINY (NUM(cbrt(DBL_EPSILON)))

// Is the specified number tiny?
#define is_tiny(x) (fabs(x) <= TINY)

// Reciprocal of the number.
#define inverse(x) (NUM(1.0) / x)

// Average of two values.
#define avg(a, b) (((a) + (b)) / 2)

// Harmonic average of two values.
#define havg(a, b) (2 / (inverse(a) + inverse(b)))

// Power functions. Just for convenience.
#define pow2(x) pown((x), 2)
#define pow3(x) pown((x), 3)
#define pow4(x) pown((x), 4)
#define pow5(x) pown((x), 5)
#define pow6(x) pown((x), 6)
#define pow7(x) pown((x), 7)
#define pow8(x) pown((x), 8)

// Polynomial evaluation using Horner's method.
#define horner(x_, ...)                                                        \
  ({                                                                           \
    const typeof(x_) x = (x_);                                                 \
    const typeof(x_) coeffs[] = {__VA_ARGS__};                                 \
    const size_t size = countof(coeffs);                                       \
    typeof(x_) result = coeffs[size - 1];                                      \
    for (ssize_t i = size - 2; i >= 0; --i) result = result * x + coeffs[i];   \
    result;                                                                    \
  })

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Vectors
//

// Vector types.
#define Vec2 TIT_CAT(Num, 2)
#define Vec3 TIT_CAT(Num, 3)
#define Vec4 TIT_CAT(Num, 4)

// Vector length.
#define norm(a) length(a)
#define norm2(a)                                                               \
  ({                                                                           \
    const typeof(a) a_ = (a);                                                  \
    dot(a_, a_);                                                               \
  })

// Sum of the vector elements. Very inefficient.
#define sum(a_)                                                                \
  ({                                                                           \
    const typeof(a_) a = (a_);                                                 \
    typeof(a_[0]) result = a[0];                                               \
    for (size_t i = 1; i < countof(a); ++i) result += a[i];                    \
    result;                                                                    \
  })

// Filter the vector with a comparison result.
#define filter(a, m) select((a), ((typeof(a)) 0), (m))

// Vector outer product. I don't know how to make it generic.
#define vec2_outer(a_, b_)                                                     \
  ({                                                                           \
    const Vec2 a = (a_);                                                       \
    const Vec2 b = (b_);                                                       \
    (Mat2){a[0] * b, a[1] * b};                                                \
  })
#define vec3_outer(a, b)                                                       \
  ({                                                                           \
    const Vec3 _a = (a);                                                       \
    const Vec3 _b = (b);                                                       \
    (Mat3){_a[0] * _b, _a[1] * _b, _a[2] * _b};                                \
  })
#define vec4_outer(a_, b_)                                                     \
  ({                                                                           \
    const Vec4 a = (a_);                                                       \
    const Vec4 b = (b_);                                                       \
    (Mat4){a[0] * b, a[1] * b, a[2] * b, a[3] * b};                            \
  })

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Matrices
//

// Matrix types.
typedef struct {
  Vec2 rows[2];
} Mat2;
typedef struct {
  Vec3 rows[3];
} Mat3;
typedef struct {
  Vec4 rows[4];
} Mat4;

// Scale the matrix.
#define mat_scale(A_, s_)                                                      \
  ({                                                                           \
    typeof(A_) A = (A_);                                                       \
    const typeof(s_) s = (s_);                                                 \
    for (size_t i = 0; i < countof(A.rows); ++i) A.rows[i] *= s;               \
    A;                                                                         \
  })

// Add two matrices.
#define mat_add(A_, B_)                                                        \
  ({                                                                           \
    typeof(A_) A = (A_);                                                       \
    const typeof(B_) B = (B_);                                                 \
    for (size_t i = 0; i < countof(A.rows); ++i) A.rows[i] += B.rows[i];       \
    A;                                                                         \
  })

// Compute the LDL decomposition of a matrix.
#define mat_ldl(A, F)                                                          \
  ({                                                                           \
    bool ok = true;                                                            \
    const size_t dim = countof(A.rows);                                        \
    for (size_t i = 0; i < dim; ++i) {                                         \
      for (size_t j = 0; j < i; ++j) {                                         \
        F.rows[i][j] = A.rows[i][j];                                           \
        for (size_t k = 0; k < j; ++k) {                                       \
          F.rows[i][j] -= F.rows[i][k] * F.rows[k][k] * F.rows[j][k];          \
        }                                                                      \
        F.rows[i][j] /= F.rows[j][j];                                          \
      }                                                                        \
      F.rows[i][i] = A.rows[i][i];                                             \
      for (size_t k = 0; k < i; ++k) {                                         \
        F.rows[i][i] -= F.rows[i][k] * F.rows[k][k] * F.rows[i][k];            \
      }                                                                        \
      if (is_tiny(F.rows[i][i])) {                                             \
        ok = false;                                                            \
        break;                                                                 \
      }                                                                        \
    }                                                                          \
    ok;                                                                        \
  })

// Solve the system of equations with LDL factors.
#define mat_ldl_solve(F, b)                                                    \
  ({                                                                           \
    typeof(b) x = b;                                                           \
    const size_t dim = countof(F.rows);                                        \
    for (size_t i = 0; i < dim; ++i) {                                         \
      for (size_t k = 0; k < i; ++k) {                                         \
        x[i] -= F.rows[i][k] * x[k];                                           \
      }                                                                        \
    }                                                                          \
    for (size_t i = 0; i < dim; ++i) {                                         \
      x[i] /= F.rows[i][i];                                                    \
    }                                                                          \
    for (ssize_t i = dim - 1; i >= 0; --i) {                                   \
      for (size_t k = i + 1; k < dim; ++k) {                                   \
        x[i] -= F.rows[k][i] * x[k];                                           \
      }                                                                        \
    }                                                                          \
    x;                                                                         \
  })

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#ifndef Dim
#error "`Dim` constant is not defined!"
#endif

// Vector, matrix types and corresponding functions of the specified dimension.
#if Dim == 1
#define Vec Vec1
#define Mat Mat1
#define vec_outer(a, b) vec1_outer((a), (b))
#elif Dim == 2
#define Vec Vec2
#define Mat Mat2
#define vec_outer(a, b) vec2_outer((a), (b))
#elif Dim == 3
#define Vec Vec3
#define Mat Mat3
#define vec_outer(a, b) vec3_outer((a), (b))
#else
#error "Unsupported `Dim` value!"
#endif

// Same, but for the specified dimension plus one.
#if Dim == 1
#define VecP Vec2
#define MatP Mat2
#define vecP_outer(a, b) vec2_outer((a), (b))
#elif Dim == 2
#define VecP Vec3
#define MatP Mat3
#define vecP_outer(a, b) vec3_outer((a), (b))
#elif Dim == 3
#define VecP Vec4
#define MatP Mat4
#define vecP_outer(a, b) vec4_outer((a), (b))
#else
#error "Unsupported `Dim` value!"
#endif

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
