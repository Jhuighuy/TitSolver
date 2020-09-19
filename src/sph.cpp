#include <cmath>
#include <vector>
#include <algorithm>
#include <fstream>
#include <iostream>
//#include <execution>
#include <cassert>

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

#include "TitParticle.hpp"
#include "TitSmoothingKernels.hpp"
#include "TitRootFinder.hpp"
#include "TitParticleTree.hpp"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

double Kappa = 1.0, Gamma = 1.4;

template<typename real_t>
class TEquationOfState {
public:
};  // class EquationOfState

template<typename real_t>
inline real_t EquationOfState_Pressure(real_t density, real_t thermalEnergy) {
    //return Kappa*std::pow(density, Gamma);
    return (Gamma - 1.0)*density*thermalEnergy;
}
template<typename real_t>
inline real_t EquationOfState_SpeedOfSound(real_t density, real_t pressure) {
    return std::sqrt(Gamma*pressure/density);
}

#include "TitSmoothEstimator.hpp"

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/


/** 
 ** @brief Estimate Density and Pressure.
 **/
template<typename real_t, int nDim,
         typename TParticleEstimator, typename TArtificialViscosity>
void LeapfrogStep(TParticle<real_t>* particles, 
                  TParticle<real_t>* particlesTemp, size_t numParticles,
            const TSmoothingKernel<real_t, nDim>& smoothingKernel,
            const TParticleEstimator& estimator,
            const TArtificialViscosity& artificialViscosity) {
    const double courantFactor = 0.1;
    double timeStep = courantFactor*0.005,//estimator.m_KernelWidth, 
           halfTimeStep = 0.5*timeStep;
#if 1
    TParticleArray<real_t, nDim> Particles{particles, numParticles};
    Particles.SortParticles();
    estimator.EstimateDensity(Particles, smoothingKernel);
    estimator.EstimateAcceleration(Particles, smoothingKernel, artificialViscosity);
    std::for_each(//std::execution::par_unseq,
                  particles, particles+numParticles, [&](Particle& iParticle) {
        /// @todo Boundary conditions!
        if ((&iParticle - particles) <= 100) return;
        if (numParticles-(&iParticle - particles) <= 10) return;
        iParticle.Velocity += timeStep*iParticle.Acceleration;
        iParticle.Position += timeStep*iParticle.Velocity;
        iParticle.ThermalEnergy += timeStep*iParticle.Heating;
    });
#else
    SortParticles(particles, numParticles);
    estimator.EstimateDensity(particles, numParticles, smoothingKernel);
    estimator.EstimateAcceleration(particles, numParticles, smoothingKernel, artificialViscosity);
    std::for_each_n(std::execution::par_unseq,
                    particles, numParticles, [&](const Particle& iParticle) {
        /// @todo Boundary conditions!
        if ((&iParticle - particles) <= 100) return;
        Particle& iParticleTemp = *(particlesTemp + (&iParticle - particles));
        // Extrapolate position and velocity.
        iParticleTemp.Position = iParticle.Position + halfTimeStep*iParticle.Velocity;
        iParticleTemp.Velocity = iParticle.Velocity + halfTimeStep*iParticle.Acceleration;
        //iParticleTemp.ThermalEnergy = iParticle.ThermalEnergy + halfTimeStep*iParticle.Heating;
        /// @todo Extrapolate hydrodynamical properties (... what?).
    });
    SortParticles(particlesTemp, numParticles);
    estimator.EstimateDensity(particlesTemp, numParticles, smoothingKernel);
    estimator.EstimateAcceleration(particlesTemp, numParticles, smoothingKernel, artificialViscosity);
    std::for_each_n(std::execution::par_unseq,
                    particles, numParticles, [&](Particle& iParticle) {
        /// @todo Boundary conditions!
        if ((&iParticle - particles) <= 100) return;
        if (numParticles-(&iParticle - particles) <= 10) return;
        Particle& iParticleTemp = *(particlesTemp + (&iParticle - particles));
        // Update position and velocity.
        iParticleTemp.Velocity = iParticle.Velocity;
        iParticle.Velocity = iParticleTemp.Velocity + timeStep*iParticleTemp.Acceleration;
        iParticle.Position += halfTimeStep*(iParticle.Velocity + iParticleTemp.Velocity);
        iParticle.ThermalEnergy += timeStep*iParticle.Heating;
    });
#endif
}   // LeapfrogStep

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

int main() {

    //std::cout << FindRoot(0.0, 1.5, 
    //    [](double x) { return 4*x-exp(x); }) << std::endl;
    //return 0;

    std::vector<Particle> particles;
    for (size_t i = 0; i < 1600; ++i) {
        Particle particle;
        particle.Mass = 1.0/1600;
        particle.KernelWidth = 0.001;
        particle.ThermalEnergy = 1.0/0.4;
        particle.Position = i/1600.0;
        particles.push_back(particle); 
    }
    for (size_t i = 0; i < 200; ++i) {
        Particle particle;
        particle.Mass = 1.0/1600; 
        particle.KernelWidth = 0.001;  
        particle.ThermalEnergy = 0.1/(0.4*0.125);
        particle.Position = 1.0 + i/200.0;
        particles.push_back(particle);
    }
    for (size_t n = 0; n < 3*250; ++n) {
        TGradHSmoothEstimator<double, 1> estimator;
        TQuinticSmoothingKernel<double, 1> smoothingKernel;
        TArtificialViscosity_AlphaBeta<double, 1> artificialViscosity;
        std::vector<Particle> particlesNew(particles);
        LeapfrogStep(particles.data(), particlesNew.data(), particles.size(), 
                     smoothingKernel, estimator, artificialViscosity);
    }
    std::ofstream output("particles.txt");
    std::sort(particles.begin(), particles.end(), [&](const Particle& p, const Particle& q) {
        return Less(p.Position, q.Position);
    }); 
    for (const Particle& particle : particles) {
        output << particle.Position << " " << particle.Density << " ";
        output << particle.Velocity << " ";
        output << particle.Pressure << " ";
        output << particle.ThermalEnergy << " ";
        output << particle.KernelWidth << " ";
        output << std::endl;
    }
    return 0;
}
