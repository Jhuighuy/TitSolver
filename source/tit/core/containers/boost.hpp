/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <functional>
#include <utility>

#include <boost/container/flat_map.hpp>
#include <boost/container/small_vector.hpp>
#include <boost/container/static_vector.hpp>

#include "tit/core/basic_types.hpp"

namespace tit {

namespace bc = boost::container;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Boost small vector.
template<class T, size_t N>
using SmallVector = bc::small_vector<T, N>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Boost inplace vector.
template<class T, size_t N>
using InplaceVector = bc::static_vector<T, N>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Boost flat map.
template<class K, class V, class Compare = std::less<K>>
using FlatMap = bc::flat_map<K, V, Compare>;

/// Boost small flat map.
template<class K, class V, size_t N, class C = std::less<K>>
using SmallFlatMap = bc::flat_map<K, V, C, SmallVector<std::pair<K, V>, N>>;

/// Boost inplace flat map.
template<class K, class V, size_t N, class C = std::less<K>>
using InplaceFlatMap = bc::flat_map<K, V, C, InplaceVector<std::pair<K, V>, N>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
