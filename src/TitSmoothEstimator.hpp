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
template<typename Real, int Dim>
class TSmoothEstimator 
{
public:
    /** Estimate density, kernel width, pressure and sound speed. */
    virtual void EstimateDensity(
        TParticleArray<Real, Dim>& particles, 
        const TSmoothingKernel<Real, Dim>& smoothingKernel) const = 0;

    /** Estimate acceleration and thermal heating. */
    virtual void EstimateAcceleration(
        TParticleArray<Real, Dim>& particles, 
        const TSmoothingKernel<Real, Dim>& smoothingKernel,
        const TArtificialViscosity<Real, Dim>& artificialViscosity) const = 0;
};  // class TSmoothEstimator

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/********************************************************************
 ** The particle estimator with a fixed kernel width.
 ********************************************************************/
template<typename Real, int Dim>
class TClassicSmoothEstimator final : public TSmoothEstimator<Real, Dim> 
{
public:
    Real m_KernelWidth;
    TClassicSmoothEstimator(Real w) : m_KernelWidth(w) {}

public:
    /** Estimate density, kernel width, pressure and sound speed. */
    void EstimateDensity(
        TParticleArray<Real, Dim>& particles, 
        const TSmoothingKernel<Real, Dim>& smoothingKernel) const final;

    /** Estimate acceleration and thermal heating. */
    void EstimateAcceleration(
        TParticleArray<Real, Dim>& particles, 
        const TSmoothingKernel<Real, Dim>& smoothingKernel,
        const TArtificialViscosity<Real, Dim>& artificialViscosity) const final;
};  // class TClassicSmoothEstimator

template<typename Real, int Dim>
void TClassicSmoothEstimator<Real, Dim>::EstimateDensity(
    TParticleArray<Real, Dim>& particles, 
    const TSmoothingKernel<Real, Dim>& smoothingKernel) const 
{
    Real fixedSearchWidth = smoothingKernel.GetRadius(m_KernelWidth);
    ForEach(particles, [&](Particle& a) 
    {
        a.Density = Real(0.0);
        ForEachNeighbour(particles, a, fixedSearchWidth, [&](const Particle& b) 
        {
            Real abKernelValue = smoothingKernel.GetValue(DeltaPosition(a,b), m_KernelWidth);
            a.Density += b.Mass*abKernelValue;
        });
        a.Pressure = EquationOfState_Pressure(a.Density, a.InternalEnergy);
        a.SoundSpeed = EquationOfState_SpeedOfSound(a.Density, a.Pressure);
    });
}

