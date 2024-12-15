/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "tit/core/meta/list.hpp"

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

TEST_CASE("meta::List::operator+") {
  constexpr meta::List<A, B> s1{};
  constexpr meta::List<B, C> s2{};
  constexpr meta::List<C, D> s3{};
  STATIC_CHECK(s1 + s2 + s3 == meta::List<A, B, B, C, C, D>{});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("meta::List::cartesian_product") {
  STATIC_CHECK(
      meta::cartesian_product(meta::List<A, B>{}, meta::List<C, D>{}) ==
      meta::List<meta::List<A, C>,
                 meta::List<A, D>,
                 meta::List<B, C>,
                 meta::List<B, D>>{});
  STATIC_CHECK(
      meta::cartesian_product(meta::List<A, B, C>{}, meta::List<D, E, F>{}) ==
      meta::List<meta::List<A, D>,
                 meta::List<A, E>,
                 meta::List<A, F>,
                 meta::List<B, D>,
                 meta::List<B, E>,
                 meta::List<B, F>,
                 meta::List<C, D>,
                 meta::List<C, E>,
                 meta::List<C, F>>{});
  STATIC_CHECK(meta::cartesian_product(meta::List<A, B>{},
                                       meta::List<C, D>{},
                                       meta::List<E, F>{}) ==
               meta::List<meta::List<A, C, E>,
                          meta::List<A, C, F>,
                          meta::List<A, D, E>,
                          meta::List<A, D, F>,
                          meta::List<B, C, E>,
                          meta::List<B, C, F>,
                          meta::List<B, D, E>,
                          meta::List<B, D, F>>{});
  STATIC_CHECK(meta::cartesian_product(meta::List<A, B, C>{},
                                       meta::List<D>{},
                                       meta::List<E, F>{}) ==
               meta::List<meta::List<A, D, E>,
                          meta::List<A, D, F>,
                          meta::List<B, D, E>,
                          meta::List<B, D, F>,
                          meta::List<C, D, E>,
                          meta::List<C, D, F>>{});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
