/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <cstddef> // IWYU pragma: keep
#ifdef __GLIBCXX__
#include <type_traits>
#endif

#include <boost/container/static_vector.hpp>

#include "tit/core/basic_types.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Inplace vector.
template<class Val, size_t Capacity>
using InplaceVector = boost::container::static_vector<Val, Capacity>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit

// GCC 15 seems to wrongly identify InplaceVector as not being noexcept
// swappable. We have to make a workaround.
#ifdef __GLIBCXX__
template<class Val, size_t Capacity>
struct std::is_nothrow_swappable<tit::InplaceVector<Val, Capacity>> :
    std::is_nothrow_swappable<Val> {};
#endif
