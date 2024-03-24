/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "tit/core/basic_types.hpp"
#include "tit/core/vec.hpp"

#include "tit/core/vec/vec.testing.hpp"
#include "tit/testing/test.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE_TEMPLATE("VecMask<SIMD>", Vec, VEC_TYPES) {
  using Num = vec_num_t<Vec>;
  constexpr auto Dim = vec_dim_v<Vec>;
  SUBCASE("false initialization") {
    VecMask<Num, Dim> const v{};
    for (size_t i = 0; i < Dim; ++i) CHECK_FALSE(v[i]);
  }
  SUBCASE("true initialization") {
    Vec const m(true);
    for (size_t i = 0; i < Dim; ++i) CHECK(m[i]);
  }
  SUBCASE("aggregate initialization") {
    auto const a = gen_array<bool, Dim>();
    auto const m = to_vec_mask<Num>(a);
    for (size_t i = 0; i < Dim; ++i) CHECK(m[i] == a[i]);
  }
  SUBCASE("aggregate assignment") {
    auto const a = gen_array<bool, Dim>();
    VecMask<Num, Dim> m{};
    m = to_vec_mask<Num>(a);
    for (size_t i = 0; i < Dim; ++i) CHECK(m[i] == a[i]);
  }
  SUBCASE("subscript") {
    auto const a = gen_array<bool, Dim>();
    VecMask<Num, Dim> m{};
    for (size_t i = 0; i < Dim; ++i) m[i] = a[i];
    for (size_t i = 0; i < Dim; ++i) CHECK(m[i] == a[i]);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE_TEMPLATE("Vec<SIMD>::operator==", Vec, VEC_TYPES) {
  auto const v1 = gen_val<Vec>();
  auto const v2 = gen_val<Vec>();
  auto const v1_ref = to_ref_vec(v1);
  auto const v2_ref = to_ref_vec(v2);
  CHECK(equal_to_ref(v1 == v2, v1_ref == v2_ref));
}

TEST_CASE_TEMPLATE("Vec<SIMD>::operator!=", Vec, VEC_TYPES) {
  auto const v1 = gen_val<Vec>();
  auto const v2 = gen_val<Vec>();
  auto const v1_ref = to_ref_vec(v1);
  auto const v2_ref = to_ref_vec(v2);
  CHECK(equal_to_ref(v1 != v2, v1_ref != v2_ref));
}

TEST_CASE_TEMPLATE("Vec<SIMD>::operator<", Vec, VEC_TYPES) {
  auto const v1 = gen_val<Vec>();
  auto const v2 = gen_val<Vec>();
  auto const v1_ref = to_ref_vec(v1);
  auto const v2_ref = to_ref_vec(v2);
  CHECK(equal_to_ref(v1 < v2, v1_ref < v2_ref));
}

TEST_CASE_TEMPLATE("Vec<SIMD>::operator<=", Vec, VEC_TYPES) {
  auto const v1 = gen_val<Vec>();
  auto const v2 = gen_val<Vec>();
  auto const v1_ref = to_ref_vec(v1);
  auto const v2_ref = to_ref_vec(v2);
  CHECK(equal_to_ref(v1 <= v2, v1_ref <= v2_ref));
}

TEST_CASE_TEMPLATE("Vec<SIMD>::operator>", Vec, VEC_TYPES) {
  auto const v1 = gen_val<Vec>();
  auto const v2 = gen_val<Vec>();
  auto const v1_ref = to_ref_vec(v1);
  auto const v2_ref = to_ref_vec(v2);
  CHECK(equal_to_ref(v1 > v2, v1_ref > v2_ref));
}

TEST_CASE_TEMPLATE("Vec<SIMD>::operator>=", Vec, VEC_TYPES) {
  auto const v1 = gen_val<Vec>();
  auto const v2 = gen_val<Vec>();
  auto const v1_ref = to_ref_vec(v1);
  auto const v2_ref = to_ref_vec(v2);
  CHECK(equal_to_ref(v1 >= v2, v1_ref >= v2_ref));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE_TEMPLATE("VecMask<SIMD>::blend_zero", Vec, VEC_TYPES) {
  auto const v1 = gen_val<Vec>();
  auto const v2 = gen_val<Vec>();
  auto const v1_ref = to_ref_vec(v1);
  auto const v2_ref = to_ref_vec(v2);
  CHECK(equal_to_ref(blend_zero(v1 > v2, v1 - v2),
                     blend_zero(v1_ref > v2_ref, v1_ref - v2_ref)));
}

TEST_CASE_TEMPLATE("VecMask<SIMD>::blend", Vec, VEC_TYPES) {
  auto const v1 = gen_val<Vec>();
  auto const v2 = gen_val<Vec>();
  auto const v1_ref = to_ref_vec(v1);
  auto const v2_ref = to_ref_vec(v2);
  CHECK(equal_to_ref(blend(v1 > v2, v1 - v2, v1 + v2),
                     blend(v1_ref > v2_ref, v1_ref - v2_ref, v1_ref + v2_ref)));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
