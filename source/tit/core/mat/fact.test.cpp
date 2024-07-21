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
    const Mat A{{2.0}};
    const auto fact = lu(A);
    REQUIRE(fact);
    SUBCASE("factors") {
      const Mat L{{1.0}};
      const Mat U{{2.0}};
      REQUIRE(all(A == L * U));
      CHECK(all(approx_equal_to(fact->L(), L)));
      CHECK(all(approx_equal_to(fact->U(), U)));
    }
    SUBCASE("det") {
      CHECK(approx_equal_to(fact->det(), 2.0));
    }
    SUBCASE("solve") {
      const Vec b{6.0};
      const Vec x{3.0};
      REQUIRE(all(A * x == b));
      CHECK(all(approx_equal_to(fact->solve(b), x)));
    }
    SUBCASE("inverse") {
      const Mat A_inv{{0.5}};
      REQUIRE(all(approx_equal_to(A * A_inv, eye(A))));
      CHECK(all(approx_equal_to(fact->inverse(), A_inv)));
    }
  }
  SUBCASE("2x2") {
    const Mat A{
        {4.0, 3.0},
        {6.0, 3.0},
    };
    const auto fact = lu(A);
    REQUIRE(fact);
    SUBCASE("factors") {
      const Mat L{
          {1.0, 0.0},
          {1.5, 1.0},
      };
      // clang-format off
      const Mat U{
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
      const Vec b{7.0, 9.0};
      const Vec x{1.0, 1.0};
      REQUIRE(all(A * x == b));
      CHECK(all(approx_equal_to(fact->solve(b), x)));
    }
    SUBCASE("inverse") {
      // clang-format off
      const auto A_inv = Mat{
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
    const Mat A{
        { 2.0, -1.0, -2.0},
        {-4.0,  6.0,  3.0},
        {-4.0, -2.0,  8.0},
    };
    // clang-format on
    const auto fact = lu(A);
    REQUIRE(fact);
    SUBCASE("factors") {
      // clang-format off
      const Mat L{
          { 1.0,  0.0, 0.0},
          {-2.0,  1.0, 0.0},
          {-2.0, -1.0, 1.0},
      };
      const Mat U{
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
      const Vec b{24.0, 24.0, 24.0};
      const Vec x{75.0, 30.0, 48.0};
      REQUIRE(all(A * x == b));
      CHECK(all(approx_equal_to(fact->solve(b), x)));
    }
    SUBCASE("inverse") {
      // clang-format off
      const auto A_inv = Mat{
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
    const Mat A{
        { 1.0, -2.0,  3.0,  4.0},
        { 5.0,  6.0,  7.0,  8.0},
        { 9.0, 10.0, 11.0, 12.0},
        {13.0, 14.0, 15.0, 16.0},
    };
    // clang-format on
    const auto fact = lu(A);
    REQUIRE(!fact);
    CHECK(fact.error() == FactError::near_singular);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("Mat::chol") {
  SUBCASE("1x1") {
    const Mat A{{4.0}};
    const auto fact = chol(A);
    REQUIRE(fact);
    SUBCASE("factors") {
      const Mat L{{2.0}};
      REQUIRE(all(A == L * transpose(L)));
      CHECK(all(approx_equal_to(fact->L(), L)));
    }
    SUBCASE("det") {
      CHECK(approx_equal_to(fact->det(), 4.0));
    }
    SUBCASE("solve") {
      const Vec b{12.0};
      const Vec x{3.0};
      REQUIRE(all(A * x == b));
      CHECK(all(approx_equal_to(fact->solve(b), x)));
    }
    SUBCASE("inverse") {
      const Mat A_inv{{0.25}};
      REQUIRE(all(approx_equal_to(A * A_inv, eye(A))));
      CHECK(all(approx_equal_to(fact->inverse(), A_inv)));
    }
  }
  SUBCASE("3x3") {
    // clang-format off
    const Mat A{
        {  4.0,  12.0, -16.0},
        { 12.0,  37.0, -43.0},
        {-16.0, -43.0,  98.0},
    };
    // clang-format on
    const auto fact = chol(A);
    REQUIRE(fact);
    SUBCASE("factors") {
      // clang-format off
      const Mat L{
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
      const Vec b{9.0, 9.0, 9.0};
      const Vec x{341.25, -93.0, 15.0};
      REQUIRE(all(A * x == b));
      CHECK(all(approx_equal_to(fact->solve(b), x)));
    }
    SUBCASE("inverse") {
      // clang-format off
      const auto A_inv = Mat{
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
    const Mat A{
        {  4.0,  12.0, -16.0,  4.0},
        { 12.0,  35.0, -53.0, 14.0},
        {-16.0, -53.0,  48.0, 21.0},
        {  4.0,  14.0,  21.0, 80.0},
    };
    // clang-format on
    const auto fact = chol(A);
    REQUIRE(!fact);
    CHECK(fact.error() == FactError::not_positive_definite);
  }
  SUBCASE("4x4 singular") {
    // clang-format off
    const Mat A{
        {  4.0,  12.0, -16.0,  4.0},
        { 12.0,  36.0, -48.0, 12.0},
        {-16.0, -48.0,  73.0, 11.0},
        {  4.0,  12.0,  11.0, 86.0},
    };
    // clang-format on
    const auto fact = chol(A);
    REQUIRE(!fact);
    CHECK(fact.error() == FactError::near_singular);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("Mat::ldl") {
  SUBCASE("1x1") {
    const Mat A{{2.0}};
    const auto fact = ldl(A);
    REQUIRE(fact);
    SUBCASE("factors") {
      const Mat L{{1.0}};
      const Mat D{{2.0}};
      REQUIRE(all(A == L * D * transpose(L)));
      CHECK(all(approx_equal_to(fact->L(), L)));
      CHECK(all(approx_equal_to(fact->D(), D)));
    }
    SUBCASE("det") {
      CHECK(approx_equal_to(fact->det(), 2.0));
    }
    SUBCASE("solve") {
      const Vec b{6.0};
      const Vec x{3.0};
      REQUIRE(all(A * x == b));
      CHECK(all(approx_equal_to(fact->solve(b), x)));
    }
    SUBCASE("inverse") {
      const Mat A_inv{{0.5}};
      REQUIRE(all(approx_equal_to(A * A_inv, eye(A))));
      CHECK(all(approx_equal_to(fact->inverse(), A_inv)));
    }
  }
  SUBCASE("3x3") {
    // clang-format off
    const Mat A{
        {  4.0,  12.0, -16.0},
        { 12.0,  37.0, -43.0},
        {-16.0, -43.0,  98.0},
    };
    // clang-format on
    const auto fact = ldl(A);
    REQUIRE(fact);
    SUBCASE("factors") {
      // clang-format off
      const Mat L{
          { 1.0, 0.0, 0.0},
          { 3.0, 1.0, 0.0},
          {-4.0, 5.0, 1.0},
      };
      // clang-format on
      const Mat D{
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
      const Vec b{9.0, 9.0, 9.0};
      const Vec x{341.25, -93.0, 15.0};
      REQUIRE(all(A * x == b));
      CHECK(all(approx_equal_to(fact->solve(b), x)));
    }
    SUBCASE("inverse") {
      // clang-format off
      const auto A_inv = Mat{
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
    const Mat A{
        {  4.0,  12.0, -16.0,  4.0},
        { 12.0,  35.0, -53.0, 14.0},
        {-16.0, -53.0,  48.0, 21.0},
        {  4.0,  14.0,  21.0, 80.0},
    };
    // clang-format on
    const auto fact = ldl(A);
    REQUIRE(fact);
    SUBCASE("factors") {
      // clang-format off
      const Mat L{
          { 1.0,  0.0, 0.0, 0.0},
          { 3.0,  1.0, 0.0, 0.0},
          {-4.0,  5.0, 1.0, 0.0},
          { 1.0, -2.0, 3.0, 1.0},
      };
      const Mat D{
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
      const Vec b{9.0, 9.0, 9.0, 9.0};
      const Vec x{-27990.75, 7440.0, -1308.0, 441.0};
      REQUIRE(all(A * x == b));
      CHECK(all(approx_equal_to(fact->solve(b), x)));
    }
    SUBCASE("inverse") {
      // clang-format off
      const auto A_inv = Mat{
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
    const Mat A{
        {  4.0,  12.0, -16.0,  4.0},
        { 12.0,  35.0, -53.0, 14.0},
        {-16.0, -53.0,  39.0, -6.0},
        {  4.0,  14.0,  -6.0, -1.0},
    };
    // clang-format on
    const auto fact = ldl(A);
    REQUIRE(!fact);
    CHECK(fact.error() == FactError::near_singular);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
