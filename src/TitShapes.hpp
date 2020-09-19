/** Copyright (C) Oleg Butakov 2020. */
#pragma once
#ifndef TIT_SHAPES_HPP_
#define TIT_SHAPES_HPP_

#include <cmath>

#include "TitVector.hpp"

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/********************************************************************
 ** Sphere shape.
 ********************************************************************/
template<typename real_t, int nDim>
struct TSphere {
    TVector<real_t, nDim> Center;
    real_t Radius;
    constexpr TSphere() = default;
};  // struct TSphere

/** Test if point is inside a sphere. */
template<typename real_t, int nDim> constexpr 
bool IsPointInsideSphere(
    const TVector<real_t, nDim>& point, const TSphere<real_t, nDim>& sphere
) noexcept {
    TSphere<real_t, nDim> delta = point - sphere.Center;
    return Dot(delta, delta) < std::pow(sphere.Radius, 2);
}   // IsPointInside

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/********************************************************************
 ** Axis-aligned box shape.
 ********************************************************************/
template<typename real_t, int nDim>
struct TBox {
    TVector<real_t, nDim> Min, Max;
    constexpr TBox() = default;
};  // struct TBox

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#endif  // ifndef TIT_SHAPES_HPP_
