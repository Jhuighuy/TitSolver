/** Copyright (C) Oleg Butakov 2020. */
#pragma once
#ifndef TIT_SMOOTH_ESTIMATOR_HPP_
#define TIT_SMOOTH_ESTIMATOR_HPP_

#include "TitParticle.hpp"
#include "TitSmoothingKernels.hpp"
#include "TitArtificialViscosity.hpp"

/********************************************************************
 ** The particle estimator with a fixed kernel width.
 ********************************************************************/
template<typename real_t, int nDim>
class TSmoothEstimator {
public:
    /** Estimate density, kernel width, pressure and sound speed. */
    virtual void EstimateDensity(
        TParticleArray<real_t, nDim>& particles, 
        const TSmoothingKernel<real_t, nDim>& smoothingKernel
    ) const = 0;
    /** Estimate acceleration and thermal heating. */
    virtual void EstimateAcceleration(
        TParticleArray<real_t, nDim>& particles, 
        const TSmoothingKernel<real_t, nDim>& smoothingKernel,
        const TArtificialViscosity<real_t, nDim>& artificialViscosity
    ) const = 0;
};  // class TSmoothEstimator

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/********************************************************************
 ** The particle estimator with a fixed kernel width.
 ********************************************************************/
template<typename real_t, int nDim>
class TClassicSmoothEstimator final
    : public TSmoothEstimator<real_t, nDim> {
public:
    real_t m_KernelWidth;
    TClassicSmoothEstimator(real_t w) : m_KernelWidth(w) {}
public:
    /** Estimate density, kernel width, pressure and sound speed. */
    void EstimateDensity(TParticleArray<real_t, nDim>& particles, 
        const TSmoothingKernel<real_t, nDim>& smoothingKernel
    ) const final;
    /** Estimate acceleration and thermal heating. */
    void EstimateAcceleration(
        TParticleArray<real_t, nDim>& particles, 
        const TSmoothingKernel<real_t, nDim>& smoothingKernel,
        const TArtificialViscosity<real_t, nDim>& artificialViscosity
    ) const final;
};  // class TSmoothEstimator

template<typename real_t, int nDim>
void TClassicSmoothEstimator<real_t, nDim>::EstimateDensity(
    TParticleArray<real_t, nDim>& particles, 
    const TSmoothingKernel<real_t, nDim>& smoothingKernel
) const {
    real_t fixedSearchWidth = smoothingKernel.GetRadius(m_KernelWidth);
    ForEach(particles, [&](Particle& a) {
        a.Density = {};
        ForEachNeighbour(particles, a, fixedSearchWidth, [&](const Particle& b) {
            real_t abKernelValue = smoothingKernel.GetValue(DeltaPosition(a,b), m_KernelWidth);
            a.Density += b.Mass*abKernelValue;
        });
        a.Pressure = EquationOfState_Pressure(a.Density, a.ThermalEnergy);
        a.SoundSpeed = EquationOfState_SpeedOfSound(a.Density, a.Pressure);
    });
}   // TClassicSmoothEstimator::EstimateDensity

template<typename real_t, int nDim>
void TClassicSmoothEstimator<real_t, nDim>::EstimateAcceleration(
    TParticleArray<real_t, nDim>& particles, 
    const TSmoothingKernel<real_t, nDim>& smoothingKernel,
    const TArtificialViscosity<real_t, nDim>& artificialViscosity
) const {
    real_t fixedSearchWidth = smoothingKernel.GetRadius(m_KernelWidth);
    ForEach(particles, [&](Particle& a) {
        a.Acceleration = {};
        a.Heating = {};
        ForEachNeighbour(particles, a, fixedSearchWidth, [&](const Particle& b) {
            real_t kinematicViscosity = artificialViscosity.Value(a,b, m_KernelWidth);
            TVector<real_t, nDim> abKernelGradient { 
                smoothingKernel.GetGradientValue(DeltaPosition(a,b), m_KernelWidth) 
            };
            a.Acceleration -= b.Mass*(a.Pressure/Square(a.Density) 
                                    + b.Pressure/Square(b.Density) + kinematicViscosity)*abKernelGradient;
            a.Heating += b.Mass*(a.Pressure/Square(a.Density) + kinematicViscosity)*Dot(DeltaVelocity(a,b), abKernelGradient);
        });
    });
}   // TClassicSmoothEstimator::EstimateAcceleration

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/********************************************************************
 ** The particle estimator with a fixed kernel width.
 ********************************************************************/
