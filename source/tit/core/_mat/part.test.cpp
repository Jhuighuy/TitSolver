/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "tit/core/mat.hpp"
#include "tit/core/vec.hpp"
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
    CHECK(part_at<(lower | diag)>(A, 1, 1) == 5.0);
    CHECK(part_at<(lower | unit)>(A, 1, 1) == 1.0);
    CHECK(part_at<(lower | transposed)>(A, 1, 0) == 2.0);
    CHECK(part_at<(lower | transposed)>(A, 0, 1) == 0.0);
  }
  SUBCASE("upper") {
    CHECK(part_at<upper>(A, 0, 1) == 2.0);
    CHECK(part_at<upper>(A, 1, 0) == 0.0);
    CHECK(part_at<upper>(A, 1, 1) == 0.0);
    CHECK(part_at<(upper | diag)>(A, 1, 1) == 5.0);
    CHECK(part_at<(upper | unit)>(A, 1, 1) == 1.0);
    CHECK(part_at<(upper | transposed)>(A, 0, 1) == 4.0);
    CHECK(part_at<(upper | transposed)>(A, 1, 0) == 0.0);
  }
  SUBCASE("weird") {
    CHECK(part_at<(lower | upper)>(A, 1, 1) == 0.0);
    CHECK(part_at<(lower | upper)>(A, 0, 1) == 2.0);
    CHECK(part_at<(lower | upper)>(A, 1, 0) == 4.0);
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
    CHECK(copy_part<diag>(A) == Mat{
                                    {1.0, 0.0, 0.0},
                                    {0.0, 5.0, 0.0},
                                    {0.0, 0.0, 9.0},
                                });
  }
  SUBCASE("lower") {
    CHECK(copy_part<(lower | unit)>(A) == Mat{
                                              {1.0, 0.0, 0.0},
                                              {4.0, 1.0, 0.0},
                                              {7.0, 8.0, 1.0},
                                          });
    CHECK(copy_part<(lower | diag)>(A) == Mat{
                                              {1.0, 0.0, 0.0},
                                              {4.0, 5.0, 0.0},
                                              {7.0, 8.0, 9.0},
                                          });
    CHECK(copy_part<(lower | diag | transposed)>(A) == Mat{
                                                           {1.0, 0.0, 0.0},
                                                           {2.0, 5.0, 0.0},
                                                           {3.0, 6.0, 9.0},
                                                       });
  }
  SUBCASE("upper") {
    CHECK(copy_part<(upper | unit)>(A) == Mat{
                                              {1.0, 2.0, 3.0},
                                              {0.0, 1.0, 6.0},
                                              {0.0, 0.0, 1.0},
                                          });
    CHECK(copy_part<(upper | diag)>(A) == Mat{
                                              {1.0, 2.0, 3.0},
                                              {0.0, 5.0, 6.0},
                                              {0.0, 0.0, 9.0},
                                          });
    CHECK(copy_part<(upper | diag | transposed)>(A) == Mat{
                                                           {1.0, 4.0, 7.0},
                                                           {0.0, 5.0, 8.0},
                                                           {0.0, 0.0, 9.0},
                                                       });
  }
  SUBCASE("weird") {
    CHECK(copy_part<(lower | upper)>(A) == Mat{
                                               {0.0, 2.0, 3.0},
                                               {4.0, 0.0, 6.0},
                                               {7.0, 8.0, 0.0},
                                           });
    CHECK(copy_part<(lower | unit | upper)>(A) == Mat{
                                                      {1.0, 2.0, 3.0},
                                                      {4.0, 1.0, 6.0},
                                                      {7.0, 8.0, 1.0},
                                                  });
    CHECK(copy_part<(lower | diag | upper)>(A) == A);
  }
}

TEST_CASE("Mat::transpose") {
  CHECK(transpose(Mat{
            {1.0, 2.0},
            {3.0, 4.0},
        }) == Mat{
                  {1.0, 3.0},
                  {2.0, 4.0},
              });
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("Mat::part_solve_inplace") {
  // clang-format off
  const Mat A{
      { 2.0, -1.0, -2.0},
      {-2.0,  4.0, -1.0},
      {-2.0, -1.0,  5.0},
  };
  // clang-format on
  const Vec x{1.0, 2.0, 3.0};
  using enum MatPart;
  SUBCASE("diag") {
    Vec b{2.0, 8.0, 15.0};
    REQUIRE(copy_part<diag>(A) * x == b);
    part_solve_inplace<diag>(A, b);
    CHECK_APPROX_EQ(b, x);
  }
  SUBCASE("lower") {
    SUBCASE("unit") {
      Vec b{1.0, 0.0, -1.0};
      REQUIRE(copy_part<lower_unit>(A) * x == b);
      part_solve_inplace<lower_unit>(A, b);
      CHECK_APPROX_EQ(b, x);
    }
    SUBCASE("diag") {
      Vec b{2.0, 6.0, 11.0};
      REQUIRE(copy_part<lower_diag>(A) * x == b);
      part_solve_inplace<lower_diag>(A, b);
      CHECK_APPROX_EQ(b, x);
    }
    SUBCASE("transposed") {
      Vec b{2.0, 7.0, 11.0};
      REQUIRE(copy_part<(lower_diag | transposed)>(A) * x == b);
      part_solve_inplace<lower_diag | transposed>(A, b);
      CHECK_APPROX_EQ(b, x);
    }
  }
  SUBCASE("upper") {
    SUBCASE("unit") {
      Vec b{-7.0, -1.0, 3.0};
      REQUIRE(copy_part<upper_unit>(A) * x == b);
      part_solve_inplace<upper_unit>(A, b);
      CHECK_APPROX_EQ(b, x);
    }
    SUBCASE("diag") {
      Vec b{-6.0, 5.0, 15.0};
      REQUIRE(copy_part<upper_diag>(A) * x == b);
      part_solve_inplace<upper_diag>(A, b);
      CHECK_APPROX_EQ(b, x);
    }
    SUBCASE("transposed") {
      Vec b{-8.0, 5.0, 15.0};
      REQUIRE(copy_part<(upper_diag | transposed)>(A) * x == b);
      part_solve_inplace<upper_diag | transposed>(A, b);
      CHECK_APPROX_EQ(b, x);
    }
  }
  SUBCASE("multiple") {
    Vec b{-14.0, 10.0, 30.0};
    REQUIRE(copy_part<lower_diag>(A) * copy_part<upper_unit>(A) * x == b);
    part_solve_inplace<lower_diag, upper_unit>(A, b);
    CHECK_APPROX_EQ(b, x);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
