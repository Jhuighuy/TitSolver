/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "tit/core/basic_types.hpp"
#include "tit/core/mat.hpp"
#include "tit/core/vec.hpp"

#include "tit/core/mat/mat.testing.hpp"
#include "tit/testing/test.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("Mat::jacobi") {
  SUBCASE("1x1") {
    Mat const A{{2.0}};
    auto const eig = jacobi(A);
    REQUIRE(eig);
    auto const& [V, d] = *eig;
    CHECK(norm(V[0]) > 0.0);
    CHECK(all(approx_equal_to(V * A, diag(d) * V)));
  }
  SUBCASE("2x2") {
    SUBCASE("indefinite") {
      // clang-format off
      Mat const A = {
          { 1.0, -2.0},
          {-2.0,  1.0},
      };
      // clang-format on
      auto const eig = jacobi(A);
      REQUIRE(eig);
      auto const& [V, d] = *eig;
      for (size_t i = 0; i < 2; ++i) CHECK(norm(V[i]) > 0.0);
      CHECK(all(approx_equal_to(V * A, diag(d) * V)));
    }
  }
  SUBCASE("4x4") {
    SUBCASE("positive definite") {
      Mat const A = {
          {2.0, 1.0, 1.0, 0.0},
          {1.0, 3.0, 0.0, 1.0},
          {1.0, 0.0, 4.0, 1.0},
          {0.0, 1.0, 1.0, 2.0},
      };
      auto const eig = jacobi(A);
      REQUIRE(eig);
      auto const& [V, d] = *eig;
      for (size_t i = 0; i < 4; ++i) CHECK(norm(V[i]) > 0.0);
      CHECK(all(approx_equal_to(V * A, diag(d) * V)));
    }
  }
  SUBCASE("not converged") {
    Mat const A = {
        {2.0, 1.0, 1.0, 0.0},
        {1.0, 3.0, 0.0, 1.0},
        {1.0, 0.0, 4.0, 1.0},
        {0.0, 1.0, 1.0, 2.0},
    };
    // Not enough iterations for this threshold.
    auto const eig = jacobi(A, /*eps=*/1.0e-16, /*max_iter=*/3);
    REQUIRE(!eig);
    CHECK(eig.error() == MatEigError::not_converged);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
