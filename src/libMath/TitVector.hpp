/** Copyright (C) Oleg Butakov 2020. */
#pragma once
#ifndef TIT_LIBMATH_VECTOR_HPP_
#define TIT_LIBMATH_VECTOR_HPP_

#include <cmath>
#include <array>
#include <tuple>
#include <iostream>
#include <algorithm>
#include <type_traits>

#include "libMath/TitMath.hpp"

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/********************************************************************
 ** Algebraic vector.
 ********************************************************************/
template<typename Real, int Dim>
union Vector;
template<typename Real, int Dim>
using TVector = Vector<Real, Dim>;

template<typename Real>
union Vector<Real, 1> final 
{
    static_assert(std::is_floating_point_v<Real>);
    std::array<Real, 1> Components;
    struct { Real x; };

    /** Initialize a vector with constant component. */
    constexpr Vector(Real a=Real(0.0)) : Components{a} {}
};  // union Vector<1>

template<typename Real>
union Vector<Real, 2> final 
{
    static_assert(std::is_floating_point_v<Real>);
    std::array<Real, 2> Components;
    struct { Real x, y; };

    /** Initialize a vector with constant component. */
    constexpr Vector(Real a=Real(0.0)) : Components{a,a} {}
};  // union Vector<2>

template<typename Real>
union Vector<Real, 3> final 
{
    static_assert(std::is_floating_point_v<Real>);
    std::array<Real, 3> Components;
    struct { Real x, y, z; };

    /** Initialize a vector with constant component. */
    constexpr Vector(Real a=Real(0.0)) : Components{a,a,a} {}
};  // union Vector<3>

template<typename Real>
union Vector<Real, 4> final 
{
    static_assert(std::is_floating_point_v<Real>);
    std::array<Real, 4> Components;
    struct { Real x, y, z, w; };

    /** Initialize a vector with constant component. */
    constexpr Vector(Real a=Real(0.0)) : Components{a,a,a,a} {}
};  // union Vector<4>

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Vector unary plus operator. */
template<typename Real, int Dim> constexpr
Vector<Real, Dim> operator+(Vector<Real, Dim> p) noexcept 
{
    return p;
}
/** Vector addition operator. */
template<typename Real, int Dim> constexpr
Vector<Real, Dim> operator+(Vector<Real, Dim> p, Vector<Real, Dim> q) noexcept 
{
    return p += q;
}
/** Vector addition assignment operator. */
template<typename Real, int Dim> constexpr 
Vector<Real, Dim>& operator+=(Vector<Real, Dim>& p, Vector<Real, Dim> q) noexcept 
{
    for (int iDim = 0; iDim < Dim; ++iDim) 
    {
        p.Components[iDim] += q.Components[iDim];
    }
    return p;
}

/** Vector negation operator. */
template<typename Real, int Dim> constexpr 
Vector<Real, Dim> operator-(Vector<Real, Dim> p) noexcept 
{
    for (int iDim = 0; iDim < Dim; ++iDim) 
    {
        p.Components[iDim] = -p.Components[iDim];
    }
    return p;
}
/** Vector subtraction operator. */
template<typename Real, int Dim> constexpr 
Vector<Real, Dim> operator-(Vector<Real, Dim> p, Vector<Real, Dim> q) noexcept 
{
    return p -= q;
}
/** Vector subtraction assignment operator. */
template<typename Real, int Dim>
Vector<Real, Dim>& operator-=(Vector<Real, Dim>& p, Vector<Real, Dim> q) noexcept 
{
    for (int iDim = 0; iDim < Dim; ++iDim) 
    {
        p.Components[iDim] -= q.Components[iDim];
    }
    return p;
}

/** Vector multiplication operator. */
/** @{ */
template<typename Real, int Dim> constexpr
Vector<Real, Dim> operator*(Real a, Vector<Real, Dim> p) noexcept 
{
    return p *= a;
}
template<typename Real, int Dim> constexpr
Vector<Real, Dim> operator*(Vector<Real, Dim> p, Real a) noexcept 
{
    return p *= a;
}
/** @} */
/** Vector multiplication assignment operator. */
template<typename Real, int Dim> constexpr
Vector<Real, Dim>& operator*=(Vector<Real, Dim>& p, Real a) noexcept 
{
    for (int iDim = 0; iDim < Dim; ++iDim) 
    {
        p.Components[iDim] *= a;
    }
    return p;
}

