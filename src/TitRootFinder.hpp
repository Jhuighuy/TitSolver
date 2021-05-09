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
template<typename Real, typename Func>
static std::tuple<bool,Real,Real> 
FindRoot_Bisection(Real minX, Real maxX, const Func& F,
  Real delta=Real(1e-3), Real epsilon=Real(1e-2), size_t maxNumIters=100) 
{
  static_assert(std::is_floating_point_v<Real>);
  assert(minX <= maxX);
  Real minF = F(minX);
  if (Abs(minF) < epsilon) 
  {
    return std::make_tuple(true, minX, minF);
  }
  Real maxF = F(maxX);
  if (Abs(maxF) < epsilon) 
  {
    return std::make_tuple(true, maxX, maxF);
  }
  static constexpr auto errorCode = 
    std::make_tuple(false, Real(0.0), (0.0));
  if (Sign(minF) == Sign(maxF)) 
  {
    return errorCode;
  }
  /* Find the root! */
  for (size_t iter = 0; iter < maxNumIters; ++iter) {
    Real midX = Average(minX, maxX);
    Real midF = F(midX);
    if (Abs(midF) < epsilon) 
    {
      return std::make_tuple(true, midX, midF);
    }
    if (Sign(minF) == Sign(midF)) 
    {
      minX = midX; 
      minF = midF;
    } 
    else if (Sign(midF) == Sign(maxF)) 
    {
      maxX = midX; 
      maxF = midF;
    } 
    else 
    {
      return errorCode;
    }
    if ((maxX - minX) < delta) 
    {
      return std::make_tuple(true, midX, midF);
    }
  }
  return errorCode;
}   // FindRoot_Bisection

/********************************************************************
 ** Find Root using Newton's method.
 ********************************************************************/
template<typename Real, typename Func>
static void FindRoot(
  Real& x, const Func& F,
  Real delta=1e-3, size_t maxNumIters=100
) {
  //std::cout << x << ' ';
  //std::cout.flush();
  for (size_t iter = 0; iter < maxNumIters; ++iter) {
    auto [value, derivativeValue] = F();
    Real deltaX = value/derivativeValue;
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
