/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "tit/core/basic_types.hpp"
#include "tit/core/vec.hpp"

#include "tit/core/vec/vec.testing.hpp"
#include "tit/testing/random.hpp"
#include "tit/testing/test.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE_TEMPLATE("Vec<SIMD>", Vec, VEC_TYPES) {
  using Num = vec_num_t<Vec>;
  constexpr auto Dim = vec_dim_v<Vec>;
  SUBCASE("zero initialization") {
    Vec const v{};
    for (size_t i = 0; i < Dim; ++i) CHECK(v[i] == Num{0});
  }
  SUBCASE("value initialization") {
    auto const c = gen_val<Num>();
    Vec const v(c);
    for (size_t i = 0; i < Dim; ++i) CHECK(v[i] == c);
  }
  SUBCASE("aggregate initialization") {
    auto const a = gen_array<Num, Dim>();
    auto const v = to_vec(a);
    for (size_t i = 0; i < Dim; ++i) CHECK(v[i] == a[i]);
  }
  SUBCASE("aggregate assignment") {
    auto const a = gen_array<Num, Dim>();
    Vec v{};
    v = to_vec(a);
    for (size_t i = 0; i < Dim; ++i) CHECK(v[i] == a[i]);
  }
  SUBCASE("subscript") {
    auto const a = gen_array<Num, Dim>();
    Vec v{};
    for (size_t i = 0; i < Dim; ++i) v[i] = a[i];
    for (size_t i = 0; i < Dim; ++i) CHECK(v[i] == a[i]);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE_TEMPLATE("Vec<SIMD>::operator+", Vec, VEC_TYPES) {
  SUBCASE("unary plus") {
    auto const v = gen_val<Vec>();
    auto const v_ref = to_ref_vec(v);
    CHECK(equal_to_ref(+v, +v_ref));
  }
  SUBCASE("addition") {
    SUBCASE("normal") {
      auto const v1 = gen_val<Vec>();
      auto const v2 = gen_val<Vec>();
      auto const v1_ref = to_ref_vec(v1);
      auto const v2_ref = to_ref_vec(v2);
      CHECK(equal_to_ref(v1 + v2, v1_ref + v2_ref));
    }
    SUBCASE("with assignment") {
      auto /* */ v1 = gen_val<Vec>();
      auto const v2 = gen_val<Vec>();
      auto /* */ v1_ref = to_ref_vec(v1);
      auto const v2_ref = to_ref_vec(v2);
      CHECK(equal_to_ref(v1 += v2, v1_ref += v2_ref));
    }
  }
}

TEST_CASE_TEMPLATE("Vec<SIMD>::operator-", Vec, VEC_TYPES) {
  SUBCASE("negation") {
    auto const v = gen_val<Vec>();
    auto const v_ref = to_ref_vec(v);
    CHECK(equal_to_ref(-v, -v_ref));
  }
  SUBCASE("subtraction") {
    SUBCASE("normal") {
      auto const v1 = gen_val<Vec>();
      auto const v2 = gen_val<Vec>();
      auto const v1_ref = to_ref_vec(v1);
      auto const v2_ref = to_ref_vec(v2);
      CHECK(equal_to_ref(v1 - v2, v1_ref - v2_ref));
    }
    SUBCASE("with assignment") {
      auto /* */ v1 = gen_val<Vec>();
      auto const v2 = gen_val<Vec>();
      auto /* */ v1_ref = to_ref_vec(v1);
      auto const v2_ref = to_ref_vec(v2);
      CHECK(equal_to_ref(v1 -= v2, v1_ref -= v2_ref));
    }
  }
}

TEST_CASE_TEMPLATE("Vec<SIMD>::operator*", Vec, VEC_TYPES) {
  using Num = vec_num_t<Vec>;
  SUBCASE("scaling") {
    SUBCASE("normal") {
      auto const c = gen_val<Num>();
      auto const v = gen_val<Vec>();
      auto const c_ref = to_ref_val(c);
      auto const v_ref = to_ref_vec(v);
      CHECK(equal_to_ref(c * v, c_ref * v_ref));
      CHECK(equal_to_ref(v * c, v_ref * c_ref));
    }
    SUBCASE("with assignment") {
      auto const c = gen_val<Num>();
      auto /* */ v = gen_val<Vec>();
      auto const c_ref = to_ref_val(c);
      auto /* */ v_ref = to_ref_vec(v);
      CHECK(equal_to_ref(v *= c, v_ref *= c_ref));
    }
  }
  SUBCASE("multiplication") {
    SUBCASE("normal") {
      auto const v1 = gen_val<Vec>();
      auto const v2 = gen_val<Vec>();
      auto const v1_ref = to_ref_vec(v1);
      auto const v2_ref = to_ref_vec(v2);
      CHECK(equal_to_ref(v1 * v2, v1_ref * v2_ref));
    }
    SUBCASE("with assignment") {
      auto /* */ v1 = gen_val<Vec>();
      auto const v2 = gen_val<Vec>();
      auto /* */ v1_ref = to_ref_vec(v1);
      auto const v2_ref = to_ref_vec(v2);
      CHECK(equal_to_ref(v1 *= v2, v1_ref *= v2_ref));
    }
  }
}

TEST_CASE_TEMPLATE("Vec<SIMD>::operator/", Vec, VEC_TYPES) {
  using Num = vec_num_t<Vec>;
  SUBCASE("scaling") {
    SUBCASE("normal") {
      auto const c = gen_val<Num>();
      auto const v = gen_val<Vec>();
      auto const c_ref = to_ref_val(c);
      auto const v_ref = to_ref_vec(v);
      CHECK(approx_equal_to_ref(v / c, v_ref / c_ref));
    }
    SUBCASE("with assignment") {
      auto const c = gen_val<Num>();
      auto /* */ v = gen_val<Vec>();
      auto const c_ref = to_ref_val(c);
      auto /* */ v_ref = to_ref_vec(v);
      CHECK(approx_equal_to_ref(v /= c, v_ref /= c_ref));
    }
  }
  SUBCASE("division") {
    SUBCASE("normal") {
      auto const v1 = gen_val<Vec>();
      auto const v2 = gen_val<Vec>();
      auto const v1_ref = to_ref_vec(v1);
      auto const v2_ref = to_ref_vec(v2);
      CHECK(equal_to_ref(v1 / v2, v1_ref / v2_ref));
    }
    SUBCASE("with assignment") {
      auto /* */ v1 = gen_val<Vec>();
      auto const v2 = gen_val<Vec>();
      auto /* */ v1_ref = to_ref_vec(v1);
      auto const v2_ref = to_ref_vec(v2);
      CHECK(equal_to_ref(v1 /= v2, v1_ref /= v2_ref));
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE_TEMPLATE("Vec<SIMD>::abs", Vec, VEC_TYPES) {
  auto const v = gen_val<Vec>();
  auto const v_ref = to_ref_vec(v);
  CHECK(equal_to_ref(abs(v), abs(v_ref)));
}

TEST_CASE_TEMPLATE("Vec<SIMD>::abs_delta", Vec, VEC_TYPES) {
  auto const v1 = gen_val<Vec>();
  auto const v2 = gen_val<Vec>();
  auto const v1_ref = to_ref_vec(v1);
  auto const v2_ref = to_ref_vec(v2);
  CHECK(equal_to_ref(abs_delta(v1, v2), abs_delta(v1_ref, v2_ref)));
}

TEST_CASE_TEMPLATE("Vec<SIMD>::minimum", Vec, VEC_TYPES) {
  auto const v1 = gen_val<Vec>();
  auto const v2 = gen_val<Vec>();
  auto const v1_ref = to_ref_vec(v1);
  auto const v2_ref = to_ref_vec(v2);
  CHECK(equal_to_ref(minimum(v1, v2), minimum(v1_ref, v2_ref)));
}

TEST_CASE_TEMPLATE("Vec<SIMD>::maximum", Vec, VEC_TYPES) {
  auto const v1 = gen_val<Vec>();
  auto const v2 = gen_val<Vec>();
  auto const v1_ref = to_ref_vec(v1);
  auto const v2_ref = to_ref_vec(v2);
  CHECK(equal_to_ref(maximum(v1, v2), maximum(v1_ref, v2_ref)));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE_TEMPLATE("Vec<SIMD>::floor", Vec, VEC_TYPES) {
  auto const v = gen_val<Vec>();
  auto const v_ref = to_ref_vec(v);
  CHECK(equal_to_ref(floor(v), floor(v_ref)));
}

TEST_CASE_TEMPLATE("Vec<SIMD>::round", Vec, VEC_TYPES) {
  auto const v = gen_val<Vec>();
  auto const v_ref = to_ref_vec(v);
  CHECK(equal_to_ref(round(v), round(v_ref)));
}

TEST_CASE_TEMPLATE("Vec<SIMD>::ceil", Vec, VEC_TYPES) {
  auto const v = gen_val<Vec>();
  auto const v_ref = to_ref_vec(v);
  CHECK(equal_to_ref(ceil(v), ceil(v_ref)));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE_TEMPLATE("Vec<SIMD>::sum", Vec, ALL_VEC_TYPES) {
  auto const v = gen_val<Vec>();
  auto const v_ref = to_ref_vec(v);
  CHECK(approx_equal_to_ref(sum(v), sum(v_ref)));
}

TEST_CASE_TEMPLATE("Vec<SIMD>::dot", Vec, ALL_VEC_TYPES) {
  auto const v1 = gen_val<Vec>();
  auto const v2 = gen_val<Vec>();
  auto const v1_ref = to_ref_vec(v1);
  auto const v2_ref = to_ref_vec(v2);
  CHECK(approx_equal_to_ref(dot(v1, v2), dot(v1_ref, v2_ref)));
}

TEST_CASE_TEMPLATE("Vec<SIMD>::min_value", Vec, ALL_VEC_TYPES) {
  auto const v = gen_val<Vec>();
  auto const v_ref = to_ref_vec(v);
  CHECK(equal_to_ref(min_value(v), min_value(v_ref)));
}

TEST_CASE_TEMPLATE("Vec<SIMD>::max_value", Vec, ALL_VEC_TYPES) {
  auto const v = gen_val<Vec>();
  auto const v_ref = to_ref_vec(v);
  CHECK(equal_to_ref(max_value(v), max_value(v_ref)));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE_TEMPLATE("Vec<SIMD>::norm2", Vec, VEC_TYPES) {
  auto const v = gen_val<Vec>();
  auto const v_ref = to_ref_vec(v);
  CHECK(approx_equal_to_ref(norm2(v), norm2(v_ref)));
}

TEST_CASE_TEMPLATE("Vec<SIMD>::norm", Vec, VEC_TYPES) {
  auto const v = gen_val<Vec>();
  auto const v_ref = to_ref_vec(v);
  CHECK(approx_equal_to_ref(norm(v), norm(v_ref)));
}

TEST_CASE_TEMPLATE("Vec<SIMD>::normalize", Vec, VEC_TYPES) {
  auto const v = gen_val<Vec>();
  auto const v_ref = to_ref_vec(v);
  CHECK(approx_equal_to_ref(normalize(v), normalize(v_ref)));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
