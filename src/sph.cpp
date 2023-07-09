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

#if 1

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
      fixed[a] = i < 10;
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
      fixed[b] = i >= (200 - 1 - 10);
      rho[b] = 0.125;
      m[b] = 1.0 / 1600;
      h[b] = 0.001;
      r[b] = 1.0 + i / 200.0;
      if constexpr (has<B>(eps)) eps[b] = 0.1 / (0.4 * 0.125);
      if constexpr (has<B>(alpha)) alpha[b] = 1.0;
    });
  }

  for (size_t n = 0; n < 3 * 2500; ++n) {
    std::cout << n << std::endl;
    constexpr double dt = 0.00005;
    timeint.step(dt, particles);
  }

  particles.sort();
  particles.print("particles.txt");

  return 0;
}

#endif

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#if 0

int main() {
  using namespace tit;
  using namespace tit::sph;

  GradHSmoothEstimator estimator{
      WeaklyCompressibleFluidEquationOfState{1.0, 1.0},
      {},
      {}};
  EulerIntegrator timeint{std::move(estimator)};

  using Vars = required_variables_t<decltype(estimator)>;
  ParticleArray<double, 2, Vars> particles{};
  for (size_t i = 0; i < 100; ++i) {
    for (size_t j = 0; j < 100; ++j) {
      particles.append([&]<class A>(A a) {
        using namespace particle_variables;
        fixed[a] = (i < 3 || i >= 97) || (j < 3 || j >= 97);
        m[a] = 1.0 / 10000;
        rho[a] = 1.0;
        h[a] = 0.01;
        r[a][0] = i / 100.0;
        r[a][1] = j / 100.0;
        if (j >= 97) v[a][0] = 0.1;
        if constexpr (has<A>(alpha)) alpha[a] = 1.0;
      });
    }
  }

  for (size_t n = 0; n < 100000; ++n) {
    std::cout << n << std::endl;
    constexpr double dt = 0.0005;
    timeint.step(dt, particles);
    if (n % 10 == 0) particles.print("particles.csv");
  }

  particles.print("particles.csv");

  return 0;
}

#endif

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