template<typename real_t, int nDim>
class TGradHSmoothEstimator final
    : public TSmoothEstimator<real_t, nDim> {
public:
    const real_t m_Coupling = 1.55;
    /** Estimate density, kernel width, pressure and sound speed. */
    void EstimateDensity(
        TParticleArray<real_t, nDim>& particles, 
        const TSmoothingKernel<real_t, nDim>& smoothingKernel
    ) const final;
    /** Estimate acceleration and thermal heating. */
    void EstimateAcceleration(
        TParticleArray<real_t, nDim>& particles, 
        const TSmoothingKernel<real_t, nDim>& smoothingKernel,
        const TArtificialViscosity<real_t, nDim>& artificialViscosity
    ) const final;
};  // class TGradHSmoothEstimator

template<typename real_t, int nDim>
void TGradHSmoothEstimator<real_t, nDim>::EstimateDensity(
    TParticleArray<real_t, nDim>& particles, 
    const TSmoothingKernel<real_t, nDim>& smoothingKernel
) const {
    ForEach(particles, [&](Particle& a) {
        FindRoot(a.KernelWidth, [&]() {
            a.Density = {}; 
            a.DensityWidthDerivative = {};
            real_t aSearchWidth = smoothingKernel.GetRadius(a.KernelWidth);
            ForEachNeighbour(particles, a, aSearchWidth, [&](const Particle& b) {
                real_t abaKernelValue 
                    = smoothingKernel.GetValue(DeltaPosition(a,b), a.KernelWidth);
                real_t abaKernelWidthDerivative 
                    = smoothingKernel.GetRadiusDerivative(DeltaPosition(a,b), a.KernelWidth);
                a.Density += b.Mass*abaKernelValue;
                a.DensityWidthDerivative += b.Mass*abaKernelWidthDerivative;
            });
            real_t aExpectedDensity = a.Mass*std::pow(m_Coupling/a.KernelWidth, nDim);
            real_t aExpectedDensityWidthDerivative = -nDim*aExpectedDensity/a.KernelWidth;
            return std::make_pair(aExpectedDensity - a.Density,
                                  aExpectedDensityWidthDerivative - a.DensityWidthDerivative);
        });
        a.Pressure = EquationOfState_Pressure(a.Density, a.ThermalEnergy);
        a.SoundSpeed = EquationOfState_SpeedOfSound(a.Density, a.Pressure);
    });
}   // TGradHSmoothEstimator::EstimateDensity

template<typename real_t, int nDim>
void TGradHSmoothEstimator<real_t, nDim>::EstimateAcceleration(
    TParticleArray<real_t, nDim>& particles, 
    const TSmoothingKernel<real_t, nDim>& smoothingKernel,
    const TArtificialViscosity<real_t, nDim>& artificialViscosity
) const {
    ForEach(particles, [&](Particle& a) {
        a.Acceleration = {}; 
        a.Heating = {};
        real_t aOmega = real_t(1.0) 
            + a.KernelWidth*a.DensityWidthDerivative/(nDim*a.Density);
        real_t aSearchWidth = smoothingKernel.GetRadius(a.KernelWidth);
        ForEachNeighbour(particles, a,aSearchWidth, [&](const Particle& b) {
            real_t bOmega = real_t(1.0) 
                + b.KernelWidth*b.DensityWidthDerivative/(nDim*b.Density);
            real_t kinematicViscosity 
                = artificialViscosity.Value(a,b, Average(a.KernelWidth,b.KernelWidth));
            TVector<real_t, nDim> abaKernelGradient 
                = smoothingKernel.GetGradientValue(DeltaPosition(a,b), a.KernelWidth);
            TVector<real_t, nDim> abbKernelGradient 
                = smoothingKernel.GetGradientValue(DeltaPosition(a,b), b.KernelWidth);
            TVector<real_t, nDim> abAverageKernelGradient
                = Average(abaKernelGradient, abbKernelGradient);
            a.Acceleration -= (
                b.Mass*(a.Pressure/(aOmega*Square(a.Density))*abaKernelGradient 
                      + b.Pressure/(bOmega*Square(b.Density))*abbKernelGradient 
                                         + kinematicViscosity*abAverageKernelGradient));
            a.Heating += Dot(DeltaVelocity(a,b), 
                b.Mass*(a.Pressure/(aOmega*Square(a.Density))*abaKernelGradient 
                                         + kinematicViscosity*abAverageKernelGradient));
        });
    });
}   // TGradHSmoothEstimator::EstimateAcceleration

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#endif  // ifndef TIT_SMOOTH_ESTIMATOR_HPP_
