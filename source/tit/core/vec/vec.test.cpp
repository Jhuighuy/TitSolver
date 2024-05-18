/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "tit/core/math.hpp"
#include "tit/core/vec.hpp"

#include "tit/testing/strict.hpp"
#include "tit/testing/test.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Use custom number type in this test to avoid vectorization.
using Num = Strict<double>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("Vec") {
  SUBCASE("zero initialization") {
    Vec<Num, 2> v{};
    CHECK(v[0] == 0.0_d);
    CHECK(v[1] == 0.0_d);
  }
  SUBCASE("zero assignment") {
    Vec v{1.0_d, 2.0_d};
    v = {};
    CHECK(v[0] == 0.0_d);
    CHECK(v[1] == 0.0_d);
  }
  SUBCASE("value initialization") {
    const Vec<Num, 2> v(3.0_d);
    CHECK(v[0] == 3.0_d);
    CHECK(v[1] == 3.0_d);
  }
  SUBCASE("aggregate initialization") {
    const Vec v{1.0_d, 2.0_d};
    CHECK(v[0] == 1.0_d);
    CHECK(v[1] == 2.0_d);
  }
  SUBCASE("aggregate assignment") {
    Vec<Num, 2> v;
    v = {3.0_d, 4.0_d};
    CHECK(v[0] == 3.0_d);
    CHECK(v[1] == 4.0_d);
  }
  SUBCASE("subscript") {
    Vec<Num, 2> v;
    v[0] = 3.0_d, v[1] = 4.0_d;
    CHECK(v[0] == 3.0_d);
    CHECK(v[1] == 4.0_d);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("static_vec_cast") {
  CHECK(all(static_vec_cast<int>(Vec{1.0_d, 2.0_d}) == Vec{1, 2}));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("Vec::operator+") {
  SUBCASE("normal") {
    CHECK(all(Vec{1.0_d, 2.0_d} + Vec{3.0_d, 4.0_d} == Vec{4.0_d, 6.0_d}));
  }
  SUBCASE("with assignment") {
    Vec v{1.0_d, 2.0_d};
    v += Vec{3.0_d, 4.0_d};
    CHECK(all(v == Vec{4.0_d, 6.0_d}));
  }
}

TEST_CASE("Vec::operator-") {
  SUBCASE("negation") {
    CHECK(all(-Vec{1.0_d, 2.0_d} == Vec{-1.0_d, -2.0_d}));
  }
  SUBCASE("subtraction") {
    SUBCASE("normal") {
      CHECK(all(Vec{3.0_d, 4.0_d} - Vec{1.0_d, 2.0_d} == Vec{2.0_d, 2.0_d}));
    }
    SUBCASE("with assignment") {
      Vec v{3.0_d, 4.0_d};
      v -= Vec{1.0_d, 2.0_d};
      CHECK(all(v == Vec{2.0_d, 2.0_d}));
    }
  }
}

TEST_CASE("Vec::operator*") {
  SUBCASE("scaling") {
    SUBCASE("normal") {
      CHECK(all(4.0_d * Vec{2.0_d, 3.0_d} == Vec{8.0_d, 12.0_d}));
      CHECK(all(Vec{2.0_d, 3.0_d} * 4.0_d == Vec{8.0_d, 12.0_d}));
    }
    SUBCASE("with assignment") {
      Vec v{2.0_d, 3.0_d};
      v *= 4.0_d;
      CHECK(all(v == Vec{8.0_d, 12.0_d}));
    }
  }
  SUBCASE("multiplication") {
    SUBCASE("normal") {
      CHECK(all(Vec{2.0_d, 3.0_d} * Vec{4.0_d, 5.0_d} == Vec{8.0_d, 15.0_d}));
    }
    SUBCASE("with assignment") {
      Vec v{2.0_d, 3.0_d};
      v *= Vec{4.0_d, 5.0_d};
      CHECK(all(v == Vec{8.0_d, 15.0_d}));
    }
  }
}

TEST_CASE("Vec::operator/") {
  SUBCASE("scaling") {
    SUBCASE("normal") {
      CHECK(all(Vec{8.0_d, 12.0_d} / 4.0_d == Vec{2.0_d, 3.0_d}));
    }
    SUBCASE("with assignment") {
      Vec v{8.0_d, 12.0_d};
      v /= 4.0_d;
      CHECK(all(v == Vec{2.0_d, 3.0_d}));
    }
  }
  SUBCASE("division") {
    SUBCASE("normal") {
      CHECK(all(Vec{8.0_d, 15.0_d} / Vec{2.0_d, 3.0_d} == Vec{4.0_d, 5.0_d}));
    }
    SUBCASE("with assignment") {
      Vec v{8.0_d, 15.0_d};
      v /= Vec{2.0_d, 3.0_d};
      CHECK(all(v == Vec{4.0_d, 5.0_d}));
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("Vec::abs") {
  CHECK(all(abs(Vec{-1.0_d, +2.0_d}) == Vec{1.0_d, 2.0_d}));
}

TEST_CASE("Vec::abs_delta") {
  CHECK(all(abs_delta(Vec{1.0_d, 4.0_d}, Vec{3.0_d, 2.0_d}) ==
            Vec{2.0_d, 2.0_d}));
}

TEST_CASE("Vec::minimum") {
  CHECK(all(minimum(Vec{-3.0_d, +4.0_d}, Vec{+3.0_d, +2.0_d}) ==
            Vec{-3.0_d, +2.0_d}));
}

TEST_CASE("Vec::maximum") {
  CHECK(all(maximum(Vec{-3.0_d, +4.0_d}, Vec{+3.0_d, +2.0_d}) ==
            Vec{+3.0_d, +4.0_d}));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("Vec::floor") {
  CHECK(all(floor(Vec{1.5_d, 2.7_d}) == Vec{1.0_d, 2.0_d}));
}

TEST_CASE("Vec::round") {
  CHECK(all(round(Vec{1.5_d, 2.7_d}) == Vec{2.0_d, 3.0_d}));
}

TEST_CASE("Vec::ceil") {
  CHECK(all(ceil(Vec{1.5_d, 2.7_d}) == Vec{2.0_d, 3.0_d}));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("Vec::sum") {
  CHECK(sum(Vec{1.0_d, 2.0_d}) == 3.0_d);
  CHECK(sum(Vec{3.0_d, 4.0_d, 5.0_d}) == 12.0_d);
}

TEST_CASE("Vec::dot") {
  CHECK(dot(Vec{1.0_d, 2.0_d}, Vec{3.0_d, 4.0_d}) == 11.0_d);
  CHECK(dot(Vec{1.0_d, 2.0_d, 3.0_d}, Vec{4.0_d, 5.0_d, 6.0_d}) == 32.0_d);
}

TEST_CASE("Vec::min_value") {
  CHECK(min_value(Vec{3.0_d, 2.0_d, 4.0_d}) == 2.0_d);
  CHECK(min_value(Vec{5.0_d, 4.0_d, 6.0_d, 3.0_d}) == 3.0_d);
}

TEST_CASE("Vec::max_value") {
  CHECK(max_value(Vec{3.0_d, 2.0_d, 4.0_d}) == 4.0_d);
  CHECK(max_value(Vec{5.0_d, 4.0_d, 6.0_d, 3.0_d}) == 6.0_d);
}

TEST_CASE("Vec::min_value_index") {
  CHECK(min_value_index(Vec{2.0_d, 3.0_d}) == 0);
  CHECK(min_value_index(Vec{3.0_d, 2.0_d, 4.0_d}) == 1);
  CHECK(min_value_index(Vec{5.0_d, 4.0_d, 6.0_d, 3.0_d}) == 3);
}

TEST_CASE("Vec::max_value_index") {
  CHECK(min_value_index(Vec{3.0_d, 2.0_d}) == 1);
  CHECK(max_value_index(Vec{3.0_d, 2.0_d, 4.0_d}) == 2);
  CHECK(max_value_index(Vec{5.0_d, 4.0_d, 6.0_d, 3.0_d}) == 2);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("Vec::norm2") {
  CHECK(norm2(Vec{3.0_d, 4.0_d}) == 25.0_d);
  CHECK(norm2(Vec{2.0_d, 10.0_d, 11.0_d}) == 225.0_d);
}

TEST_CASE("Vec::norm") {
  CHECK(norm(Vec{-3.0_d}) == 3.0_d);
  CHECK(approx_equal_to(norm(Vec{3.0_d, 4.0_d}), 5.0_d));
  CHECK(approx_equal_to(norm(Vec{2.0_d, 10.0_d, 11.0_d}), 15.0_d));
}

TEST_CASE("Vec::normalize") {
  CHECK(all(normalize(Vec{0.0_d}) == Vec{0.0_d}));
  CHECK(all(normalize(Vec{-3.0_d}) == Vec{-1.0_d}));
  CHECK(all(normalize(Vec{0.0_d, 0.0_d}) == Vec{0.0_d, 0.0_d}));
  CHECK(all(approx_equal_to(normalize(Vec{3.0_d, 4.0_d}), Vec{0.6_d, 0.8_d})));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("Vec::cross") {
  CHECK(all(cross(Vec{1.0_d, 0.0_d, 0.0_d}, Vec{0.0_d, 1.0_d, 0.0_d}) ==
            Vec{0.0_d, 0.0_d, 1.0_d}));
  CHECK(all(cross(Vec{1.0_d, 2.0_d, 3.0_d}, Vec{4.0_d, 5.0_d, 6.0_d}) ==
            Vec{-3.0_d, 6.0_d, -3.0_d}));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
