/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "tit/core/meta.hpp"

#include "tit/testing/test.hpp"

namespace tit {
namespace {

struct A {};
struct B {};
struct C {};
struct D {};
struct E {};
struct F {};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("meta::Set") {
  constexpr meta::Set<A, B, C, D> s1{};
  constexpr meta::Set<A, B, D, E, C> s2{};
  STATIC_CHECK(s1.contains(C{}));
  STATIC_CHECK(s2.includes(s1));
  STATIC_CHECK_FALSE(s1.contains(E{}));
  STATIC_CHECK_FALSE(s1.includes(s2));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("meta::Set::contains") {
  constexpr meta::Set<A, B, C, D> s1{};
  STATIC_CHECK(s1.contains(A{}));
  STATIC_CHECK(s1.contains(B{}));
  STATIC_CHECK(s1.contains(C{}));
  STATIC_CHECK(s1.contains(D{}));
  STATIC_CHECK_FALSE(s1.contains(E{}));
}

TEST_CASE("meta::Set::includes") {
  constexpr meta::Set<A, B, C, D> s1{};
  STATIC_CHECK(s1.includes(s1));
  STATIC_CHECK(s1.includes(meta::Set<A, B, C, D>{}));
  STATIC_CHECK(s1.includes(meta::Set<A, B, C>{}));
  STATIC_CHECK_FALSE(s1.includes(meta::Set<E, F>{}));
  STATIC_CHECK_FALSE(s1.includes(meta::Set<A, F>{}));
}

TEST_CASE("meta::Set::find") {
  constexpr meta::Set<A, B, C, D> s1{};
  STATIC_CHECK(s1.find(A{}) == 0);
  STATIC_CHECK(s1.find(B{}) == 1);
  STATIC_CHECK(s1.find(C{}) == 2);
  STATIC_CHECK(s1.find(D{}) == 3);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("meta::Set::operator==") {
  constexpr meta::Set<A, B, C, D> s1{};
  constexpr meta::Set<B, C, D, A> s2{};
  constexpr meta::Set<A, B, D, E> s3{};
  STATIC_CHECK(s1 == s1);
  STATIC_CHECK(s1 == s2);
  STATIC_CHECK(s1 != s3);
  STATIC_CHECK_FALSE(s1 == s3);
}

TEST_CASE("meta::Set::operator<") {
  constexpr meta::Set<A, B, C, D> s1{};
  constexpr meta::Set<A, B, D, E, C> s2{};
  constexpr meta::Set<A, B, C> s3{};
  constexpr meta::Set<B, C> s4{};
  STATIC_CHECK(s1 < s2);
  STATIC_CHECK_FALSE(s1 < s1);
  STATIC_CHECK_FALSE(s1 < s3);
  STATIC_CHECK_FALSE(s1 < s4);
}

TEST_CASE("meta::Set::operator<=") {
  constexpr meta::Set<A, B, C, D> s1{};
  constexpr meta::Set<A, B, D, E, C> s2{};
  constexpr meta::Set<A, B, C> s3{};
  constexpr meta::Set<B, C> s4{};
  STATIC_CHECK(s1 <= s1);
  STATIC_CHECK(s1 <= s2);
  STATIC_CHECK_FALSE(s1 <= s3);
  STATIC_CHECK_FALSE(s1 <= s4);
}

TEST_CASE("meta::Set::operator>") {
  constexpr meta::Set<A, B, C, D> s1{};
  constexpr meta::Set<A, B, D, E, C> s2{};
  constexpr meta::Set<A, B, C> s3{};
  constexpr meta::Set<B, C> s4{};
  STATIC_CHECK(s2 > s1);
  STATIC_CHECK_FALSE(s1 > s1);
  STATIC_CHECK_FALSE(s3 > s1);
  STATIC_CHECK_FALSE(s4 > s1);
}

TEST_CASE("meta::Set::operator>=") {
  constexpr meta::Set<A, B, C, D> s1{};
  constexpr meta::Set<A, B, D, E, C> s2{};
  constexpr meta::Set<A, B, C> s3{};
  constexpr meta::Set<B, C> s4{};
  STATIC_CHECK(s1 >= s1);
  STATIC_CHECK(s2 >= s1);
  STATIC_CHECK_FALSE(s3 >= s1);
  STATIC_CHECK_FALSE(s4 >= s1);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("meta::Set::operator|") {
  constexpr meta::Set s0{};
  constexpr meta::Set<A, B, C> s1{};
  constexpr meta::Set<C, A, B> s2{};
  constexpr meta::Set<C, B, D, E> s3{};
  STATIC_CHECK((s1 | s0) == s1);
  STATIC_CHECK((s1 | s2) == s1);
  STATIC_CHECK((s1 | s3) == meta::Set<A, B, C, D, E>{});
}

TEST_CASE("meta::Set::operator&") {
  constexpr meta::Set s0{};
  constexpr meta::Set<A, B, C> s1{};
  constexpr meta::Set<C, A, B> s2{};
  constexpr meta::Set<C, D, E> s3{};
  constexpr meta::Set<D, E, F> s4{};
  STATIC_CHECK((s1 & s0) == s0);
  STATIC_CHECK((s1 & s2) == s1);
  STATIC_CHECK((s1 & s3) == meta::Set<C>{});
  STATIC_CHECK((s1 & s4) == s0);
}

TEST_CASE("meta::Set::operator-") {
  constexpr meta::Set s0{};
  constexpr meta::Set<A, B, C> s1{};
  constexpr meta::Set<C, A, B> s2{};
  constexpr meta::Set<C, B, D, E> s3{};
  STATIC_CHECK((s1 - s0) == s1);
  STATIC_CHECK((s1 - s2) == s0);
  STATIC_CHECK((s1 - s3) == meta::Set<A>{});
  STATIC_CHECK((s3 - s1) == meta::Set<D, E>{});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
