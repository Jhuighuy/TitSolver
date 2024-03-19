/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "tit/core/meta.hpp"

#include "tit/testing/test.hpp"

namespace tit {
namespace {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

// Some "meta types".
struct a_t {};
struct b_t {};
struct c_t {};
struct d_t {};
struct e_t {};
struct f_t {};
static_assert(meta::type<a_t>);

// This is not a "meta type", since it is not empty.
struct g_t {
  int data;
};
static_assert(!meta::type<g_t>);

// This is not a "meta type", since it is not trivial.
struct h_t {
  h_t() {
    CHECK(false);
  }
};
static_assert(!meta::type<h_t>);

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

static_assert(meta::contains_v<a_t, a_t, b_t, c_t>);
static_assert(!meta::contains_v<a_t, b_t, c_t, d_t>);

static_assert(meta::index_of_v<a_t, a_t, b_t, c_t> == 0);
static_assert(meta::index_of_v<b_t, a_t, b_t, c_t> == 1);
static_assert(meta::index_of_v<c_t, a_t, b_t, c_t> == 2);

static_assert(meta::all_unique_v<a_t, b_t, c_t>);
static_assert(meta::all_unique_v<a_t, b_t, c_t, d_t>);
static_assert(meta::all_unique_v<a_t, b_t, c_t, d_t, e_t>);
static_assert(!meta::all_unique_v<a_t, b_t, c_t, a_t>);
static_assert(!meta::all_unique_v<a_t, b_t, c_t, b_t>);
static_assert(!meta::all_unique_v<a_t, b_t, c_t, c_t>);
static_assert(!meta::all_unique_v<a_t, b_t, c_t, a_t, b_t>);

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

TEST_CASE("meta::Set") {
  constexpr meta::Set<a_t, b_t, c_t, d_t> s1{};
  constexpr meta::Set<a_t, b_t, d_t, e_t, c_t> s2{};
  STATIC_CHECK(s1.contains(c_t{}));
  STATIC_CHECK(s2.includes(s1));
  STATIC_CHECK_FALSE(s1.contains(e_t{}));
  STATIC_CHECK_FALSE(s1.includes(s2));
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

TEST_CASE("meta::Set::operator==") {
  constexpr meta::Set<a_t, b_t, c_t, d_t> s1{};
  constexpr meta::Set<b_t, c_t, d_t, a_t> s2{};
  constexpr meta::Set<a_t, b_t, d_t, e_t> s3{};
  STATIC_CHECK(s1 == s1);
  STATIC_CHECK(s1 == s2);
  STATIC_CHECK(s1 != s3);
  STATIC_CHECK_FALSE(s1 == s3);
}

TEST_CASE("meta::Set::operator<") {
  constexpr meta::Set<a_t, b_t, c_t, d_t> s1{};
  constexpr meta::Set<a_t, b_t, d_t, e_t, c_t> s2{};
  constexpr meta::Set<a_t, b_t, c_t> s3{};
  constexpr meta::Set<b_t, c_t> s4{};
  STATIC_CHECK(s1 < s2);
  STATIC_CHECK_FALSE(s1 < s1);
  STATIC_CHECK_FALSE(s1 < s3);
  STATIC_CHECK_FALSE(s1 < s4);
}

TEST_CASE("meta::Set::operator<=") {
  constexpr meta::Set<a_t, b_t, c_t, d_t> s1{};
  constexpr meta::Set<a_t, b_t, d_t, e_t, c_t> s2{};
  constexpr meta::Set<a_t, b_t, c_t> s3{};
  constexpr meta::Set<b_t, c_t> s4{};
  STATIC_CHECK(s1 <= s1);
  STATIC_CHECK(s1 <= s2);
  STATIC_CHECK_FALSE(s1 <= s3);
  STATIC_CHECK_FALSE(s1 <= s4);
}

TEST_CASE("meta::Set::operator>") {
  constexpr meta::Set<a_t, b_t, c_t, d_t> s1{};
  constexpr meta::Set<a_t, b_t, d_t, e_t, c_t> s2{};
  constexpr meta::Set<a_t, b_t, c_t> s3{};
  constexpr meta::Set<b_t, c_t> s4{};
  STATIC_CHECK(s2 > s1);
  STATIC_CHECK_FALSE(s1 > s1);
  STATIC_CHECK_FALSE(s3 > s1);
  STATIC_CHECK_FALSE(s4 > s1);
}

TEST_CASE("meta::Set::operator>=") {
  constexpr meta::Set<a_t, b_t, c_t, d_t> s1{};
  constexpr meta::Set<a_t, b_t, d_t, e_t, c_t> s2{};
  constexpr meta::Set<a_t, b_t, c_t> s3{};
  constexpr meta::Set<b_t, c_t> s4{};
  STATIC_CHECK(s1 >= s1);
  STATIC_CHECK(s2 >= s1);
  STATIC_CHECK_FALSE(s3 >= s1);
  STATIC_CHECK_FALSE(s4 >= s1);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

TEST_CASE("meta::Set::operator|") {
  constexpr meta::Set s0{};
  constexpr meta::Set<a_t, b_t, c_t> s1{};
  constexpr meta::Set<c_t, a_t, b_t> s2{};
  constexpr meta::Set<c_t, b_t, d_t, e_t> s3{};
  STATIC_CHECK((s1 | s0) == s1);
  STATIC_CHECK((s1 | s2) == s1);
  STATIC_CHECK((s1 | s3) == meta::Set<a_t, b_t, c_t, d_t, e_t>{});
}

TEST_CASE("meta::Set::operator&") {
  constexpr meta::Set s0{};
  constexpr meta::Set<a_t, b_t, c_t> s1{};
  constexpr meta::Set<c_t, a_t, b_t> s2{};
  constexpr meta::Set<c_t, d_t, e_t> s3{};
  constexpr meta::Set<d_t, e_t, f_t> s4{};
  STATIC_CHECK((s1 & s0) == s0);
  STATIC_CHECK((s1 & s2) == s1);
  STATIC_CHECK((s1 & s3) == meta::Set<c_t>{});
  STATIC_CHECK((s1 & s4) == s0);
}

TEST_CASE("meta::Set::operator-") {
  constexpr meta::Set s0{};
  constexpr meta::Set<a_t, b_t, c_t> s1{};
  constexpr meta::Set<c_t, a_t, b_t> s2{};
  constexpr meta::Set<c_t, b_t, d_t, e_t> s3{};
  STATIC_CHECK((s1 - s0) == s1);
  STATIC_CHECK((s1 - s2) == s0);
  STATIC_CHECK((s1 - s3) == meta::Set<a_t>{});
  STATIC_CHECK((s3 - s1) == meta::Set<d_t, e_t>{});
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace
} // namespace tit
