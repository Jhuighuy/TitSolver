/** Copyright (C) Oleg Butakov 2020. */
#pragma once
#ifndef TIT_HELPERS_HPP_
#define TIT_HELPERS_HPP_

#include <cmath>

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** @f$\pi@f$ constant. */
template<typename Real>
static const Real Pi = 4.0*std::atan(1.0);

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Compute square. */
template <typename Type> constexpr 
inline Type Square(Type value) 
{
    return value*value;
}

template <typename Type> constexpr 
inline Type Pow2(Type value) 
{
    return std::pow(value, 2);
}
template <typename Type> constexpr 
inline Type Pow3(Type value) 
{
    return std::pow(value, 3);
}
template <typename Type> constexpr 
inline Type Pow4(Type value) 
{
    return std::pow(value, 4);
}
template <typename Type> constexpr 
inline Type Pow5(Type value) 
{
    return std::pow(value, 5);
}

template <typename Type> 
inline Type Exp(Type value) 
{
    return std::exp(value);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Compute average. */
template <typename Type> constexpr 
inline Type Average(Type valueA, Type valueB) 
{
    return 0.5*(valueA + valueB);
}

/** Compute sign. */
template <typename Type> constexpr 
inline int Sign(Type value) 
{
    return (Type(0) < value) - (value < Type(0));
}

/** Compute safe inverse. */
template <typename Type> constexpr 
inline Type SafeInverse(Type value) 
{
    return value == Type(0) ? Type(0) : Type(1)/value;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#endif  // ifndef TIT_HELPERS_HPP_
