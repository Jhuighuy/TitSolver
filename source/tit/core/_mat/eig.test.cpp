/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "tit/core/basic_types.hpp"
#include "tit/core/mat.hpp"
#include "tit/core/vec.hpp"

#include "tit/testing/test.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("Mat::jacobi") {
  SUBCASE("1x1") {
    const Mat A{{2.0}};
    const auto eig = jacobi(A);
    REQUIRE(eig);
    const auto& [V, d] = *eig;
    CHECK(norm(V[0]) > 0.0);
    CHECK_APPROX_EQ(V * A, diag(d) * V);
  }
  SUBCASE("2x2") {
    SUBCASE("indefinite") {
      const Mat A{
          {1.0, -2.0},
          {-2.0, 1.0},
      };
      const auto eig = jacobi(A);
      REQUIRE(eig);
      const auto& [V, d] = *eig;
      for (size_t i = 0; i < 2; ++i) CHECK(norm(V[i]) > 0.0);
      CHECK_APPROX_EQ(V * A, diag(d) * V);
    }
  }
  SUBCASE("4x4") {
    SUBCASE("positive definite") {
      const Mat A{
          {2.0, 1.0, 1.0, 0.0},
          {1.0, 3.0, 0.0, 1.0},
          {1.0, 0.0, 4.0, 1.0},
          {0.0, 1.0, 1.0, 2.0},
      };
      const auto eig = jacobi(A);
      REQUIRE(eig);
      const auto& [V, d] = *eig;
      for (size_t i = 0; i < 4; ++i) CHECK(norm(V[i]) > 0.0);
      CHECK_APPROX_EQ(V * A, diag(d) * V);
    }
  }
  SUBCASE("not converged") {
    const Mat A{
        {2.0, 1.0, 1.0, 0.0},
        {1.0, 3.0, 0.0, 1.0},
        {1.0, 0.0, 4.0, 1.0},
        {0.0, 1.0, 1.0, 2.0},
    };
    // Not enough iterations for this threshold.
    const auto eig = jacobi(A, /*eps=*/1.0e-16, /*max_iter=*/3);
    REQUIRE(!eig);
    CHECK(eig.error() == MatEigError::not_converged);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
