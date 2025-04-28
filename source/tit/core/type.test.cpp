/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <string_view>
#include <type_traits>
#include <vector>

#include "tit/core/type.hpp"

#include "tit/testing/test.hpp"

template<class>
struct Template1 {};
template<class>
struct Template2 {};

struct A {};
struct B {};
struct C {};
struct D {};
struct E {};
struct F {};

namespace test {
struct Base {};
struct Derived : Base {};
struct PolymorphicBase : tit::VirtualBase {};
struct PolymorphicDerived : PolymorphicBase {};
} // namespace test

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

static_assert(specialization_of<Template1<int>, Template1>);
static_assert(!specialization_of<Template1<int>, Template2>);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

static_assert(std::is_same_v<composition_t<int,
                                           std::make_unsigned,
                                           std::add_const,
                                           std::add_pointer>,
                             const unsigned*>);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

static_assert(
    sizeof(long) == sizeof(long long) && !std::is_same_v<long, long long> &&
    std::is_same_v<normalize_type_t<long>, normalize_type_t<long long>>);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

static_assert(contains_v<A, A, B, C>);
static_assert(!contains_v<A, B, C, D>);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

static_assert(index_of_v<A, A, B, C> == 0);
static_assert(index_of_v<B, A, B, C> == 1);
static_assert(index_of_v<C, A, B, C> == 2);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

static_assert(all_unique_v<A, B, C>);
static_assert(all_unique_v<A, B, C, D>);
static_assert(all_unique_v<A, B, C, D, E>);
static_assert(!all_unique_v<A, B, C, A>);
static_assert(!all_unique_v<A, B, C, B>);
static_assert(!all_unique_v<A, B, C, C>);
static_assert(!all_unique_v<A, B, C, A, B>);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("TypeSet") {
  SUBCASE("methods") {
    constexpr TypeSet<A, B, C, D> s{};
    SUBCASE("for_each") {
      std::vector<std::string_view> names;
      s.for_each([&names]<class T>(T /*item*/) {
        names.push_back(type_name_of<T>());
      });
      CHECK_RANGE_EQ(names, {"A", "B", "C", "D"});
    }
    SUBCASE("contains") {
      STATIC_CHECK(s.contains(A{}));
      STATIC_CHECK(s.contains(B{}));
      STATIC_CHECK(s.contains(C{}));
      STATIC_CHECK(s.contains(D{}));
      STATIC_CHECK_FALSE(s.contains(E{}));
    }
    SUBCASE("find") {
      STATIC_CHECK(s.find(A{}) == 0);
      STATIC_CHECK(s.find(B{}) == 1);
      STATIC_CHECK(s.find(C{}) == 2);
      STATIC_CHECK(s.find(D{}) == 3);
    }
  }
  SUBCASE("comparison") {
    constexpr TypeSet<A, B, C, D> s1{};
    constexpr TypeSet<A, B, D, E, C> s2{};
    constexpr TypeSet<A, B, C> s3{};
    constexpr TypeSet<B, C> s4{};
    SUBCASE("operator==") {
      STATIC_CHECK(s1 == s1);
      STATIC_CHECK(s1 == TypeSet<A, C, D, B>{});
      STATIC_CHECK(s1 != s3);
      STATIC_CHECK_FALSE(s1 == s3);
    }
    SUBCASE("operator<") {
      STATIC_CHECK(s1 < s2);
      STATIC_CHECK_FALSE(s1 < s1);
      STATIC_CHECK_FALSE(s1 < s3);
      STATIC_CHECK_FALSE(s1 < s4);
    }
    SUBCASE("operator<=") {
      STATIC_CHECK(s1 <= s1);
      STATIC_CHECK(s1 <= s2);
      STATIC_CHECK_FALSE(s1 <= s3);
      STATIC_CHECK_FALSE(s1 <= s4);
    }
    SUBCASE("operator>") {
      STATIC_CHECK(s2 > s1);
      STATIC_CHECK_FALSE(s1 > s1);
      STATIC_CHECK_FALSE(s3 > s1);
      STATIC_CHECK_FALSE(s4 > s1);
    }
    SUBCASE("operator>=") {
      STATIC_CHECK(s1 >= s1);
      STATIC_CHECK(s2 >= s1);
      STATIC_CHECK_FALSE(s3 >= s1);
      STATIC_CHECK_FALSE(s4 >= s1);
    }
  }
  SUBCASE("operations") {
    constexpr TypeSet s0{};
    constexpr TypeSet<A, B, C> s1{};
    constexpr TypeSet<C, A, B> s2{};
    constexpr TypeSet<C, B, D, E> s3{};
    constexpr TypeSet<D, E, F> s4{};
    SUBCASE("union") {
      STATIC_CHECK((s1 | s0) == s1);
      STATIC_CHECK((s1 | s2) == s1);
      STATIC_CHECK((s1 | s3) == TypeSet<A, B, C, D, E>{});
    }
    SUBCASE("intersection") {
      STATIC_CHECK((s1 & s0) == s0);
      STATIC_CHECK((s1 & s2) == s1);
      STATIC_CHECK((s1 & s3) == TypeSet<B, C>{});
      STATIC_CHECK((s1 & s4) == s0);
    }
    SUBCASE("difference") {
      STATIC_CHECK((s1 - s0) == s1);
      STATIC_CHECK((s1 - s2) == s0);
      STATIC_CHECK((s1 - s3) == TypeSet<A>{});
      STATIC_CHECK((s3 - s1) == TypeSet<D, E>{});
      STATIC_CHECK((s1 - s4) == s1);
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("type_name_of") {
  SUBCASE("type parameter") {
    STATIC_CHECK(type_name_of<void>() == "void");
    STATIC_CHECK(type_name_of<int>() == "int");
    STATIC_CHECK(type_name_of<float>() == "float");
    STATIC_CHECK(type_name_of<test::Base>() == "test::Base");
    STATIC_CHECK(type_name_of<test::Derived>() == "test::Derived");
  }
  SUBCASE("argument parameter") {
    SUBCASE("non-polymorphic") {
      test::Derived obj;
      CHECK(type_name_of(obj) == "test::Derived");
      CHECK(type_name_of(static_cast<test::Base&>(obj)) == "test::Base");
    }
    SUBCASE("polymorphic") {
      test::PolymorphicDerived obj;
      CHECK(type_name_of(obj) == "test::PolymorphicDerived");
      CHECK(type_name_of(static_cast<test::PolymorphicBase&>(obj)) ==
            "test::PolymorphicDerived");
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
