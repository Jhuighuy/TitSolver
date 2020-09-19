/** Copyright (C) Oleg Butakov 2020. */
#pragma once
#ifndef TIT_ROOT_FINDER_HPP_
#define TIT_ROOT_FINDER_HPP_

#include <tuple>
#include <cmath>

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/********************************************************************
 ** Find Root using the Bisection method.
 ********************************************************************/
template<typename real_t, typename func_t>
static std::tuple<bool, real_t, real_t> FindRoot_Bisection(
    real_t minX, real_t maxX, const func_t& F,
    real_t delta=1e-3, real_t epsilon=1e-2, size_t maxNumIters=100
) {
    static_assert(std::is_floating_point_v<real_t>);
    assert(minX <= maxX);
    // Compute value at the left end.
    real_t minF = F(minX);
    if (std::abs(minF) < epsilon) {
        return std::make_tuple(true, minX, minF);
    }
    // Compute value at the right end.
    real_t maxF = F(maxX);
    if (std::abs(maxF) < epsilon) {
        return std::make_tuple(true, maxX, maxF);
    }
    // Can we find the root?
    if (minF*maxF > 0.0) {
        return std::make_tuple(false, 0.0, 0.0);
    }
    // Find the root!
    for (size_t iter = 0; iter < maxNumIters; ++iter) {
        // Compute value at the average.
        real_t averageX = 0.5*(minX + maxX);
        real_t averageF = F(averageX);
        if (std::abs(averageF) < epsilon) {
            return std::make_tuple(true, averageX, averageF);
        }
        if (minF*averageF > 0.0) {
            minX = averageX, minF = averageF;
        } else if (averageF*maxF > 0.0) {
            maxX = averageX, maxF = averageF;
        } else {
            return std::make_tuple(false, 0.0, 0.0);
        }
        if ((maxX - minX) < delta) {
            return std::make_tuple(true, averageX, averageF);
        }
    }
    return std::make_tuple(false, 0.0, 0.0);
}   // FindRoot_Bisection

/********************************************************************
 ** Find Root using Newton's method.
 ********************************************************************/
template<typename real_t, typename func_t>
static void FindRoot(
    real_t& x, const func_t& F,
    real_t delta=1e-3, size_t maxNumIters=100
) {
    //std::cout << x << ' ';
    //std::cout.flush();
    for (size_t iter = 0; iter < maxNumIters; ++iter) {
        auto [value, derivativeValue] = F();
        real_t deltaX = value/derivativeValue;
        x -= deltaX;
        //std::cout << x << ' ';
        //std::cout.flush();
        if (std::abs(deltaX/x) < delta) {
            break;
        }
    }
    //std::cout << std::endl;
    //std::cout.flush();
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/


#endif  // ifndef TIT_ROOT_FINDER_HPP_
