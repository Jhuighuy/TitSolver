/** Copyright (C) Oleg Butakov 2020. */
#pragma once
#ifndef TIT_ARTIFICIAL_VISCOSITY_HPP_
#define TIT_ARTIFICIAL_VISCOSITY_HPP_

#include "TitSmoothingKernels.hpp"

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/********************************************************************
 ** Abstract artificial viscosity estimator.
 ********************************************************************/
template<typename real_t, int nDim>
class TArtificialViscosity {
public:
  /** 
   * Compute artificial viscosity. 
   */
  virtual real_t Value(
    const TParticle<real_t>& a, const TParticle<real_t>& b, real_t kernelWidth
  ) const noexcept = 0;
};  // class TArtificialViscosityEstimator

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/********************************************************************
 ** Dummy artificial viscosity estimator.
 ********************************************************************/
template<typename real_t, int nDim>
class TArtificialViscosity_Dummy final
  : public TArtificialViscosity<real_t, nDim> {
public:
  real_t Value(
    const TParticle<real_t>& a, const TParticle<real_t>& b, real_t kernelWidth
  ) const noexcept override;
};  // class TArtificialViscosityEstimator_Dummy

template<typename real_t, int nDim>
real_t TArtificialViscosity_Dummy<real_t, nDim>::Value(
  const TParticle<real_t>& a, const TParticle<real_t>& b, real_t kernelWidth
) const noexcept {
  static_cast<void>(a), static_cast<void>(b);
  static_cast<void>(kernelWidth);
  return 0.0;
}   // TArtificialViscosityEstimator_Dummy::Value

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/********************************************************************
 ** The Alpha-Beta artificial viscosity estimator.
 ********************************************************************/
template<typename real_t, int nDim>
class TArtificialViscosity_AlphaBeta final
  : public TArtificialViscosity<real_t, nDim> {
private:
  const real_t m_AlphaSPH, m_BetaSPH;
public:
  TArtificialViscosity_AlphaBeta(real_t alphaSPH = 1.0, 
                   real_t betaSPH = 2.0);
  real_t Value(
    const TParticle<real_t>& a, const TParticle<real_t>& b, real_t kernelWidth
  ) const noexcept override;
};  // class TArtificialViscosityEstimator_AlphaBeta

template<typename real_t, int nDim>
TArtificialViscosity_AlphaBeta<real_t, nDim>::TArtificialViscosity_AlphaBeta(
  real_t alphaSPH, real_t betaSPH
) : m_AlphaSPH(alphaSPH), m_BetaSPH(betaSPH) {
}   // TArtificialViscosity_AlphaBeta::TArtificialViscosity_AlphaBeta

template<typename real_t, int nDim>
real_t TArtificialViscosity_AlphaBeta<real_t, nDim>::Value(
  const TParticle<real_t>& a, const TParticle<real_t>& b, real_t kernelWidth
) const noexcept {
  TVector<real_t, nDim> 
    deltaVelocity = a.Velocity - b.Velocity,
    deltaPosition = a.Position - b.Position;
  if (Dot(deltaVelocity, deltaPosition) >= 0.0) {
    return 0.0;
  }
  real_t averageDensity = 0.5*(a.Density + b.Density),
       averageSoundSpeed = 0.5*(a.SoundSpeed + b.SoundSpeed);
  static const real_t Epsilon = 1e-2;
  real_t dynamicViscosity = 
    kernelWidth*Dot(deltaVelocity,deltaPosition)/(
      Dot(deltaPosition,deltaPosition) + Epsilon*Pow2(kernelWidth)
    );
  return (-m_AlphaSPH*averageSoundSpeed + m_BetaSPH*dynamicViscosity)*dynamicViscosity/averageDensity;
}   // TArtificialViscosity_AlphaBeta::Value

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/


#endif  // ifndef TIT_ARTIFICIAL_VISCOSITY_HPP_
