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

#if 0

int main() {
  using namespace tit;
  using namespace tit::sph;

  GradHSmoothEstimator estimator{IdealGasEquationOfState{}, {}, {}};
  EulerIntegrator timeint{std::move(estimator)};

  using Vars = required_variables_t<decltype(estimator)>;
  ParticleArray<double, 1, Vars> particles{};
  for (size_t i = 0; i < 1600; ++i) {
    particles.append([&]<class A>(A a) {
      using namespace particle_variables;
      fixed[a] = i < 3;
      m[a] = 1.0 / 1600;
      rho[a] = 1.0;
      h[a] = 0.001;
      r[a] = i / 1600.0;
      if constexpr (has<A>(eps)) eps[a] = 1.0 / 0.4;
      if constexpr (has<A>(alpha)) alpha[a] = 1.0;
    });
  }
  for (size_t i = 0; i < 200; ++i) {
    using namespace particle_variables;
    particles.append([&]<class B>(B b) {
      fixed[b] = i >= (200 - 1 - 3);
      rho[b] = 0.125;
      m[b] = 1.0 / 1600;
      h[b] = 0.001;
      r[b] = 1.0 + i / 200.0;
      if constexpr (has<B>(eps)) eps[b] = 0.1 / (0.4 * 0.125);
      if constexpr (has<B>(alpha)) alpha[b] = 1.0;
    });
  }

  for (size_t n = 0; n < 3 * 3000; ++n) {
    std::cout << n << std::endl;
    constexpr double dt = 0.00005 * 2500 / 3000;
    timeint.step(dt, particles);
  }

  particles.sort();
  particles.print("particles-sod.csv");

  return 0;
}

#endif

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#if 1

int main() {
  using namespace tit;
  using namespace tit::sph;

  GradHSmoothEstimator estimator{
      WeaklyCompressibleFluidEquationOfState{10.0, 1.0},
      {},
      {}};
  EulerIntegrator timeint{std::move(estimator)};

  using Vars = required_variables_t<decltype(estimator)>;
  ParticleArray<double, 2, Vars> particles{};
  for (size_t i = 0; i < 100; ++i) {
    for (size_t j = 0; j < 100; ++j) {
      const bool is_fixed = (i < 3 || i >= 97) || (j < 3 || j >= 97);
      const bool is_water = (i < 40) && (j < 80);
      if (!(is_fixed || is_water)) continue;
      particles.append([&]<class A>(A a) {
        using namespace particle_variables;
        fixed[a] = is_fixed;
        m[a] = 1.0 / (3 * 40 * 80);
        rho[a] = 1.0;
        h[a] = 0.01;
        r[a][0] = i / 100.0;
        r[a][1] = j / 100.0;
        if constexpr (has<A>(alpha)) alpha[a] = 1.0;
      });
    }
  }

  for (size_t n = 0; n < 2000; ++n) {
    std::cout << n << std::endl;
    constexpr double dt = 0.00025;
    timeint.step(dt, particles);
    if (n % 10 == 0) particles.print("particles-dam.csv");
  }

  particles.print("particles-dam.csv");

  return 0;
}

#endif

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
