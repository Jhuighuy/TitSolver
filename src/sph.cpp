#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <vector>
// #include <execution>
#include <cassert>

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

#include "TitParticle.hpp"
#include "tit/sph/smooting_kernel.hpp"
using namespace tit;
using namespace tit::sph;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

double Kappa = 1.0, Gamma = 1.4;

template<class Real>
class EquationOfState {
private:

  Real _gamma = Real{1.4};

public:

  constexpr Real pressure(Real density, Real thermal_energy) const {
    return (_gamma - 1.0) * density * thermal_energy;
  }

  constexpr Real sound_speed(Real density, Real pressure) const {
    return sqrt(_gamma * pressure / density);
  }

}; // class EquationOfState

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#include "tit/sph/smooth_estimator.hpp"

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/**
 ** @brief Estimate Density and Pressure.
 **/
template<class Real, dim_t Dim, class ParticleEstimator,
         class ArtificialViscosity>
void LeapfrogStep(Particle<Real, Dim>* particles,
                  Particle<Real, Dim>* particlesTemp, size_t numParticles,
                  const SmoothingKernel<Real, Dim>& smoothingKernel,
                  const ParticleEstimator& estimator,
                  const ArtificialViscosity& artificialViscosity) {
  const double courantFactor = 0.1;
  double timeStep = courantFactor * 0.005, halfTimeStep = 0.5 * timeStep;
#if 1
  EquationOfState<Real> eos{};
  ParticleArray<Real, Dim> Particles{particles, numParticles};
  Particles.SortParticles();
  estimator.estimate_density(Particles, smoothingKernel, eos);
  estimator.estimate_acceleration(Particles, smoothingKernel,
                                  artificialViscosity);
  std::for_each( // std::execution::par_unseq,
      particles, particles + numParticles, [&](Particle<Real, Dim>& iParticle) {
        /// @todo Boundary conditions!
        if ((&iParticle - particles) <= 100) return;
        if (numParticles - (&iParticle - particles) <= 10) return;
        iParticle.velocity += timeStep * iParticle.acceleration;
        iParticle.position += timeStep * iParticle.velocity;
        iParticle.thermal_energy += timeStep * iParticle.thermal_energy_deriv;
      });
#else
  SortParticles(particles, numParticles);
  estimator.estimate_density(particles, numParticles, smoothingKernel);
  estimator.estimate_acceleration(particles, numParticles, smoothingKernel,
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
  estimator.estimate_acceleration(particlesTemp, numParticles, smoothingKernel,
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
    particle.mass = 1.0 / 1600;
    particle.width = 0.001;
    particle.thermal_energy = 1.0 / 0.4;
    particle.position = i / 1600.0;
    particles.push_back(particle);
  }
  for (size_t i = 0; i < 200; ++i) {
    Particle particle;
    particle.mass = 1.0 / 1600;
    particle.width = 0.001;
    particle.thermal_energy = 0.1 / (0.4 * 0.125);
    particle.position = 1.0 + i / 200.0;
    particles.push_back(particle);
  }
  for (size_t n = 0; n < 3 * 250; ++n) {
    std::cout << n << std::endl;
    // ClassicSmoothEstimator<double, 1> estimator{0.005};
    GradHSmoothEstimator<double, 1> estimator{};
    QuinticSmoothingKernel<double, 1> smoothingKernel;
    AlphaBetaArtificialViscosity<double, 1> artificialViscosity;
    std::vector<Particle> particlesNew(particles);
    LeapfrogStep(particles.data(), particlesNew.data(), particles.size(),
                 smoothingKernel, estimator, artificialViscosity);
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
