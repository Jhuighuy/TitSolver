/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "tit/core/mat.hpp"
#include "tit/core/math.hpp"
#include "tit/core/vec.hpp"

#include "tit/core/mat/mat.testing.hpp"
#include "tit/testing/test.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("Mat::lu") {
  SUBCASE("1x1") {
    Mat const A{{2.0}};
    auto const fact = lu(A);
    REQUIRE(fact);
    SUBCASE("factors") {
      Mat const L{{1.0}};
      Mat const U{{2.0}};
      REQUIRE(all(A == L * U));
      CHECK(all(approx_equal_to(fact->L(), L)));
      CHECK(all(approx_equal_to(fact->U(), U)));
    }
    SUBCASE("det") {
      CHECK(approx_equal_to(fact->det(), 2.0));
    }
    SUBCASE("solve") {
      Vec const b{6.0};
      Vec const x{3.0};
      REQUIRE(all(A * x == b));
      CHECK(all(approx_equal_to(fact->solve(b), x)));
    }
    SUBCASE("inverse") {
      Mat const A_inv{{0.5}};
      REQUIRE(all(approx_equal_to(A * A_inv, eye(A))));
      CHECK(all(approx_equal_to(fact->inverse(), A_inv)));
    }
  }
  SUBCASE("2x2") {
    Mat const A{
        {4.0, 3.0},
        {6.0, 3.0},
    };
    auto const fact = lu(A);
    REQUIRE(fact);
    SUBCASE("factors") {
      Mat const L{
          {1.0, 0.0},
          {1.5, 1.0},
      };
      // clang-format off
      Mat const U{
          {4.0,  3.0},
          {0.0, -1.5},
      };
      // clang-format on
      REQUIRE(all(A == L * U));
      CHECK(all(approx_equal_to(fact->L(), L)));
      CHECK(all(approx_equal_to(fact->U(), U)));
    }
    SUBCASE("det") {
      CHECK(approx_equal_to(fact->det(), -6.0));
    }
    SUBCASE("solve") {
      Vec const b{7.0, 9.0};
      Vec const x{1.0, 1.0};
      REQUIRE(all(A * x == b));
      CHECK(all(approx_equal_to(fact->solve(b), x)));
    }
    SUBCASE("inverse") {
      // clang-format off
      auto const A_inv = Mat{
          {-3.0,  3.0},
          { 6.0, -4.0},
      } / 6.0;
      // clang-format on
      REQUIRE(all(approx_equal_to(A * A_inv, eye(A))));
      CHECK(all(approx_equal_to(fact->inverse(), A_inv)));
    }
  }
  SUBCASE("3x3") {
    // clang-format off
    Mat const A{
        { 2.0, -1.0, -2.0},
        {-4.0,  6.0,  3.0},
        {-4.0, -2.0,  8.0},
    };
    // clang-format on
    auto const fact = lu(A);
    REQUIRE(fact);
    SUBCASE("factors") {
      // clang-format off
      Mat const L{
          { 1.0,  0.0, 0.0},
          {-2.0,  1.0, 0.0},
          {-2.0, -1.0, 1.0},
      };
      Mat const U{
          {2.0, -1.0, -2.0},
          {0.0,  4.0, -1.0},
          {0.0,  0.0,  3.0},
      };
      // clang-format on
      REQUIRE(all(A == L * U));
      CHECK(all(approx_equal_to(fact->L(), L)));
      CHECK(all(approx_equal_to(fact->U(), U)));
    }
    SUBCASE("det") {
      CHECK(approx_equal_to(fact->det(), 24.0));
    }
    SUBCASE("solve") {
      Vec const b{24.0, 24.0, 24.0};
      Vec const x{75.0, 30.0, 48.0};
      REQUIRE(all(A * x == b));
      CHECK(all(approx_equal_to(fact->solve(b), x)));
    }
    SUBCASE("inverse") {
      // clang-format off
      auto const A_inv = Mat{
          {6.75, 1.5, 1.125},
          { 2.5, 1.0,  0.25},
          { 4.0, 1.0,   1.0},
      } / 3.0;
      // clang-format on
      REQUIRE(all(approx_equal_to(A * A_inv, eye(A))));
      CHECK(all(approx_equal_to(fact->inverse(), A_inv)));
    }
  }
  SUBCASE("4x4 singular") {
    // clang-format off
    Mat const A{
        { 1.0, -2.0,  3.0,  4.0},
        { 5.0,  6.0,  7.0,  8.0},
        { 9.0, 10.0, 11.0, 12.0},
        {13.0, 14.0, 15.0, 16.0},
    };
    // clang-format on
    auto const fact = lu(A);
    REQUIRE(!fact);
    CHECK(fact.error() == FactError::near_singular);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("Mat::chol") {
  SUBCASE("1x1") {
    Mat const A{{4.0}};
    auto const fact = chol(A);
    REQUIRE(fact);
    SUBCASE("factors") {
      Mat const L{{2.0}};
      REQUIRE(all(A == L * transpose(L)));
      CHECK(all(approx_equal_to(fact->L(), L)));
    }
    SUBCASE("det") {
      CHECK(approx_equal_to(fact->det(), 4.0));
    }
    SUBCASE("solve") {
      Vec const b{12.0};
      Vec const x{3.0};
      REQUIRE(all(A * x == b));
      CHECK(all(approx_equal_to(fact->solve(b), x)));
    }
    SUBCASE("inverse") {
      Mat const A_inv{{0.25}};
      REQUIRE(all(approx_equal_to(A * A_inv, eye(A))));
      CHECK(all(approx_equal_to(fact->inverse(), A_inv)));
    }
  }
  SUBCASE("3x3") {
    // clang-format off
    Mat const A{
        {  4.0,  12.0, -16.0},
        { 12.0,  37.0, -43.0},
        {-16.0, -43.0,  98.0},
    };
    // clang-format on
    auto const fact = chol(A);
    REQUIRE(fact);
    SUBCASE("factors") {
      // clang-format off
      Mat const L{
          { 2.0, 0.0, 0.0},
          { 6.0, 1.0, 0.0},
          {-8.0, 5.0, 3.0},
      };
      // clang-format on
      REQUIRE(all(A == L * transpose(L)));
      CHECK(all(approx_equal_to(fact->L(), L)));
    }
    SUBCASE("det") {
      CHECK(approx_equal_to(fact->det(), 36.0));
    }
    SUBCASE("solve") {
      Vec const b{9.0, 9.0, 9.0};
      Vec const x{341.25, -93.0, 15.0};
      REQUIRE(all(A * x == b));
      CHECK(all(approx_equal_to(fact->solve(b), x)));
    }
    SUBCASE("inverse") {
      // clang-format off
      auto const A_inv = Mat{
          {444.25, -122.0, 19.0},
          {-122.0,   34.0, -5.0},
          {  19.0,   -5.0,  1.0},
      } / 9.0;
      // clang-format on
      REQUIRE(all(approx_equal_to(A * A_inv, eye(A))));
      CHECK(all(approx_equal_to(fact->inverse(), A_inv)));
    }
  }
  SUBCASE("4x4 indefinite") {
    // clang-format off
    Mat const A{
        {  4.0,  12.0, -16.0,  4.0},
        { 12.0,  35.0, -53.0, 14.0},
        {-16.0, -53.0,  48.0, 21.0},
        {  4.0,  14.0,  21.0, 80.0},
    };
    // clang-format on
    auto const fact = chol(A);
    REQUIRE(!fact);
    CHECK(fact.error() == FactError::not_positive_definite);
  }
  SUBCASE("4x4 singular") {
    // clang-format off
    Mat const A{
        {  4.0,  12.0, -16.0,  4.0},
        { 12.0,  36.0, -48.0, 12.0},
        {-16.0, -48.0,  73.0, 11.0},
        {  4.0,  12.0,  11.0, 86.0},
    };
    // clang-format on
    auto const fact = chol(A);
    REQUIRE(!fact);
    CHECK(fact.error() == FactError::near_singular);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("Mat::ldl") {
  SUBCASE("1x1") {
    Mat const A{{2.0}};
    auto const fact = ldl(A);
    REQUIRE(fact);
    SUBCASE("factors") {
      Mat const L{{1.0}};
      Mat const D{{2.0}};
      REQUIRE(all(A == L * D * transpose(L)));
      CHECK(all(approx_equal_to(fact->L(), L)));
      CHECK(all(approx_equal_to(fact->D(), D)));
    }
    SUBCASE("det") {
      CHECK(approx_equal_to(fact->det(), 2.0));
    }
    SUBCASE("solve") {
      Vec const b{6.0};
      Vec const x{3.0};
      REQUIRE(all(A * x == b));
      CHECK(all(approx_equal_to(fact->solve(b), x)));
    }
    SUBCASE("inverse") {
      Mat const A_inv{{0.5}};
      REQUIRE(all(approx_equal_to(A * A_inv, eye(A))));
      CHECK(all(approx_equal_to(fact->inverse(), A_inv)));
    }
  }
  SUBCASE("3x3") {
    // clang-format off
    Mat const A{
        {  4.0,  12.0, -16.0},
        { 12.0,  37.0, -43.0},
        {-16.0, -43.0,  98.0},
    };
    // clang-format on
    auto const fact = ldl(A);
    REQUIRE(fact);
    SUBCASE("factors") {
      // clang-format off
      Mat const L{
          { 1.0, 0.0, 0.0},
          { 3.0, 1.0, 0.0},
          {-4.0, 5.0, 1.0},
      };
      // clang-format on
      Mat const D{
          {4.0, 0.0, 0.0},
          {0.0, 1.0, 0.0},
          {0.0, 0.0, 9.0},
      };
      REQUIRE(all(A == L * D * transpose(L)));
      CHECK(all(approx_equal_to(fact->L(), L)));
      CHECK(all(approx_equal_to(fact->D(), D)));
    }
    SUBCASE("det") {
      CHECK(approx_equal_to(fact->det(), 36.0));
    }
    SUBCASE("solve") {
      Vec const b{9.0, 9.0, 9.0};
      Vec const x{341.25, -93.0, 15.0};
      REQUIRE(all(A * x == b));
      CHECK(all(approx_equal_to(fact->solve(b), x)));
    }
    SUBCASE("inverse") {
      // clang-format off
      auto const A_inv = Mat{
          {444.25, -122.0, 19.0},
          {-122.0,   34.0, -5.0},
          {  19.0,   -5.0,  1.0},
      } / 9.0;
      // clang-format on
      REQUIRE(all(approx_equal_to(A * A_inv, eye(A))));
      CHECK(all(approx_equal_to(fact->inverse(), A_inv)));
    }
  }
  SUBCASE("4x4 indefinite") {
    // clang-format off
    Mat const A{
        {  4.0,  12.0, -16.0,  4.0},
        { 12.0,  35.0, -53.0, 14.0},
        {-16.0, -53.0,  48.0, 21.0},
        {  4.0,  14.0,  21.0, 80.0},
    };
    // clang-format on
    auto const fact = ldl(A);
    REQUIRE(fact);
    SUBCASE("factors") {
      // clang-format off
      Mat const L{
          { 1.0,  0.0, 0.0, 0.0},
          { 3.0,  1.0, 0.0, 0.0},
          {-4.0,  5.0, 1.0, 0.0},
          { 1.0, -2.0, 3.0, 1.0},
      };
      Mat const D{
          {4.0,  0.0, 0.0,  0.0},
          {0.0, -1.0, 0.0,  0.0},
          {0.0,  0.0, 9.0,  0.0},
          {0.0,  0.0, 0.0, -1.0},
      };
      // clang-format on
      REQUIRE(all(A == L * D * transpose(L)));
      CHECK(all(approx_equal_to(fact->L(), L)));
      CHECK(all(approx_equal_to(fact->D(), D)));
    }
    SUBCASE("det") {
      CHECK(approx_equal_to(fact->det(), 36.0));
    }
    SUBCASE("solve") {
      Vec const b{9.0, 9.0, 9.0, 9.0};
      Vec const x{-27990.75, 7440.0, -1308.0, 441.0};
      REQUIRE(all(A * x == b));
      CHECK(all(approx_equal_to(fact->solve(b), x)));
    }
    SUBCASE("inverse") {
      // clang-format off
      auto const A_inv = Mat{
          {-36581.75,  9724.0, -1709.0,  576.0},
          {   9724.0, -2585.0,   454.0, -153.0},
          {  -1709.0,   454.0,   -80.0,   27.0},
          {    576.0,  -153.0,    27.0,   -9.0},
      } / 9.0;
      // clang-format on
      REQUIRE(all(approx_equal_to(A * A_inv, eye(A))));
      CHECK(all(approx_equal_to(fact->inverse(), A_inv)));
    }
  }
  SUBCASE("4x4 singular") {
    // clang-format off
    Mat const A{
        {  4.0,  12.0, -16.0,  4.0},
        { 12.0,  35.0, -53.0, 14.0},
        {-16.0, -53.0,  39.0, -6.0},
        {  4.0,  14.0,  -6.0, -1.0},
    };
    // clang-format on
    auto const fact = ldl(A);
    REQUIRE(!fact);
    CHECK(fact.error() == FactError::near_singular);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
