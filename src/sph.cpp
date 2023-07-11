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

  for (size_t n = 0; n < 3 * 2500; ++n) {
    std::cout << n << std::endl;
    constexpr double dt = 0.00005;
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

  const dim_t N = 100;
  const real_t N_x = 4 * N;
  const real_t N_y = 3 * N;
  const real_t N_fixed = 3;
  const real_t N_x_dam = 1 * N;
  const real_t N_y_dam = 2 * N;

  const real_t L = 1.0;
  const real_t spacing = L / N;
  const real_t timestep = 5.0e-5;
  const real_t h_0 = 1.5 * spacing;
  const real_t rho_0 = 1000.0;
  const real_t cs_0 = 120.0;

  GradHSmoothEstimator estimator{
      WeaklyCompressibleFluidEquationOfState{cs_0, rho_0}, QuarticKernel{},
      AlphaBetaArtificialViscosity{}};
  EulerIntegrator timeint{std::move(estimator)};

  using Vars = required_variables_t<decltype(estimator)>;
  ParticleArray<real_t, 2, Vars> particles{};
  for (dim_t i = -N_fixed; i < N_x + N_fixed; ++i) {
    for (dim_t j = -N_fixed; j < N_y; ++j) {
      const bool is_fixed = (i < 0 || i >= N_x) || (j < 0);
      const bool is_water = (i < N_x_dam) && (j < N_y_dam);
      if (!(is_fixed || is_water)) continue;
      particles.append([&]<class A>(A a) {
        using namespace particle_variables;
        fixed[a] = is_fixed;
        rho[a] = rho_0;
        m[a] = rho_0 / (N_x_dam * N_y_dam) * 2;
        h[a] = h_0;
        r[a][0] = i * spacing;
        r[a][1] = j * spacing;
        if constexpr (has<A>(alpha)) alpha[a] = 1.0;
      });
    }
  }

  particles.print("particles-dam.csv");

  for (size_t n = 0; n < 100000; ++n) {
    std::cout << n << std::endl;
    timeint.step(timestep, particles);
    if (n % 100 == 0) particles.print("particles-dam.csv");
  }

  particles.print("particles-dam.csv");

  return 0;
}

#endif

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
