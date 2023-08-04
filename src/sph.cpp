#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <vector>

#include "TitParticle.hpp"
#include "tit/sph/equation_of_state.hpp"
#include "tit/sph/kernel.hpp"
#include "tit/sph/smooth_estimator.hpp"
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

  const auto N = 100;
  const auto N_x = 4 * N;
  const auto N_y = 3 * N;
  const auto N_fixed = 4;
  const auto N_x_dam = 1 * N;
  const auto N_y_dam = 2 * N;

  const Real length = 1.0;
  const Real spacing = length / N;
  const Real timestep = 1.0e-5;
  const Real h_0 = 2.0 * spacing;
  const Real rho_0 = 1000.0;
  const Real m_0 = rho_0 * pow2(spacing) / //
                   /* 1162.056 * 1000.0*/  // cubic kernel
                   1252.71379884 * 1000.0  // wend quartic
      ;
  const Real cs_0 = 120.0;

  // Setup the SPH estimator:
  auto estimator = ClassicSmoothEstimator{
      // Weakly compressible equation of state.
      LinearWeaklyCompressibleFluidEquationOfState{cs_0, rho_0},
      // Standart quartic spline kernel.
      EighthOrderWendlandKernel{},
      // No artificial viscosity is required.
      DeltaSPHArtificialViscosity{cs_0, rho_0}};

  // Setup the time itegrator:
  auto timeint = EulerIntegrator{std::move(estimator)};

  // Setup the particles array:
  ParticleArray particles{
      // 2D space.
      Space<Real, 2>{},
      // Fields that are required by the estimator.
      timeint.required_fields,
      // Set of whole system constants.
      m, // Particle mass assumed constant.
      h, // Particle width assumed constant.
  };

  // Generate individual particles.
  auto num_fixed_particles = 0, num_fluid_particles = 0;
  for (auto i = -N_fixed; i < N_x + N_fixed; ++i) {
    for (auto j = -N_fixed; j < N_y; ++j) {
      const bool is_fixed = (i < 0 || i >= N_x) || (j < 0);
      const bool is_fluid = (i < N_x_dam) && (j < N_y_dam);
      if (is_fixed) ++num_fixed_particles;
      else if (is_fluid) ++num_fluid_particles;
      else continue;
      auto a = particles.append();
      fixed[a] = is_fixed;
      r[a] = spacing * Vec{i + 0.5, j + 0.5};
    }
  }
  std::cout << "Num. fixed particles: " << num_fixed_particles << std::endl;
  std::cout << "Num. fluid particles: " << num_fluid_particles << std::endl;

  // Set global particle variables.
  rho[particles] = rho_0;
  m[particles] = m_0;
  h[particles] = h_0;
  if constexpr (has<decltype(particles)>(alpha)) {
    alpha[particles] = 1.0;
  }

  // Setup the particle adjacency structure.
  auto adjacent_particles = ParticleAdjacency{particles};

  particles.print("particles-dam.csv");

  real_t time = 0.0, exectime = 0.0, printtime = 0.0;
  for (size_t n = 0; time <= 2.7; ++n, time += timestep) {
    std::cout << n << "\t\t" << time << "\t\t" << exectime / n << "\t\t"
              << printtime / (n / 200) << std::endl;
    auto start = std::chrono::high_resolution_clock::now();
    timeint.step(timestep, particles, adjacent_particles);
    auto delta = std::chrono::high_resolution_clock::now() - start;
    exectime +=
        1.0e-9 *
        std::chrono::duration_cast<std::chrono::nanoseconds>(delta).count();
    if (n % 200 == 0 && n != 0) {
      start = std::chrono::high_resolution_clock::now();
      particles.print("particles-dam.csv");
      auto delta = std::chrono::high_resolution_clock::now() - start;
      printtime +=
          1.0e-9 *
          std::chrono::duration_cast<std::chrono::nanoseconds>(delta).count();
    }
  }

  particles.print("particles-dam.csv");

  std::cout << "Total time: " << (exectime + printtime) / 60.0 << std::endl;

  return 0;
}

#endif

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

int main() {
  return sph_main<tit::real_t>();
}