template<typename Real, int Dim>
void TClassicSmoothEstimator<Real, Dim>::EstimateAcceleration(
    TParticleArray<Real, Dim>& particles, 
    const TSmoothingKernel<Real, Dim>& smoothingKernel,
    const TArtificialViscosity<Real, Dim>& artificialViscosity) const 
{
    Real fixedSearchWidth = smoothingKernel.GetRadius(m_KernelWidth);
    ForEach(particles, [&](Particle& a) 
    {
        a.VelocityDerivative = Real(0.0);
        a.InternalEnergyDerivative = Real(0.0);
        ForEachNeighbour(particles, a, fixedSearchWidth, [&](const Particle& b) 
        {
            Real kinematicViscosity = artificialViscosity.Value(a, b, m_KernelWidth);
            Vector<Real, Dim> abKernelGradient =
                smoothingKernel.GetGradientValue(DeltaPosition(a, b), m_KernelWidth);
            a.VelocityDerivative -= b.Mass*(
                a.Pressure/Pow2(a.Density) + 
                b.Pressure/Pow2(b.Density) + kinematicViscosity)*abKernelGradient;
            a.InternalEnergyDerivative += b.Mass*(
                a.Pressure/Pow2(a.Density) + kinematicViscosity)*Dot(DeltaVelocity(a, b),abKernelGradient);
        });
    });
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/********************************************************************
 ** The particle estimator with a fixed kernel width.
 ********************************************************************/
template<typename Real, int Dim>
class TGradHSmoothEstimator final : public TSmoothEstimator<Real, Dim> 
{
public:
    const Real m_Coupling = 1.55;

    /** Estimate density, kernel width, pressure and sound speed. */
    void EstimateDensity(
        TParticleArray<Real, Dim>& particles, 
        const TSmoothingKernel<Real, Dim>& smoothingKernel) const final;

    /** Estimate acceleration and thermal heating. */
    void EstimateAcceleration(
        TParticleArray<Real, Dim>& particles, 
        const TSmoothingKernel<Real, Dim>& smoothingKernel,
        const TArtificialViscosity<Real, Dim>& artificialViscosity) const final;
};  // class TGradHSmoothEstimator

template<typename Real, int Dim>
void TGradHSmoothEstimator<Real, Dim>::EstimateDensity(
    TParticleArray<Real, Dim>& particles, 
    const TSmoothingKernel<Real, Dim>& smoothingKernel) const 
{
    ForEach(particles, [&](Particle& a) 
    {
        FindRoot(a.KernelWidth, [&]() 
        {
            a.Density = Real(0.0); 
            a.DensityWidthDerivative = Real(0.0);
            Real aSearchWidth = smoothingKernel.GetRadius(a.KernelWidth);
            ForEachNeighbour(particles, a, aSearchWidth, [&](const Particle& b) 
            {
                Real abaKernelValue = 
                    smoothingKernel.GetValue(DeltaPosition(a,b), a.KernelWidth);
                Real abaKernelWidthDerivative = 
                    smoothingKernel.GetRadiusDerivative(DeltaPosition(a,b), a.KernelWidth);
                a.Density += b.Mass*abaKernelValue;
                a.DensityWidthDerivative += b.Mass*abaKernelWidthDerivative;
            });
            Real aExpectedDensity = a.Mass*Pow(m_Coupling/a.KernelWidth, Dim);
            Real aExpectedDensityWidthDerivative = -Dim*aExpectedDensity/a.KernelWidth;
            return MakePair(
                aExpectedDensity - a.Density,
                aExpectedDensityWidthDerivative - a.DensityWidthDerivative
            );
        });
        a.Pressure = EquationOfState_Pressure(a.Density, a.InternalEnergy);
        a.SoundSpeed = EquationOfState_SpeedOfSound(a.Density, a.Pressure);
    });
}

template<typename Real, int Dim>
void TGradHSmoothEstimator<Real, Dim>::EstimateAcceleration(
    TParticleArray<Real, Dim>& particles, 
    const TSmoothingKernel<Real, Dim>& smoothingKernel,
    const TArtificialViscosity<Real, Dim>& artificialViscosity) const 
{
    ForEach(particles, [&](Particle& a) 
    {
        a.VelocityDerivative = Real(0.0);
        a.InternalEnergyDerivative = Real(0.0);
        Real aOmega = Real(1.0) + a.KernelWidth*a.DensityWidthDerivative/(Dim*a.Density);
        Real aSearchWidth = smoothingKernel.GetRadius(a.KernelWidth);
        ForEachNeighbour(particles, a, aSearchWidth, [&](const Particle& b) 
        {
            Real bOmega = Real(1.0) + b.KernelWidth*b.DensityWidthDerivative/(Dim*b.Density);
            Real kinematicViscosity = 
                artificialViscosity.Value(a, b, Average(a.KernelWidth,b.KernelWidth));
            Vector<Real, Dim> abaKernelGradient = 
                smoothingKernel.GetGradientValue(DeltaPosition(a, b), a.KernelWidth);
            Vector<Real, Dim> abbKernelGradient = 
                smoothingKernel.GetGradientValue(DeltaPosition(a, b), b.KernelWidth);
            Vector<Real, Dim> abAverageKernelGradient = Average(abaKernelGradient, abbKernelGradient);
            a.VelocityDerivative -= b.Mass*(
                a.Pressure/(aOmega*Pow2(a.Density))*abaKernelGradient +
                b.Pressure/(bOmega*Pow2(b.Density))*abbKernelGradient + kinematicViscosity*abAverageKernelGradient);
            a.InternalEnergyDerivative += Dot(DeltaVelocity(a,b), b.Mass*(
                a.Pressure/(aOmega*Pow2(a.Density))*abaKernelGradient + kinematicViscosity*abAverageKernelGradient));
        });
    });
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#endif  // ifndef TIT_SMOOTH_ESTIMATOR_HPP_
