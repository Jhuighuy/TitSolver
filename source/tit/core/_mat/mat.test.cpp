/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <format>

#include "tit/core/mat.hpp"
#include "tit/core/vec.hpp"

#include "tit/testing/test.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("Mat") {
  SUBCASE("zero initialization") {
    Mat<double, 2> M{};
    CHECK(M[0] == Vec<double, 2>{});
    CHECK(M[1] == Vec<double, 2>{});
  }
  SUBCASE("zero assignment") {
    Mat M{
        {1.0, 2.0},
        {3.0, 4.0},
    };
    M = {};
    CHECK(M[0] == Vec<double, 2>{});
    CHECK(M[1] == Vec<double, 2>{});
  }
  SUBCASE("value initialization") {
    const Mat<double, 2> M(3.0);
    CHECK(M[0] == Vec{3.0, 0.0});
    CHECK(M[1] == Vec{0.0, 3.0});
  }
  SUBCASE("aggregate initialization") {
    const Mat M{{1.0, 2.0}, {3.0, 4.0}};
    CHECK(M[0] == Vec{1.0, 2.0});
    CHECK(M[1] == Vec{3.0, 4.0});
  }
  SUBCASE("aggregate assignment") {
    Mat<double, 2> M;
    M = Mat{
        {1.0, 2.0},
        {3.0, 4.0},
    };
    CHECK(M[0] == Vec{1.0, 2.0});
    CHECK(M[1] == Vec{3.0, 4.0});
  }
  SUBCASE("vector subscript") {
    Mat<double, 2> M;
    M[0] = {1.0, 2.0};
    M[1] = {3.0, 4.0};
    CHECK(M[0] == Vec{1.0, 2.0});
    CHECK(M[1] == Vec{3.0, 4.0});
  }
  SUBCASE("scalar subscript") {
    Mat<double, 2> M;
    M[0][0] = 1.0;
    M[0][1] = 2.0;
    M[1][0] = 3.0;
    M[1][1] = 4.0;
    CHECK(M[0, 0] == 1.0);
    CHECK(M[0, 1] == 2.0);
    CHECK(M[1, 0] == 3.0);
    CHECK(M[1, 1] == 4.0);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("zero") {
  const Mat M{
      {1.0, 0.0},
      {0.0, 1.0},
  };
  const Mat O{
      {0.0, 0.0},
      {0.0, 0.0},
  };
  CHECK(zero(M) == O);
}

TEST_CASE("Mat::eye") {
  const Mat M{
      {1.0, 0.0},
      {0.0, 1.0},
  };
  const Mat I{
      {1.0, 0.0},
      {0.0, 1.0},
  };
  CHECK(eye(M) == I);
}

TEST_CASE("Mat::diag") {
  SUBCASE("to vector") {
    CHECK(diag(Mat{
              {1.0, 1.0, 1.0},
              {1.0, 2.0, 1.0},
              {1.0, 1.0, 3.0},
          }) == //
          Vec{1.0, 2.0, 3.0});
  }
  SUBCASE("to matrix") {
    CHECK(diag(Vec{1.0, 2.0, 3.0}) == //
          Mat{
              {1.0, 0.0, 0.0},
              {0.0, 2.0, 0.0},
              {0.0, 0.0, 3.0},
          });
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("Mat::operator+") {
  SUBCASE("normal") {
    CHECK((Mat{
               {1.0, 2.0},
               {3.0, 4.0},
           } +
           Mat{
               {5.0, 6.0},
               {7.0, 8.0},
           }) == //
          Mat{
              {6.0, 8.0},
              {10.0, 12.0},
          });
  }
  SUBCASE("with assignment") {
    Mat M{
        {1.0, 2.0},
        {3.0, 4.0},
    };
    M += Mat{
        {5.0, 6.0},
        {7.0, 8.0},
    };
    CHECK(M == //
          Mat{
              {6.0, 8.0},
              {10.0, 12.0},
          });
  }
}

TEST_CASE("Mat::operator-") {
  SUBCASE("negation") {
    CHECK(-Mat{
              {1.0, 2.0},
              {3.0, 4.0},
          } == //
          Mat{
              {-1.0, -2.0},
              {-3.0, -4.0},
          });
  }
  SUBCASE("subtraction") {
    SUBCASE("normal") {
      CHECK((Mat{
                 {5.0, 6.0},
                 {7.0, 8.0},
             } -
             Mat{
                 {1.0, 2.0},
                 {3.0, 4.0},
             }) == //
            Mat{
                {4.0, 4.0},
                {4.0, 4.0},
            });
    }
    SUBCASE("with assignment") {
      Mat M{
          {5.0, 6.0},
          {7.0, 8.0},
      };
      M -= Mat{
          {1.0, 2.0},
          {3.0, 4.0},
      };
      CHECK(M == //
            Mat{
                {4.0, 4.0},
                {4.0, 4.0},
            });
    }
  }
}

TEST_CASE("Mat::operator*") {
  SUBCASE("scaling") {
    SUBCASE("normal") {
      CHECK((2.0 *
             Mat{
                 {1.0, 2.0},
                 {3.0, 4.0},
             }) == //
            Mat{
                {2.0, 4.0},
                {6.0, 8.0},
            });
      CHECK(Mat{
                {1.0, 2.0},
                {3.0, 4.0},
            } * 2.0 ==
            Mat{
                {2.0, 4.0},
                {6.0, 8.0},
            });
    }
    SUBCASE("with assignment") {
      Mat M{
          {1.0, 2.0},
          {3.0, 4.0},
      };
      M *= 2.0;
      CHECK(M == //
            Mat{
                {2.0, 4.0},
                {6.0, 8.0},
            });
    }
  }
  SUBCASE("matrix-vector multiplication") {
    CHECK((Mat{
               {1.0, 2.0},
               {3.0, 4.0},
           } *
           Vec{
               5.0,
               6.0,
           }) == //
          Vec{
              17.0,
              39.0,
          });
  }
  SUBCASE("matrix-matrix multiplication") {
    CHECK((Mat{
               {1.0, 2.0},
               {3.0, 4.0},
           } *
           Mat{
               {5.0, 6.0},
               {7.0, 8.0},
           }) == //
          Mat{
              {19.0, 22.0},
              {43.0, 50.0},
          });
  }
}

TEST_CASE("Mat::operator/") {
  SUBCASE("scaling") {
    SUBCASE("normal") {
      CHECK(Mat{
                {2.0, 4.0},
                {6.0, 8.0},
            } / 2.0 ==
            Mat{
                {1.0, 2.0},
                {3.0, 4.0},
            });
    }
    SUBCASE("with assignment") {
      Mat M{
          {2.0, 4.0},
          {6.0, 8.0},
      };
      M /= 2.0;
      CHECK(M == //
            Mat{
                {1.0, 2.0},
                {3.0, 4.0},
            });
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("Mat::tr") {
  CHECK(tr(Mat{
            {1.0, 2.0, 3.0},
            {4.0, 5.0, 6.0},
            {7.0, 8.0, 9.0},
        }) == 15.0);
}

TEST_CASE("Mat::prod_diag") {
  CHECK(prod_diag(Mat{
            {1.0, 2.0, 3.0},
            {4.0, 5.0, 6.0},
            {7.0, 8.0, 9.0},
        }) == 45.0);
}

TEST_CASE("Vec::outer") {
  CHECK(outer(
            Vec{
                1.0,
                2.0,
            },
            Vec{
                3.0,
                4.0,
            }) == //
        Mat{
            {3.0, 4.0},
            {6.0, 8.0},
        });
}

TEST_CASE("Vec::outer_sqr") {
  CHECK(outer_sqr( //
            Vec{
                1.0,
                2.0,
            }) == //
        Mat{
            {1.0, 2.0},
            {2.0, 4.0},
        });
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("Mat::operator==") {
  CHECK(Mat{
            {1.0, 2.0},
            {3.0, 4.0},
        } == //
        Mat{
            {1.0, 2.0},
            {3.0, 4.0},
        });
}

TEST_CASE("Mat::operator!=") {
  CHECK(Mat{
            {1.0, 2.0},
            {3.0, 4.0},
        } != //
        Mat{
            {1.0, 1.0},
            {3.0, 4.0},
        });
}

TEST_CASE("Mat::approx_equal_to") {
  CHECK(approx_equal_to(
      Mat{
          {1.0, 2.0},
          {3.0, 4.0},
      },
      Mat{
          {1.0, 2.0},
          {3.0, 4.0},
      }));
  CHECK_FALSE(approx_equal_to(
      Mat{
          {1.0, 2.0},
          {3.0, 4.0},
      },
      Mat{
          {1.0, 3.0},
          {3.0, 4.0},
      }));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("Mat::formatter") {
  CHECK(std::format("{}", Mat{{1, 2}, {3, 4}}) == "1 2 3 4");
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
