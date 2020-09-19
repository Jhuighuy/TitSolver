/** Copyright (C) Oleg Butakov 2020. */
#pragma once
#ifndef TIT_HELPERS_HPP_
#define TIT_HELPERS_HPP_

#include <cmath>

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** @f$\pi@f$ constant. */
template<typename real_t>
static const real_t s_Pi = 4.0*std::atan(1.0);

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Compute square. */
template <typename type_t> constexpr 
inline type_t Square(type_t value) {
    return value*value;
}   // Square

/** Compute square. */
template <typename type_t> constexpr 
inline type_t Pow2(type_t value) {
    return std::pow(value, 2);
}   // Square
template <typename type_t> constexpr 
inline type_t Pow3(type_t value) {
    return std::pow(value, 3);
}   // Square
template <typename type_t> constexpr 
inline type_t Pow4(type_t value) {
    return std::pow(value, 4);
}   // Square
template <typename type_t> constexpr 
inline type_t Pow5(type_t value) {
    return std::pow(value, 5);
}   // Square

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Compute average. */
template <typename type_t> constexpr 
inline type_t Average(type_t valueA, type_t valueB) {
    return 0.5*(valueA + valueB);
}   // Square

/** Compute sign. */
template <typename type_t> constexpr 
inline int Sign(type_t value) {
    const type_t _0(0);
    return (_0 < value) - (value < _0);
}   // Sign

/** Compute safe inverse. */
template <typename type_t> constexpr 
inline type_t SafeInverse(type_t value) {
    const type_t _0(0), _1(1);
    return value == _0 ? _0 : _1/value;
}   // SafeInverse

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#endif  // ifndef TIT_HELPERS_HPP_