/** Vector division operator. */
template<typename Real, int Dim> constexpr
Vector<Real, Dim> operator/(Vector<Real, Dim> p, Real a) noexcept 
{
    return p /= a;
}
/** Vector division assignment operator. */
template<typename Real, int Dim> constexpr
Vector<Real, Dim>& operator/=(Vector<Real, Dim>& p, Real a) noexcept 
{
    for (int iDim = 0; iDim < Dim; ++iDim) 
    {
        p.Components[iDim] /= a;
    }
    return p;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

template<typename Real, int Dim> constexpr 
int _Compare(Vector<Real, Dim> p, Vector<Real, Dim> q) noexcept 
{
    for (int iDim = 0; iDim < Dim; ++iDim) 
    {
        int sign = Sign(p.Components[iDim] - q.Components[iDim]);
        if (sign != 0) return sign;
    }
    return 0;
}

/** Vector equality operator. */
template<typename Real, int Dim> constexpr 
bool operator==(Vector<Real, Dim> p, Vector<Real, Dim> q) noexcept 
{
    return _Compare(p, q) == 0;
}
/** Vector inequality operator. */
template<typename Real, int Dim> constexpr 
bool operator!=(Vector<Real, Dim> p, Vector<Real, Dim> q) noexcept 
{
    return _Compare(p, q) != 0;
}

/** Vector lexicographical less then operator. */
template<typename Real, int Dim> constexpr
bool Less(Vector<Real, Dim> p, Vector<Real, Dim> q) noexcept 
{
    return _Compare(p, q) < 0;
}
/** Vector lexicographical less then or equal operator. */
template<typename Real, int Dim> constexpr 
bool LessOrEqual(Vector<Real, Dim> p, Vector<Real, Dim> q) noexcept 
{
    return _Compare(p, q) <= 0;
}

/** Vector lexicographical greater then operator. */
template<typename Real, int Dim> constexpr 
bool Greater(Vector<Real, Dim> p, Vector<Real, Dim> q) noexcept 
{
    return _Compare(p, q) > 0;
}
/** Vector lexicographical greater then or equal operator. */
template<typename Real, int Dim> constexpr 
bool GreaterOrEqual(Vector<Real, Dim> p, Vector<Real, Dim> q) noexcept 
{
    return _Compare(p, q) >= 0;
}

/** Vector component-wise minimum. */
template<typename Real, int Dim> constexpr 
Vector<Real, Dim> Min(Vector<Real, Dim> p, Vector<Real, Dim> q) noexcept 
{
    for (int iDim = 0; iDim < Dim; ++iDim) 
    {
        p.Components[iDim] = Min(p.Components[iDim], q.Components[iDim]);
    }
    return p;
}
/** Vector component-wise maximum. */
template<typename Real, int Dim> constexpr 
Vector<Real, Dim> Max(Vector<Real, Dim> p, Vector<Real, Dim> q) noexcept 
{
    for (int iDim = 0; iDim < Dim; ++iDim) 
    {
        p.Components[iDim] = Max(p.Components[iDim], q.Components[iDim]);
    }
    return p;
}
/** Vector component-wise minimum-maximum pair. */
template<typename Real, int Dim> constexpr 
Pair<Vector<Real, Dim>,Vector<Real, Dim>> 
MinMax(Vector<Real, Dim> p, Vector<Real, Dim> q) noexcept 
{
    for (int iDim = 0; iDim < Dim; ++iDim) 
    {
        Tie(p.Components[iDim], q.Components[iDim]) = 
            MinMax(p.Components[iDim], q.Components[iDim]);
    }
    return MakePair(p, q);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Vector dot product. */
template<typename Real, int Dim> constexpr
Real Dot(Vector<Real, Dim> p, Vector<Real, Dim> q) noexcept {
    Real d(0.0);
    for (int iDim = 0; iDim < Dim; ++iDim) 
    {
        d += p.Components[iDim]*q.Components[iDim];
    }
    return d;
}

/** Compute vector norm. */
/** @{ */
template<typename Real>
Real Norm(Vector<Real,1> p) noexcept 
{
    return Abs(p.x);
}
template<typename Real>
Real Norm(Vector<Real,2> p) noexcept 
{
    return Hypot(p.x, p.y);
}
template<typename Real>
Real Norm(Vector<Real,3> p) noexcept 
{
    return Hypot(p.x, p.y, p.z);
}
template<typename Real, int Dim>
Real Norm(Vector<Real, Dim> p) noexcept 
{
    return Sqrt(Dot(p, p));
}
/** @} */

/** Normalize vector. */
template<typename Real, int Dim>
Vector<Real, Dim> Normalize(Vector<Real, Dim> p) noexcept 
{
    return SafeInverse(Norm(p))*p;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Vector input operator. */
template<typename char_t, typename Real, int Dim>
std::basic_istream<char_t>& 
operator>>(std::basic_istream<char_t>& stream, Vector<Real, Dim>& p) 
{
    for (int iDim = 0; iDim < Dim; ++iDim) 
    {
        stream >> p.Components[iDim];
    }
    return stream;
}
/** Vector output operator. */
template<typename char_t, typename Real, int Dim>
std::basic_ostream<char_t>& 
operator<<(std::basic_ostream<char_t>& stream, Vector<Real, Dim> p) 
{
    stream << p.Components.front();
    for (int iDim = 1; iDim < Dim; ++iDim) 
    {
        stream << " " << p.Components[iDim];
    }
    return stream;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#endif  // ifndef TIT_LIBMATH_VECTOR_HPP_
