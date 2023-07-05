#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <vector>
// #include <execution>
#include <cassert>

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#include "TitParticle.hpp"
#include "tit/sph/smooth_kernel.hpp"

using namespace tit;
using namespace tit::sph;

class GasEquationOfState {
private:

  real_t _gamma = 1.4;

public:

  template<class Real>
  constexpr Real pressure(Real density, Real thermal_energy) const {
    return (_gamma - 1.0) * density * thermal_energy;
  }

  template<class Real>
  constexpr Real sound_speed(Real density, Real pressure) const {
    return sqrt(_gamma * pressure / density);
  }

}; // class EquationOfState

#include "tit/sph/smooth_estimator.hpp"
#include "tit/sph/time_integrator.hpp"

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

template<class Real, dim_t Dim, class TimeIntegrator>
void LeapfrogStep(Particle<Real, Dim>* particles, size_t numParticles,
                  const TimeIntegrator& timeint) {
  const double courantFactor = 0.1;
  double timeStep = courantFactor * 0.0005, halfTimeStep = 0.5 * timeStep;
  ParticleArray<Real, Dim> Particles{particles, numParticles};
#if 1
  timeint.integrate(timeStep, Particles);
#elif 0
  SortParticles(particles, numParticles);
  estimator.estimate_density(particles, numParticles, smoothingKernel);
  estimator.estimate_forces(particles, numParticles, smoothingKernel,
                            artificialViscosity);
  std::for_each_n(
      std::execution::par_unseq, particles, numParticles,
      [&](const Particle& iParticle) {
        /// @todo Boundary conditions!
        if ((&iParticle - particles) <= 100) return;
        Particle& iParticleTemp = *(particlesTemp + (&iParticle - particles));
        // Extrapolate position and velocity.
        iParticleTemp.position =
            iParticle.position + halfTimeStep * iParticle.velocity;
        iParticleTemp.velocity =
            iParticle.velocity + halfTimeStep * iParticle.acceleration;
        // iParticleTemp.ThermalEnergy = iParticle.ThermalEnergy +
        // halfTimeStep*iParticle.Heating;
        /// @todo Extrapolate hydrodynamical properties (... what?).
      });
  SortParticles(particlesTemp, numParticles);
  estimator.estimate_density(particlesTemp, numParticles, smoothingKernel);
  estimator.estimate_forces(particlesTemp, numParticles, smoothingKernel,
                            artificialViscosity);
  std::for_each_n(
      std::execution::par_unseq, particles, numParticles,
      [&](Particle& iParticle) {
        /// @todo Boundary conditions!
        if ((&iParticle - particles) <= 100) return;
        if (numParticles - (&iParticle - particles) <= 10) return;
        Particle& iParticleTemp = *(particlesTemp + (&iParticle - particles));
        // Update position and velocity.
        iParticleTemp.velocity = iParticle.velocity;
        iParticle.velocity =
            iParticleTemp.velocity + timeStep * iParticleTemp.acceleration;
        iParticle.position +=
            halfTimeStep * (iParticle.velocity + iParticleTemp.velocity);
        iParticle.ThermalEnergy += timeStep * iParticle.Heating;
      });
#endif
} // LeapfrogStep

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

int main() {
  using Particle = ::Particle<double, 1>;

  std::vector<Particle> particles;
  for (size_t i = 0; i < 1600; ++i) {
    Particle particle;
    memset(&particle, 0, sizeof(particle));
    particle.fixed = i <= 100;
    particle.mass = 1.0 / 1600;
    particle.width = 0.001;
    particle.thermal_energy = 1.0 / 0.4;
    particle.position = i / 1600.0;
    particle.alpha = 0.1;
    particles.push_back(particle);
  }
  for (size_t i = 0; i < 200; ++i) {
    Particle particle;
    memset(&particle, 0, sizeof(particle));
    particle.fixed = i >= 190;
    particle.mass = 1.0 / 1600;
    particle.width = 0.001;
    particle.thermal_energy = 0.1 / (0.4 * 0.125);
    particle.alpha = 0.1;
    particle.position = 1.0 + i / 200.0;
    particles.push_back(particle);
  }

  GradHSmoothEstimator estimator{};
  EulerIntegrator timeint{std::move(estimator)};
  for (size_t n = 0; n < 3 * 2500; ++n) {
    std::cout << n << std::endl;
    LeapfrogStep(particles.data(), particles.size(), timeint);
  }

  std::ofstream output("particles.txt");
  std::sort(particles.begin(), particles.end(),
            [&](const Particle& p, const Particle& q) {
              return p.position < q.position;
            });
  for (const Particle& particle : particles) {
    output << particle.position << " " << particle.density << " ";
    output << particle.velocity << " ";
    output << particle.pressure << " ";
    output << particle.thermal_energy_deriv << " ";
    output << particle.width << " ";
    output << std::endl;
  }
  return 0;
}
