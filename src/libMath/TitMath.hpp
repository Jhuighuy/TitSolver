/** Copyright (C) Oleg Butakov 2020. */
#pragma once
#ifndef TIT_LIBMATH_MATH_HPP_
#define TIT_LIBMATH_MATH_HPP_

#include <cmath>
#include <cassert>
#include <limits>
#include <algorithm>
#include <type_traits>

#include "TitHelpers.hpp"

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** @f$\pi@f$ constant. */
template<typename Real>
static const std::enable_if_t<std::is_floating_point_v<Real>,Real> Pi 
{ 
    Real(4.0*std::atan(1.0)) 
};
/** @f$\sqrt{\pi}@f$ constant. */
template<typename Real>
static const std::enable_if_t<std::is_floating_point_v<Real>,Real> SqrtPi 
{ 
    Real(std::tgamma(0.5)) 
};

/** @f$+\infty@f$ constant. */
template<typename Real>
static const std::enable_if_t<std::is_floating_point_v<Real>,Real> Infinity 
{ 
    Real(std::numeric_limits<Real>::inifinity())
};

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Absolute value function. */
template<typename Type> constexpr 
inline Type Abs(Type value) noexcept
{
    return std::abs(value);
}

/** Sign function. */
template<typename Type> constexpr 
inline int Sign(Type value) noexcept
{
    return (Type(0) < value) - (value < Type(0));
}

/** Minimum value function. */
template<typename Type> constexpr 
inline Type Min(Type valueA, Type valueB) noexcept
{
    return std::min(valueA, valueB);
}
/** Maximum value function. */
template<typename Type> constexpr 
inline Type Max(Type valueA, Type valueB) noexcept
{
    return std::max(valueA, valueB);
}
/** Minimum-maximum value pair function. */
template<typename Type> constexpr 
inline Pair<Type,Type> MinMax(Type valueA, Type valueB) noexcept
{
    return std::minmax(valueA, valueB);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Compute average. */
template<typename Type> constexpr 
inline Type Average(Type valueA, Type valueB) noexcept
{
    return 0.5*(valueA + valueB);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Varous power functions. */
/** @{ */
template<typename Type> constexpr 
inline Type Pow2(Type value) noexcept
{
    return std::pow(value, 2);
}
template<typename Type> constexpr 
inline Type Pow3(Type value) noexcept
{
    return std::pow(value, 3);
}
template<typename Type> constexpr 
inline Type Pow4(Type value) noexcept
{
    return std::pow(value, 4);
}
template<typename Type> constexpr 
inline Type Pow5(Type value) noexcept
{
    return std::pow(value, 5);
}
template<typename Type> constexpr 
inline Type Pow(Type value, unsigned power) noexcept
{
    return std::pow(value, power);
}
template<typename Real> 
inline Real Pow(Real value, Real power) noexcept
{
    static_assert(std::is_floating_point_v<Real>);
    assert(value >= Real(0.0));
    return std::pow(value, power);
}
/** @} */

/** Square root function. */
template<typename Real> 
inline Real Sqrt(Real value) noexcept
{
    static_assert(std::is_floating_point_v<Real>);
    assert(value >= Real(0.0));
    return std::sqrt(value);
}

/** @f$\sqrt{a^2+b^2}@f$ function. */
template<typename Real> 
inline Real Hypot(Real valueA, Real valueB) noexcept
{
    static_assert(std::is_floating_point_v<Real>);
    return std::hypot(valueA, valueB);
}
/** @f$\sqrt{a^2+b^2+c^2}@f$ function. */
template<typename Real> 
inline Real Hypot(Real valueA, Real valueB, Real valueC) noexcept
{
    static_assert(std::is_floating_point_v<Real>);
    return std::hypot(valueA, valueB, valueC);
}

/** Inverse function. */
template<typename Real> constexpr 
inline Real Inverse(Real value) noexcept
{
    static_assert(std::is_floating_point_v<Real>);
    assert(value != Real(0.0));
    return Real(1.0)/value;
}
/** Safe inverse function. 
 ** @returns Inverse for non-zero input, zero for zero input. */
template<typename Real> constexpr 
inline Real SafeInverse(Real value) noexcept
{
    static_assert(std::is_floating_point_v<Real>);
    return (value == Real(0.0)) ? Real(0.0) : Inverse(value);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Exponent function. */
template<typename Real> 
inline Real Exp(Real value) noexcept
{
    static_assert(std::is_floating_point_v<Real>);
    return std::exp(value);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#endif  // ifndef TIT_LIBMATH_MATH_HPP_
