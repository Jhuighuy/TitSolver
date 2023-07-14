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
      using namespace particle_fields;
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
    using namespace particle_fields;
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

template<class Real>
int sph_main() {
  using namespace tit;
  using namespace sph;
  using namespace particle_fields;

  const dim_t N = 100;
  const dim_t N_x = 4 * N;
  const dim_t N_y = 3 * N;
  const dim_t N_fixed = 3;
  const dim_t N_x_dam = 1 * N;
  const dim_t N_y_dam = 2 * N;

  const Real L = 1.0;
  const Real spacing = L / N;
  const Real timestep = 5.0e-5;
  const Real h_0 = /*1.5 */ spacing;
  const Real rho_0 = 1000.0;
  const Real cs_0 = 120.0;

  // Setup the SPH estimator:
  GradHSmoothEstimator estimator{
      // Weakly compressible equation of state.
      WeaklyCompressibleFluidEquationOfState{cs_0, rho_0},
      // Standart cubic spline kernel.
      CubicKernel{},
      // Standard alpha-beta artificial viscosity scheme.
      AlphaBetaArtificialViscosity{}};

  // Setup the time itegrator:
  EulerIntegrator timeint{std::move(estimator)};

  // Setup the particles array:
  ParticleArray particles{
      // 2D space.
      Space<Real, 2>{},
      // Fields that are required by the estimator.
      estimator.required_fields,
      // Set of whole system constants.
      m,    // Particle mass assumed constant.
      /*h*/ // Particle width assumed constant.
  };

  // Generate individual particles.
  for (dim_t i = -N_fixed; i < N_x + N_fixed; ++i) {
    for (dim_t j = -N_fixed; j < N_y; ++j) {
      const bool is_fixed = (i < 0 || i >= N_x) || (j < 0);
      const bool is_water = (i < N_x_dam) && (j < N_y_dam);
      if (!(is_fixed || is_water)) continue;
      auto a = particles.append();
      fixed[a] = is_fixed;
      r[a][0] = i * spacing;
      r[a][1] = j * spacing;
    }
  }
  // Set global particle variables.
  rho[particles] = rho_0;
  m[particles] = rho_0 / (N_x_dam * N_y_dam) * 2;
  h[particles] = h_0;
  if constexpr (has<decltype(particles)>(alpha)) {
    alpha[particles] = 1.0;
  }

  // ParticleSpatialIndex spatial_index{particles};

  particles.print("particles-dam.csv");

  for (size_t n = 0; n < 1 + 100000; ++n) {
    std::cout << n << std::endl;
    timeint.step(timestep, particles);
    if (n % 100 == 0) particles.print("particles-dam.csv");
  }

  particles.print("particles-dam.csv");

  return 0;
}

#endif

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

int main() {
  return sph_main<double>();
}
