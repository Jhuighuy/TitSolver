/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <type_traits>

#include "tit/core/type_traits.hpp"

namespace tit {
namespace {

struct A {};
struct B {};
struct C {};
struct D {};
struct E {};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

template<class>
struct Template1 {};

template<class>
struct Template2 {};

static_assert(specialization_of<Template1<A>, Template1>);
static_assert(!specialization_of<Template1<A>, Template2>);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

static_assert(contains_v<A, A, B, C>);
static_assert(!contains_v<A, B, C, D>);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

static_assert(std::is_same_v<type_at_t<0, A, B, C>, A>);
static_assert(std::is_same_v<type_at_t<1, A, B, C>, B>);
static_assert(std::is_same_v<type_at_t<2, A, B, C>, C>);

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

} // namespace
} // namespace tit
