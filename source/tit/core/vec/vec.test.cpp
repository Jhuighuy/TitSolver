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

// To test implementations with and without SIMD.
#define NUM_TYPES double, Strict<double>

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE_TEMPLATE("Vec", Num, NUM_TYPES) {
  SUBCASE("zero initialization") {
    Vec<Num, 2> v{};
    CHECK(v[0] == Num{0});
    CHECK(v[1] == Num{0});
  }
  SUBCASE("zero assignment") {
    Vec v{Num{1}, Num{2}};
    v = {};
    CHECK(v[0] == Num{0});
    CHECK(v[1] == Num{0});
  }
  SUBCASE("value initialization") {
    const Vec<Num, 2> v(Num{3});
    CHECK(v[0] == Num{3});
    CHECK(v[1] == Num{3});
  }
  SUBCASE("aggregate initialization") {
    const Vec v{Num{1}, Num{2}};
    CHECK(v[0] == Num{1});
    CHECK(v[1] == Num{2});
  }
  SUBCASE("aggregate assignment") {
    Vec<Num, 2> v;
    v = {Num{3}, Num{4}};
    CHECK(v[0] == Num{3});
    CHECK(v[1] == Num{4});
  }
  SUBCASE("subscript") {
    Vec<Num, 2> v;
    v[0] = Num{3}, v[1] = Num{4};
    CHECK(v[0] == Num{3});
    CHECK(v[1] == Num{4});
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Casts
//

TEST_CASE_TEMPLATE("Vec::unit", Num, NUM_TYPES) {
  CHECK(all(unit(Vec<Num, 2>{}) == Vec{Num{1}, Num{0}}));
  CHECK(all(unit<1>(Vec<Num, 2>{}) == Vec{Num{0}, Num{1}}));
}

TEST_CASE_TEMPLATE("Vec::vec_cat", Num, NUM_TYPES) {
  CHECK(all(vec_cat(Vec{Num{1}, Num{2}}, Vec{Num{3}, Num{4}}) ==
            Vec{Num{1}, Num{2}, Num{3}, Num{4}}));
}

TEST_CASE_TEMPLATE("Vec::vec_head", Num, NUM_TYPES) {
  CHECK(all(vec_head(Vec{Num{1}, Num{2}, Num{3}}) == Vec{Num{1}}));
  CHECK(all(vec_head<2>(Vec{Num{1}, Num{2}, Num{3}}) == Vec{Num{1}, Num{2}}));
}

TEST_CASE_TEMPLATE("Vec::vec_tail", Num, NUM_TYPES) {
  CHECK(all(vec_tail(Vec{Num{1}, Num{2}, Num{3}}) == Vec{Num{2}, Num{3}}));
  CHECK(all(vec_tail<2>(Vec{Num{1}, Num{2}, Num{3}}) == Vec{Num{3}}));
}

TEST_CASE_TEMPLATE("Vec::vec_cast<class To>", Num, NUM_TYPES) {
  SUBCASE("<class To>") {
    CHECK(all(vec_cast<int>(Vec{Num{1}, Num{2}}) == Vec{1, 2}));
  }
  SUBCASE("<template<class...> To>") {
    using Decuded = decltype(Strict{Num{}});
    const Vec<Decuded, 2> r = vec_cast<Strict>(Vec{Num{1}, Num{2}});
    CHECK(all(r == Vec{Decuded{1}, Decuded{2}}));
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Algorithms
//

TEST_CASE_TEMPLATE("Vec::minimum", Num, NUM_TYPES) {
  CHECK(all(minimum(Vec{-Num{3}, +Num{4}},    //
                    Vec{+Num{3}, +Num{2}}) == //
            Vec{-Num{3}, +Num{2}}));
}

TEST_CASE_TEMPLATE("Vec::maximum", Num, NUM_TYPES) {
  CHECK(all(maximum(Vec{-Num{3}, +Num{4}},    //
                    Vec{+Num{3}, +Num{2}}) == //
            Vec{+Num{3}, +Num{4}}));
}

TEST_CASE_TEMPLATE("VecMask::filter", Num, NUM_TYPES) {
  const auto m = Vec{Num{1}, Num{2}} == Vec{Num{3}, Num{2}};
  CHECK(all(filter(m, Vec{Num{1}, Num{2}}) == Vec{Num{0}, Num{2}}));
}

TEST_CASE_TEMPLATE("VecMask::select", Num, NUM_TYPES) {
  const auto m = Vec{Num{1}, Num{2}} == Vec{Num{3}, Num{2}};
  CHECK(all(select(m, Vec{Num{1}, Num{2}}, Vec{Num{3}, Num{4}}) ==
            Vec{Num{3}, Num{2}}));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Arithmetic operations
//

TEST_CASE_TEMPLATE("Vec::operator+", Num, NUM_TYPES) {
  SUBCASE("normal") {
    CHECK(all((Vec{Num{1}, Num{2}} +   //
               Vec{Num{3}, Num{4}}) == //
              Vec{Num{4}, Num{6}}));
  }
  SUBCASE("with assignment") {
    Vec v{Num{1}, Num{2}};
    v += Vec{Num{3}, Num{4}};
    CHECK(all(v == Vec{Num{4}, Num{6}}));
  }
}

TEST_CASE_TEMPLATE("Vec::operator-", Num, NUM_TYPES) {
  SUBCASE("negation") {
    CHECK(all(-Vec{Num{1}, Num{2}} == Vec{-Num{1}, -Num{2}}));
  }
  SUBCASE("subtraction") {
    SUBCASE("normal") {
      CHECK(all((Vec{Num{3}, Num{4}} -   //
                 Vec{Num{1}, Num{2}}) == //
                Vec{Num{2}, Num{2}}));
    }
    SUBCASE("with assignment") {
      Vec v{Num{3}, Num{4}};
      v -= Vec{Num{1}, Num{2}};
      CHECK(all(v == Vec{Num{2}, Num{2}}));
    }
  }
}

TEST_CASE_TEMPLATE("Vec::operator*", Num, NUM_TYPES) {
  SUBCASE("scaling") {
    SUBCASE("normal") {
      CHECK(all(Num{4} * Vec{Num{2}, Num{3}} == Vec{Num{8}, Num{12}}));
      CHECK(all(Vec{Num{2}, Num{3}} * Num{4} == Vec{Num{8}, Num{12}}));
    }
    SUBCASE("with assignment") {
      Vec v{Num{2}, Num{3}};
      v *= Num{4};
      CHECK(all(v == Vec{Num{8}, Num{12}}));
    }
  }
  SUBCASE("multiplication") {
    SUBCASE("normal") {
      CHECK(all((Vec{Num{2}, Num{3}} *   //
                 Vec{Num{4}, Num{5}}) == //
                Vec{Num{8}, Num{15}}));
    }
    SUBCASE("with assignment") {
      Vec v{Num{2}, Num{3}};
      v *= Vec{Num{4}, Num{5}};
      CHECK(all(v == Vec{Num{8}, Num{15}}));
    }
  }
}

TEST_CASE_TEMPLATE("Vec::operator/", Num, NUM_TYPES) {
  SUBCASE("scaling") {
    SUBCASE("normal") {
      CHECK(all(Vec{Num{8}, Num{12}} / Num{4} == Vec{Num{2}, Num{3}}));
    }
    SUBCASE("with assignment") {
      Vec v{Num{8}, Num{12}};
      v /= Num{4};
      CHECK(all(v == Vec{Num{2}, Num{3}}));
    }
  }
  SUBCASE("division") {
    SUBCASE("normal") {
      CHECK(all((Vec{Num{8}, Num{15}} /  //
                 Vec{Num{2}, Num{3}}) == //
                Vec{Num{4}, Num{5}}));
    }
    SUBCASE("with assignment") {
      Vec v{Num{8}, Num{15}};
      v /= Vec{Num{2}, Num{3}};
      CHECK(all(v == Vec{Num{4}, Num{5}}));
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Mathematical functions
//

TEST_CASE_TEMPLATE("Vec::floor", Num, NUM_TYPES) {
  CHECK(all(floor(Vec{Num{1.5}, Num{2.7}}) == Vec{Num{1}, Num{2}}));
}

TEST_CASE_TEMPLATE("Vec::round", Num, NUM_TYPES) {
  CHECK(all(round(Vec{Num{1.5}, Num{2.7}}) == Vec{Num{2}, Num{3}}));
}

TEST_CASE_TEMPLATE("Vec::ceil", Num, NUM_TYPES) {
  CHECK(all(ceil(Vec{Num{1.5}, Num{2.7}}) == Vec{Num{2}, Num{3}}));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Comparison operations
//

TEST_CASE_TEMPLATE("Vec::operator==", Num, NUM_TYPES) {
  CHECK(all((Vec{Num{1}, Num{2}} ==  //
             Vec{Num{1}, Num{3}}) == //
            VecMask<Num, 2>{true, false}));
}

TEST_CASE_TEMPLATE("Vec::operator!=", Num, NUM_TYPES) {
  CHECK(all((Vec{Num{1}, Num{2}} !=  //
             Vec{Num{1}, Num{3}}) == //
            VecMask<Num, 2>{false, true}));
}

TEST_CASE_TEMPLATE("Vec::operator<", Num, NUM_TYPES) {
  CHECK(all((Vec{Num{1}, Num{2}, Num{3}} <   //
             Vec{Num{1}, Num{2}, Num{4}}) == //
            VecMask<Num, 3>{false, false, true}));
}

TEST_CASE_TEMPLATE("Vec::operator<=", Num, NUM_TYPES) {
  CHECK(all((Vec{Num{1}, Num{2}, Num{4}} <=  //
             Vec{Num{1}, Num{2}, Num{3}}) == //
            VecMask<Num, 3>{true, true, false}));
}

TEST_CASE_TEMPLATE("Vec::operator>", Num, NUM_TYPES) {
  CHECK(all((Vec{Num{1}, Num{2}, Num{4}} >   //
             Vec{Num{1}, Num{2}, Num{3}}) == //
            VecMask<Num, 3>{false, false, true}));
}

TEST_CASE_TEMPLATE("Vec::operator>=", Num, NUM_TYPES) {
  CHECK(all((Vec{Num{1}, Num{2}, Num{3}} >=  //
             Vec{Num{1}, Num{2}, Num{4}}) == //
            VecMask<Num, 3>{true, true, false}));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Reductions
//

TEST_CASE_TEMPLATE("Vec::sum", Num, NUM_TYPES) {
  CHECK(sum(Vec{Num{1}, Num{2}}) == Num{3});
  CHECK(sum(Vec{Num{1}, Num{2}, Num{3}}) == Num{6});
  CHECK(sum(Vec{Num{1}, Num{2}, Num{3}, Num{4}}) == Num{10});
  CHECK(sum(Vec{Num{1}, Num{2}, Num{3}, Num{4}, Num{5}}) == Num{15});
  // To cover all the SIMD paths, 17 element vectors are needed.
  CHECK(sum(Vec<Num, 17>(Num{16})) == Num{17 * 16});
}

TEST_CASE_TEMPLATE("Vec::prod", Num, NUM_TYPES) {
  CHECK(prod(Vec{Num{1}, Num{2}}) == Num{2});
  CHECK(prod(Vec{Num{1}, Num{2}, Num{3}}) == Num{6});
}

TEST_CASE_TEMPLATE("Vec::min_value", Num, NUM_TYPES) {
  CHECK(min_value(Vec{Num{3}, Num{2}, Num{4}}) == Num{2});
  CHECK(min_value(Vec{Num{5}, Num{4}, Num{6}, Num{3}}) == Num{3});
  CHECK(min_value(Vec{Num{5}, Num{4}, Num{6}, Num{2}, Num{3}}) == Num{2});
  // To cover all the SIMD paths, 17 element vectors are needed.
  Vec<Num, 17> v(Num{16});
  v[8] = Num{1};
  CHECK(min_value(v) == Num{1});
}

TEST_CASE_TEMPLATE("Vec::max_value", Num, NUM_TYPES) {
  CHECK(max_value(Vec{Num{3}, Num{2}, Num{4}}) == Num{4});
  CHECK(max_value(Vec{Num{5}, Num{4}, Num{6}, Num{3}}) == Num{6});
  CHECK(max_value(Vec{Num{5}, Num{4}, Num{6}, Num{2}, Num{3}}) == Num{6});
  // To cover all the SIMD paths, 17 element vectors are needed.
  Vec<Num, 17> v(Num{16});
  v[8] = Num{17};
  CHECK(max_value(v) == Num{17});
}

TEST_CASE_TEMPLATE("Vec::min_value_index", Num, NUM_TYPES) {
  CHECK(min_value_index(Vec{Num{2}, Num{3}}) == 0);
  CHECK(min_value_index(Vec{Num{3}, Num{2}, Num{4}}) == 1);
  CHECK(min_value_index(Vec{Num{5}, Num{4}, Num{6}, Num{3}}) == 3);
}

TEST_CASE_TEMPLATE("Vec::max_value_index", Num, NUM_TYPES) {
  CHECK(min_value_index(Vec{Num{3}, Num{2}}) == 1);
  CHECK(max_value_index(Vec{Num{3}, Num{2}, Num{4}}) == 2);
  CHECK(max_value_index(Vec{Num{5}, Num{4}, Num{6}, Num{3}}) == 2);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Linear algebra
//

TEST_CASE_TEMPLATE("Vec::dot", Num, NUM_TYPES) {
  CHECK(dot(Vec{Num{1}, Num{2}}, //
            Vec{Num{3}, Num{4}}) == Num{11});
  CHECK(dot(Vec{Num{1}, Num{2}, Num{3}}, //
            Vec{Num{4}, Num{5}, Num{6}}) == Num{32});
  CHECK(dot(Vec{Num{1}, Num{2}, Num{3}, Num{4}},
            Vec{Num{5}, Num{6}, Num{7}, Num{8}}) == Num{70});
  CHECK(dot(Vec{Num{1}, Num{2}, Num{3}, Num{4}, Num{5}},
            Vec{Num{6}, Num{7}, Num{8}, Num{9}, Num{10}}) == Num{130});
  // To cover all the SIMD paths, 17 element vectors are needed.
  CHECK(dot(Vec<Num, 17>(Num{3}), Vec<Num, 17>(Num{4})) == Num{17 * 3 * 4});
}

TEST_CASE_TEMPLATE("Vec::norm2", Num, NUM_TYPES) {
  CHECK(norm2(Vec{Num{3}, Num{4}}) == Num{25});
  CHECK(norm2(Vec{Num{2}, Num{10}, Num{11}}) == Num{225});
}

TEST_CASE_TEMPLATE("Vec::norm", Num, NUM_TYPES) {
  CHECK(norm(Vec{-Num{3}}) == Num{3});
  CHECK(approx_equal_to(norm(Vec{Num{3}, Num{4}}), Num{5}));
  CHECK(approx_equal_to(norm(Vec{Num{2}, Num{10}, Num{11}}), Num{15}));
}

TEST_CASE_TEMPLATE("Vec::normalize", Num, NUM_TYPES) {
  CHECK(all(normalize(Vec{Num{0}}) == Vec{Num{0}}));
  CHECK(all(normalize(Vec{-Num{3}}) == Vec{-Num{1}}));
  CHECK(all(normalize(Vec{Num{0}, Num{0}}) == Vec{Num{0}, Num{0}}));
  CHECK(
      approx_equal_to(normalize(Vec{Num{3}, Num{4}}), Vec{Num{0.6}, Num{0.8}}));
}

TEST_CASE_TEMPLATE("Vec::approx_equal_to", Num, NUM_TYPES) {
  CHECK(approx_equal_to(Vec{Num{1}, Num{2}}, Vec{Num{1}, Num{2}}));
  CHECK_FALSE(approx_equal_to(Vec{Num{1}, Num{2}}, Vec{Num{1}, Num{3}}));
}

TEST_CASE_TEMPLATE("Vec::cross", Num, NUM_TYPES) {
  CHECK(all(cross(Vec{Num{1}, Num{0}, Num{0}},    //
                  Vec{Num{0}, Num{1}, Num{0}}) == //
            Vec{Num{0}, Num{0}, Num{1}}));
  CHECK(all(cross(Vec{Num{1}, Num{2}, Num{3}},    //
                  Vec{Num{4}, Num{5}, Num{6}}) == //
            Vec{-Num{3}, Num{6}, -Num{3}}));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
