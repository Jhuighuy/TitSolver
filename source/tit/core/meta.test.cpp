/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "tit/core/basic_types.hpp"
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

TEST_CASE("meta::Set::apply") {
  constexpr meta::Set<A, B, C, D> s{};
  size_t count = 0;
  s.apply([&count](A /*a*/, B /*b*/, C /*c*/, D /*d*/) { count += 1; });
  CHECK(count == 1);
}

TEST_CASE("meta::Set::for_each") {
  constexpr meta::Set<A, B, C, D> s{};
  size_t count = 0;
  s.for_each([s, &count](auto elem) {
    count += 1;
    CHECK(s.contains(elem));
  });
  CHECK(count == 4);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("meta::Range::contains") {
  constexpr meta::Set<A, B, C, D> s{};
  STATIC_CHECK(s.contains(A{}));
  STATIC_CHECK(s.contains(B{}));
  STATIC_CHECK(s.contains(C{}));
  STATIC_CHECK(s.contains(D{}));
  STATIC_CHECK_FALSE(s.contains(E{}));
}

TEST_CASE("meta::Range::includes") {
  constexpr meta::Set<A, B, C, D> s{};
  STATIC_CHECK(s.includes(s));
  STATIC_CHECK(s.includes(meta::Set<A, B, C, D>{}));
  STATIC_CHECK(s.includes(meta::Set<A, B, C>{}));
  STATIC_CHECK_FALSE(s.includes(meta::Set<E, F>{}));
  STATIC_CHECK_FALSE(s.includes(meta::Set<A, F>{}));
}

TEST_CASE("meta::Range::find") {
  constexpr meta::Set<A, B, C, D> s{};
  STATIC_CHECK(s.find(A{}) == 0);
  STATIC_CHECK(s.find(B{}) == 1);
  STATIC_CHECK(s.find(C{}) == 2);
  STATIC_CHECK(s.find(D{}) == 3);
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
