/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <functional>

#include "tit/core/meta.hpp"

#include "tit/testing/func_utils.hpp"
#include "tit/testing/test.hpp"

namespace tit {
namespace {

struct A {};
struct B {};
struct C {};
struct D {};
struct E {};
struct F {};

template<meta::type... Ts>
class TestRange final : public meta::Range<Ts...> {
public:

  consteval TestRange() noexcept = default;
  consteval explicit TestRange(Ts... /*elems*/)
    requires (sizeof...(Ts) != 0)
  {}

}; // class TestRange

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("meta::Range::apply") {
  constexpr TestRange<A, B, C, D> r{};
  CountedFunc counted_callback{[](A /*a*/, B /*b*/, C /*c*/, D /*d*/) {}};
  r.apply(std::ref(counted_callback));
  CHECK(counted_callback.count() == 1);
}

TEST_CASE("meta::Range::for_each") {
  constexpr TestRange<A, B, C, D> r{};
  CountedFunc counted_callback{[r](auto elem) { CHECK(r.contains(elem)); }};
  r.for_each(std::ref(counted_callback));
  CHECK(counted_callback.count() == 4);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("meta::Range::contains") {
  constexpr TestRange<A, B, C, D> r{};
  STATIC_CHECK(r.contains(A{}));
  STATIC_CHECK(r.contains(B{}));
  STATIC_CHECK(r.contains(C{}));
  STATIC_CHECK(r.contains(D{}));
  STATIC_CHECK_FALSE(r.contains(E{}));
}

TEST_CASE("meta::Range::includes") {
  constexpr TestRange<A, B, C, D> r{};
  STATIC_CHECK(r.includes(r));
  STATIC_CHECK(r.includes(meta::Set<A, B, C, D>{}));
  STATIC_CHECK(r.includes(meta::Set<A, B, C>{}));
  STATIC_CHECK_FALSE(r.includes(meta::Set<E, F>{}));
  STATIC_CHECK_FALSE(r.includes(meta::Set<A, F>{}));
}

TEST_CASE("meta::Range::find") {
  constexpr TestRange<A, B, C, D> r{};
  STATIC_CHECK(r.find(A{}) == 0);
  STATIC_CHECK(r.find(B{}) == 1);
  STATIC_CHECK(r.find(C{}) == 2);
  STATIC_CHECK(r.find(D{}) == 3);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
