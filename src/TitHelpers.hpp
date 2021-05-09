/** Copyright (C) Oleg Butakov 2020. */
#pragma once
#ifndef TIT_HELPERS_HPP_
#define TIT_HELPERS_HPP_

#include <cmath>
#include <cassert>
#include <tuple>

#include "libMath/TitMath.hpp"

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Select function. */
/** @{ */
template<typename Type> constexpr 
inline Type Select(bool condition, Type value) noexcept
{
  assert(condition);
  return value;
}
template<typename Type, typename... RestTypes> constexpr 
inline Type Select(bool condition, Type value, RestTypes... args) noexcept
{
  return condition ? value : Select(args...);
}
/** @} */

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

template<typename TypeA, typename TypeB>
using Pair = std::pair<TypeA,TypeB>;
template <typename... Types>
using Tuple = std::tuple<Types...>;

template<typename TypeA, typename TypeB> constexpr
inline Pair<TypeA,TypeB> MakePair(TypeA&& valueA, TypeB&& valueB) noexcept
{
  return std::make_pair(std::forward<TypeA>(valueA), std::forward<TypeB>(valueB));
}
template<typename... Types>
inline Tuple<std::decay_t<Types>...> MakeTuple(Types&&... values) noexcept
{
  return std::make_tuple(std::forward<Types>(values)...);
}

template<typename... Types>
inline Tuple<Types&...> Tie(Types&... values) noexcept
{
  return Tuple<Types&...>(values...);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#endif  // ifndef TIT_HELPERS_HPP_
