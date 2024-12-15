/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <functional>
#include <utility>

#include <boost/container/flat_map.hpp>
#include <boost/container/small_vector.hpp>
#include <boost/container/static_vector.hpp>

#include "tit/core/basic_types.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Boost small vector.
template<class T, size_t N>
using SmallVector = boost::container::small_vector<T, N>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Boost inplace vector.
template<class T, size_t N>
using InplaceVector = boost::container::static_vector<T, N>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Boost flat map.
template<class Key, class T, class Compare = std::less<Key>>
using FlatMap = boost::container::flat_map<Key, T, Compare>;

/// Boost small flat map.
template<class Key, class T, size_t N, class Compare = std::less<Key>>
using SmallFlatMap = boost::container::
    flat_map<Key, T, Compare, SmallVector<std::pair<Key, T>, N>>;

/// Boost inplace flat map.
template<class Key, class T, size_t N, class Compare = std::less<Key>>
using InplaceFlatMap = boost::container::
    flat_map<Key, T, Compare, InplaceVector<std::pair<Key, T>, N>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
