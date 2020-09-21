/** Copyright (C) Oleg Butakov 2020. */
#pragma once
#ifndef TIT_HELPERS_HPP_
#define TIT_HELPERS_HPP_

#include <cmath>
#include <cassert>

#define TIT_ASSERT assert

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** @f$\pi@f$ constant. */
template<typename Real>
static const Real Pi = Real(4.0*std::atan(1.0));
/** @f$\sqrt{\pi}@f$ constant. */
template<typename Real>
static const Real SqrtPi = Real(std::tgamma(0.5));

/** @f$+\infty@f$ constant. */
template<typename Real>
static const Real Infinity = std::numeric_limits<Real>::inifinity();

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Sign function. */
template<typename Type> constexpr 
inline int Sign(Type value) 
{
    return (Type(0) < value) - (value < Type(0));
}

/** Absolute value function. */
template<typename Real> constexpr 
inline Real Abs(Real value) 
{
    return std::abs(value);
}

/** Compute average. */
template<typename Type> constexpr 
inline Type Average(Type valueA, Type valueB) 
{
    return 0.5*(valueA + valueB);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Inverse function. */
template<typename Real> constexpr 
inline Real Inverse(Real value) 
{
    static_assert(std::is_floating_point_v<Real>);
    return Real(1.0)/value;
}
/** Safe inverse function. 
 ** @returns Inverse for non-zero input, zero for zero input. */
template<typename Real> constexpr 
inline Real SafeInverse(Real value) 
{
    static_assert(std::is_floating_point_v<Real>);
    return (value == Real(0.0)) ? Real(0.0) : Inverse(value);
}

/** Varous power functions. */
/** @{ */
template<typename Type> constexpr 
inline Type Pow2(Type value) 
{
    return std::pow(value, 2);
}
template<typename Type> constexpr 
inline Type Pow3(Type value) 
{
    return std::pow(value, 3);
}
template<typename Type> constexpr 
inline Type Pow4(Type value) 
{
    return std::pow(value, 4);
}
template<typename Type> constexpr 
inline Type Pow5(Type value) 
{
    return std::pow(value, 5);
}
template<typename Type> constexpr 
inline Type Pow(Type value, int power) 
{
    return std::pow(value, power);
}
template<typename Real> 
inline Real Pow(Real value, Real power) 
{
    static_assert(std::is_floating_point_v<Real>);
    return std::pow(value, power);
}
/** @} */

/** Square root function. */
template<typename Real> 
inline Real Sqrt(Real value) 
{
    static_assert(std::is_floating_point_v<Real>);
    return std::sqrt(value);
}

/** @f$\sqrt{a^2+b^2}@f$ function. */
template<typename Real> 
inline Real Hypot(Real valueA, Real valueB) 
{
    static_assert(std::is_floating_point_v<Real>);
    return std::hypot(valueA, valueB);
}
/** @f$\sqrt{a^2+b^2+c^2}@f$ function. */
template<typename Real> 
inline Real Hypot(Real valueA, Real valueB, Real valueC) 
{
    static_assert(std::is_floating_point_v<Real>);
    return std::hypot(valueA, valueB, valueC);
}

/** Exponent function. */
template<typename Real> 
inline Real Exp(Real value) 
{
    static_assert(std::is_floating_point_v<Real>);
    return std::exp(value);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** @{ */
template<typename Type> constexpr 
inline Type Select(bool condition, Type value)
{
    assert(condition);
    return value;
}
template<typename Type, typename... RestArgs> constexpr 
inline Type Select(bool condition, Type value, RestArgs... args)
{
    return condition ? value : Select(args...);
}
/** @} */

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#endif  // ifndef TIT_HELPERS_HPP_
