#include <algorithm>
#include <cassert>
#include <cmath>
#include <fstream>
#include <iostream>
#include <vector>

#include "TitParticle.hpp"
#include "tit/sph/equation_of_state.hpp"
#include "tit/sph/smooth_estimator.hpp"
#include "tit/sph/smooth_kernel.hpp"
#include "tit/sph/time_integrator.hpp"

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

int main() {
  using namespace tit;
  using namespace tit::sph;

  ParticleArray<double, 1> particles{};
  for (size_t i = 0; i < 1600; ++i) {
    particles.push_back({
        .fixed = i < 3,
        .mass = 1.0 / 1600,
        .width = 0.001,
        .thermal_energy = 1.0 / 0.4,
        .position = i / 1600.0,
        .alpha = 1.0,
    });
  }
  for (size_t i = 0; i < 200; ++i) {
    particles.push_back({
        .fixed = i >= (200 - 3),
        .mass = 1.0 / 1600,
        .width = 0.001,
        .thermal_energy = 0.1 / (0.4 * 0.125),
        .position = 1.0 + i / 200.0,
        .alpha = 1.0,
    });
  }

  GradHSmoothEstimator estimator{IdealGasEquationOfState{}, {}, {}};
  EulerIntegrator timeint{std::move(estimator)};

  for (size_t n = 0; n < 3 * 2500; ++n) {
    std::cout << n << std::endl;
    constexpr double dt = 0.00005;
    timeint.step(dt, particles);
  }

  particles.sort();
  particles.print("particles.txt");

  return 0;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
