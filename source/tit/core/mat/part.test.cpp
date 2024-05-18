/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "tit/core/mat.hpp"
#include "tit/core/vec.hpp"

#include "tit/core/mat/mat.testing.hpp"
#include "tit/core/mat/part.hpp"
#include "tit/testing/test.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("Mat::part_at") {
  const Mat A{
      {1.0, 2.0, 3.0},
      {4.0, 5.0, 6.0},
      {7.0, 8.0, 9.0},
  };
  using enum MatPart;
  SUBCASE("diag") {
    CHECK(part_at<diag>(A, 0, 1) == 0.0);
    CHECK(part_at<diag>(A, 1, 1) == 5.0);
    CHECK(part_at<diag>(A, 1, 1) == 5.0);
  }
  SUBCASE("lower") {
    CHECK(part_at<lower>(A, 1, 0) == 4.0);
    CHECK(part_at<lower>(A, 0, 1) == 0.0);
    CHECK(part_at<lower>(A, 1, 1) == 0.0);
    CHECK(part_at<lower | diag>(A, 1, 1) == 5.0);
    CHECK(part_at<lower | unit>(A, 1, 1) == 1.0);
    CHECK(part_at<lower | transposed>(A, 1, 0) == 2.0);
    CHECK(part_at<lower | transposed>(A, 0, 1) == 0.0);
  }
  SUBCASE("upper") {
    CHECK(part_at<upper>(A, 0, 1) == 2.0);
    CHECK(part_at<upper>(A, 1, 0) == 0.0);
    CHECK(part_at<upper>(A, 1, 1) == 0.0);
    CHECK(part_at<upper | diag>(A, 1, 1) == 5.0);
    CHECK(part_at<upper | unit>(A, 1, 1) == 1.0);
    CHECK(part_at<upper | transposed>(A, 0, 1) == 4.0);
    CHECK(part_at<upper | transposed>(A, 1, 0) == 0.0);
  }
  SUBCASE("weird") {
    CHECK(part_at<lower | upper>(A, 1, 1) == 0.0);
    CHECK(part_at<lower | upper>(A, 0, 1) == 2.0);
    CHECK(part_at<lower | upper>(A, 1, 0) == 4.0);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("Mat::copy_part") {
  const Mat A{
      {1.0, 2.0, 3.0},
      {4.0, 5.0, 6.0},
      {7.0, 8.0, 9.0},
  };
  using enum MatPart;
  SUBCASE("diag") {
    CHECK(all(copy_part<diag>(A) == //
              Mat{
                  {1.0, 0.0, 0.0},
                  {0.0, 5.0, 0.0},
                  {0.0, 0.0, 9.0},
              }));
  }
  SUBCASE("lower") {
    CHECK(all(copy_part<lower | unit>(A) == //
              Mat{
                  {1.0, 0.0, 0.0},
                  {4.0, 1.0, 0.0},
                  {7.0, 8.0, 1.0},
              }));
    CHECK(all(copy_part<lower | diag>(A) == //
              Mat{
                  {1.0, 0.0, 0.0},
                  {4.0, 5.0, 0.0},
                  {7.0, 8.0, 9.0},
              }));
    CHECK(all(copy_part<lower | diag | transposed>(A) == //
              Mat{
                  {1.0, 0.0, 0.0},
                  {2.0, 5.0, 0.0},
                  {3.0, 6.0, 9.0},
              }));
  }
  SUBCASE("upper") {
    CHECK(all(copy_part<upper | unit>(A) == //
              Mat{
                  {1.0, 2.0, 3.0},
                  {0.0, 1.0, 6.0},
                  {0.0, 0.0, 1.0},
              }));
    CHECK(all(copy_part<upper | diag>(A) == //
              Mat{
                  {1.0, 2.0, 3.0},
                  {0.0, 5.0, 6.0},
                  {0.0, 0.0, 9.0},
              }));
    CHECK(all(copy_part<upper | diag | transposed>(A) == //
              Mat{
                  {1.0, 4.0, 7.0},
                  {0.0, 5.0, 8.0},
                  {0.0, 0.0, 9.0},
              }));
  }
  SUBCASE("weird") {
    CHECK(all(copy_part<lower | upper>(A) == //
              Mat{
                  {0.0, 2.0, 3.0},
                  {4.0, 0.0, 6.0},
                  {7.0, 8.0, 0.0},
              }));
    CHECK(all(copy_part<lower | unit | upper>(A) == //
              Mat{
                  {1.0, 2.0, 3.0},
                  {4.0, 1.0, 6.0},
                  {7.0, 8.0, 1.0},
              }));
    CHECK(all(copy_part<lower | diag | upper>(A) == A));
  }
}

TEST_CASE("Mat::transpose") {
  CHECK(all(transpose(Mat{
                {1.0, 2.0},
                {3.0, 4.0},
            }) == //
            Mat{
                {1.0, 3.0},
                {2.0, 4.0},
            }));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("Mat::part_solve_inplace") {
  // clang-format off
  Mat const A{
      { 2.0, -1.0, -2.0},
      {-2.0,  4.0, -1.0},
      {-2.0, -1.0,  5.0},
  };
  // clang-format on
  const Vec x{1.0, 2.0, 3.0};
  using enum MatPart;
  SUBCASE("diag") {
    Vec b{2.0, 8.0, 15.0};
    REQUIRE(all(copy_part<diag>(A) * x == b));
    part_solve_inplace<diag>(A, b);
    CHECK(all(approx_equal_to(b, x)));
  }
  SUBCASE("lower") {
    SUBCASE("unit") {
      Vec b{1.0, 0.0, -1.0};
      REQUIRE(all(copy_part<lower_unit>(A) * x == b));
      part_solve_inplace<lower_unit>(A, b);
      CHECK(all(approx_equal_to(b, x)));
    }
    SUBCASE("diag") {
      Vec b{2.0, 6.0, 11.0};
      REQUIRE(all(copy_part<lower_diag>(A) * x == b));
      part_solve_inplace<lower_diag>(A, b);
      CHECK(all(approx_equal_to(b, x)));
    }
    SUBCASE("transposed") {
      Vec b{2.0, 7.0, 11.0};
      REQUIRE(all(copy_part<lower_diag | transposed>(A) * x == b));
      part_solve_inplace<lower_diag | transposed>(A, b);
      CHECK(all(approx_equal_to(b, x)));
    }
  }
  SUBCASE("upper") {
    SUBCASE("unit") {
      Vec b{-7.0, -1.0, 3.0};
      REQUIRE(all(copy_part<upper_unit>(A) * x == b));
      part_solve_inplace<upper_unit>(A, b);
      CHECK(all(approx_equal_to(b, x)));
    }
    SUBCASE("diag") {
      Vec b{-6.0, 5.0, 15.0};
      REQUIRE(all(copy_part<upper_diag>(A) * x == b));
      part_solve_inplace<upper_diag>(A, b);
      CHECK(all(approx_equal_to(b, x)));
    }
    SUBCASE("transposed") {
      Vec b{-8.0, 5.0, 15.0};
      REQUIRE(all(copy_part<upper_diag | transposed>(A) * x == b));
      part_solve_inplace<upper_diag | transposed>(A, b);
      CHECK(all(approx_equal_to(b, x)));
    }
  }
  SUBCASE("multiple") {
    Vec b{-14.0, 10.0, 30.0};
    REQUIRE(all(copy_part<lower_diag>(A) * copy_part<upper_unit>(A) * x == b));
    part_solve_inplace<lower_diag, upper_unit>(A, b);
    CHECK(all(approx_equal_to(b, x)));
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
