#include <algorithm>
#include <cassert>
#include <cmath>
#include <format>
#include <fstream>
#include <vector>

#define COMPRESSIBLE_SOD_PROBLEM 0
#define HARD_DAM_BREAKING 0
#define EASY_DAM_BREAKING 1
#define INITIALLY_SQUARE_PATCH 0
#define WITH_GODUNOV 0
#define WITH_WALLS (HARD_DAM_BREAKING || EASY_DAM_BREAKING)
#define WITH_GRAVITY (HARD_DAM_BREAKING || EASY_DAM_BREAKING)

#include "tit/core/io.hpp"
#include "tit/core/main_func.hpp"
#include "tit/core/time.hpp"

#include "tit/sph/equation_of_state.hpp"
#include "tit/sph/fluid_equations.hpp"
#include "tit/sph/kernel.hpp"
#include "tit/sph/particle_array.hpp"
#include "tit/sph/particle_array_io.hpp"
#include "tit/sph/time_integrator.hpp"

#if 0

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#elif COMPRESSIBLE_SOD_PROBLEM

template<class Real>
int sph_main(int /*argc*/, char** /*argv*/) {
  using namespace tit;
  using namespace tit::sph;

  auto equations = CompressibleFluidEquations{
      // Ideal gas equation of state.
      IdealGasEquationOfState{},
      // Use basic summation density.
      GradHSummationDensity{},
      // Cubic spline kernel.
      GaussianKernel{},
      // Use Rosswog's artificial viscosity switch with Balsara's limiter.
      AlphaBetaArtificialViscosity{}};
  EulerIntegrator timeint{std::move(equations), 1};

  ParticleArray particles{
      // 1D space.
      Space<Real, 1>{},
      // Fields that are required by the equations.
      timeint.required_fields,
  };
  for (int i = -10; i < 1600; ++i) {
    auto a = particles.append();
    fixed[a] = i < 0;
    m[a] = 1.0 / 1600;
    rho[a] = 1.0;
    h[a] = 0.015;
    r[a] = (i + 0.5) / 1600.0;
    u[a] = 1.0 / (5.0 / 3.0 - 1.0);
  }
  for (size_t i = 0; i < 200 + 10; ++i) {
    auto b = particles.append();
    fixed[b] = i >= 200;
    rho[b] = 0.125;
    m[b] = 1.0 / 1600;
    h[b] = 0.015;
    r[b] = 1.0 + (i + 0.5) / 200.0;
    u[b] = 0.1 / ((5.0 / 3.0 - 1.0) * 0.125);
  }

  // Setup the particle adjacency structure.
  auto adjacent_particles = ParticleAdjacency{particles, geom::KDTreeFactory{}};

  for (size_t n = 0; n < 3 * 2500; ++n) {
    println("{:>15}", n);
    constexpr double dt = 0.00005;
    timeint.step(dt, particles, adjacent_particles);
  }

  // particles.sort();
  particles.print("particles-sod.csv");

  return 0;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#elif HARD_DAM_BREAKING

template<class Real>
int sph_main(int /*argc*/, char** /*argv*/) {
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
  const Real timestep = 5.0e-5;
  const Real h_0 = 1.5 * spacing;
  const Real rho_0 = 1000.0;
  const Real m_0 = rho_0 * pow2(spacing) / 2;
  const Real cs_0 = 120.0;
  const Real mu_0 = 1.0e-3;

  // Setup the SPH equations:
  auto equations = FluidEquations{
      // Weakly compressible equation of state.
      WeaklyCompressibleFluidEquationOfState{cs_0, rho_0},
      // Continuity equation instead of density summation.
      ContinuityEquation{},
      // Quartic spline kernel.
      QuarticSplineKernel{},
      // No artificial viscosity is used.
      // NoArtificialViscosity{}
      // For now, we use alpha-beta viscosity. This is due to the fact that
      // D. Violeau uses k-epsilon turbulence as some sort of stabilizer.
      // and we do not have any turbulence at the moment.
      AlphaBetaArtificialViscosity{0.1, 0.0}};

  // Setup the time integrator:
  auto timeint = EulerIntegrator{std::move(equations)};
  // auto timeint = RungeKuttaIntegrator{std::move(equations)};

  // Setup the particles array:
  ParticleArray particles{
      // 2D space.
      Space<Real, 2>{},
      // Fields that are required by the equations.
      timeint.required_fields,
      // Set of whole system constants.
      m,  // Particle mass assumed constant.
      h,  // Particle width assumed constant.
      mu, // Particle molecular viscosity assumed constant.
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
  println("Num. fixed particles: {}", num_fixed_particles);
  println("Num. fluid particles: {}", num_fluid_particles);

  // Set global particle constants.
  m[particles] = m_0;
  h[particles] = h_0;
  rho[particles] = rho_0;
  mu[particles] = mu_0;

  // Setup the particle adjacency structure.
  auto adjacent_particles = ParticleAdjacency{particles};

  particles.print("output/test_output/particles-0.csv");
  system("ln -sf output/test_output/particles-0.csv "
         "output/test_output/particles.csv");

  Real time{};
  Stopwatch exectime{}, printtime{};
  for (size_t n = 0; time <= 2.7; ++n, time += timestep) {
    println("{:>15}\t\t{:>10.5f}\t\t{:>10.5f}\t\t{:>10.5f}",
            n,
            time,
            exectime.cycle(),
            printtime.cycle());
    {
      const StopwatchCycle cycle{exectime};
      timeint.step(dt, particles, adjacent_particles);
    }
    if (n % 200 == 0 && n != 0) {
      const StopwatchCycle cycle{printtime};
      const auto path =
          "output/test_output/particles-" + std::to_string(n / 200) + ".csv";
      particles.print(path);
      system(("ln -sf " + path + " output/test_output/particles.csv").c_str());
    }
  }

  particles.print("particles-dam.csv");

  return 0;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#elif EASY_DAM_BREAKING

template<class Real>
int sph_main(int /*argc*/, char** /*argv*/) {
  using namespace tit;
  using namespace sph;

  constexpr Real H = 0.6;   // Water column height.
  constexpr Real L = 2 * H; // Water column length.

  constexpr Real POOL_WIDTH = 5.366 * H;
  constexpr Real POOL_HEIGHT = 2.5 * H;

  constexpr Real dt = 5.0e-5;
  constexpr Real dr = H / 80.0;

  constexpr auto N_fixed = 4;
  constexpr auto WATER_M = int(round(L / dr));
  constexpr auto WATER_N = int(round(H / dr));
  constexpr auto POOL_M = int(round(POOL_WIDTH / dr));
  constexpr auto POOL_N = int(round(POOL_HEIGHT / dr));

  constexpr Real g = 9.81;
  constexpr Real rho_0 = 1000.0;
  constexpr Real cs_0 = 20 * sqrt(g * H);
  constexpr Real h_0 = 2.0 * dr;
  constexpr Real m_0 = rho_0 * pow(dr, 2) / 1001.21 * 1000.0;

  // Setup the SPH equations:
  auto equations =
#if WITH_GODUNOV
      GodunovFluidEquations{
          // Weakly compressible equation of state.
          LinearWeaklyCompressibleFluidEquationOfState{cs_0, rho_0},
          // Continuity equation instead of density summation.
          ContinuityEquation{},
          // C2 Wendland's spline kernel.
          QuarticWendlandKernel{},
          // Use delta-SPH artificial viscosity formulation.
          DeltaSphArtificialViscosity{cs_0, rho_0},
      }
#else
      FluidEquations{
          // Weakly compressible equation of state.
          LinearWeaklyCompressibleFluidEquationOfState{cs_0, rho_0},
          // Continuity equation instead of density summation.
          ContinuityEquation{},
          // C2 Wendland's spline kernel.
          QuarticWendlandKernel{},
          // Use delta-SPH artificial viscosity formulation.
          DeltaSphArtificialViscosity{cs_0, rho_0},
      }
#endif
  ;

  // Setup the time integrator:
#if WITH_GODUNOV
  auto timeint = RungeKuttaIntegrator{std::move(equations)};
#else
  auto timeint = EulerIntegrator{std::move(equations)};
#endif

  // Setup the particles array:
  ParticleArray particles{// 2D space.
                          Space<Real, 2>{},
                          // Fields that are required by the equations.
                          timeint.required_fields,
                          // Set of whole system constants.
                          meta::Set{
                              m, // Particle mass assumed constant.
                              h, // Particle width assumed constant.
                          }};

  // Generate individual particles.
  auto num_fixed_particles = 0;
  auto num_fluid_particles = 0;
  for (auto i = -N_fixed; i < POOL_M + N_fixed; ++i) {
    for (auto j = -N_fixed; j < POOL_N; ++j) {
      const bool is_fixed = (i < 0 || i >= POOL_M) || (j < 0);
      const bool is_fluid = (i < WATER_M) && (j < WATER_N);
      if (is_fixed) ++num_fixed_particles;
      else if (is_fluid) ++num_fluid_particles;
      else continue;
      auto a = particles.append();
      fixed[a] = is_fixed;
      r[a] = dr * Vec{i + 0.5, j + 0.5};
    }
  }
  println("Num. fixed particles: {}", num_fixed_particles);
  println("Num. fluid particles: {}", num_fluid_particles);

  // Set global particle constants.
  m[particles] = m_0;
  h[particles] = h_0;

  // Density hydrostatic initialization.
  std::ranges::for_each(particles.all(), [&]<class PV>(PV a) {
    if (fixed[a]) {
      rho[a] = rho_0;
      return;
    }
    // Compute pressure from Poisson problem.
    const auto x = r[a][0];
    const auto y = r[a][1];
    p[a] = rho_0 * g * (H - y);
    for (size_t N = 1; N < 100; N += 2) {
      constexpr auto pi = std::numbers::pi_v<Real>;
      const auto n = static_cast<Real>(N);
      p[a] -= 8 * rho_0 * g * H / pow2(pi) *
              (exp(n * pi * (x - L) / (2 * H)) * cos(n * pi * y / (2 * H))) /
              pow2(n);
    }
    // Recalculate density from EOS.
    rho[a] = rho_0 + p[a] / pow2(cs_0);
  });

  // Setup the particle mesh structure.
  ParticleMesh mesh{geom::GridFactory{h_0}};

  system("mkdir -p output/test_output/");
  system("rm -f output/test_output/*");
  print_csv(particles, "output/test_output/particles-0.csv");
  system("ln -sf output/test_output/particles-0.csv particles.csv");

  Real time{};
  Stopwatch exectime{};
  Stopwatch printtime{};
  for (size_t n = 0;; ++n) {
    println("{:>15}\t\t{:>10.5f}\t\t{:>10.5f}\t\t{:>10.5f}",
            n,
            time * sqrt(g / H),
            exectime.cycle(),
            printtime.cycle());
    {
      const StopwatchCycle cycle{exectime};
      timeint.step(dt, mesh, particles);
    }
    if (n % 200 == 0 && n != 0) {
      const StopwatchCycle cycle{printtime};
      const auto path =
          std::format("output/test_output/particles-{}.csv", n / 200);
      print_csv(particles, path);
      system(("ln -sf ./" + path + " particles.csv").c_str());
    }
    if (time * sqrt(g / H) >= 6.9) break;
    time += dt;
  }

  print_csv(particles, "particles-dam.csv");

  return 0;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#elif INITIALLY_SQUARE_PATCH

template<class Real>
int sph_main(int /*argc*/, char** /*argv*/) {
  using namespace tit;
  using namespace sph;

  constexpr Real L = 1.0; // Square patch side length.

  constexpr Real dt = 5.0e-5;
  constexpr Real dr = L / 100.0;

  constexpr auto M = int(round(L / dr));

  constexpr Real omega = 1.0;
  constexpr Real rho_0 = 1000.0;
  constexpr Real cs_0 = 5.0 * sqrt(2.0) * omega * L;
  constexpr Real h_0 = 2.0 * dr;
  constexpr Real m_0 = rho_0 * pow(dr, 2) / 1001.21 * 1000.0;

  // Setup the SPH equations:
  auto equations =
#if WITH_GODUNOV
      GodunovFluidEquations{
          // Weakly compressible equation of state.
          LinearWeaklyCompressibleFluidEquationOfState{cs_0, rho_0},
          // Continuity equation instead of density summation.
          ContinuityEquation{},
          // C2 Wendland's spline kernel.
          QuarticWendlandKernel{},
          // Use delta-SPH artificial viscosity formulation.
          DeltaSphArtificialViscosity{cs_0, rho_0},
      }
#else
      FluidEquations{
          // Weakly compressible equation of state.
          LinearWeaklyCompressibleFluidEquationOfState{cs_0, rho_0},
          // Continuity equation instead of density summation.
          ContinuityEquation{},
          // C2 Wendland's spline kernel.
          QuarticWendlandKernel{},
          // Use delta-SPH artificial viscosity formulation.
          DeltaSphArtificialViscosity{cs_0, rho_0},
      }
#endif
  ;

  // Setup the time itegrator:
#if WITH_GODUNOV
  auto timeint = RungeKuttaIntegrator{std::move(equations)};
#else
  auto timeint = EulerIntegrator{std::move(equations)};
#endif

  // Setup the particles array:
  ParticleArray particles{
      // 2D space.
      Space<Real, 2>{},
      // Fields that are required by the equations.
      timeint.required_fields,
      // Set of whole system constants.
      m, // Particle mass assumed constant.
      h  // Particle width assumed constant.
  };

  // Generate individual particles.
  for (auto i = -M / 2; i < M / 2; ++i) {
    for (auto j = -M / 2; j < M / 2; ++j) {
      auto a = particles.append();
      r[a] = dr * Vec{i + 0.5, j + 0.5};
      v[a] = Vec{omega * r[a][1], -omega * r[a][0]};
    }
  }

  // Set global particle constants.
  m[particles] = m_0;
  h[particles] = h_0;

  // Density hydrostatic initialization.
  std::ranges::for_each(particles.all(), [&]<class PV>(PV a) {
    // Compute pressure from Poisson problem.
    const auto x = r[a][0] + L / 2, y = r[a][1] + L / 2;
    p[a] = 0.0;
    for (size_t m = 1; m < 11; ++m) {
      for (size_t n = 1; n < 11; ++n) {
        constexpr auto pi = std::numbers::pi_v<Real>;
        p[a] = -32 * pow2(omega) / (m * n * pow2(pi)) /
               (pow2(n * pi / L) + pow2(m * pi / L)) * //
               sin(m * pi * x / L) * sin(n * pi * y / L);
      }
    }
    p[a] *= rho_0;
    // Recalculate density from EOS.
    rho[a] = rho_0 + p[a] / pow2(cs_0);
  });

  // Setup the particle adjacency structure.
  auto adjacent_particles = ParticleAdjacency{particles};

  particles.print("output/test_output/particles-0.csv");
  system("ln -sf output/test_output/particles-0.csv particles.csv");

  Real time{};
  Stopwatch exectime{}, printtime{};
  for (size_t n = 0; time * omega <= 9.5; ++n, time += dt) {
    println("{:>15}\t\t{:>10.5f}\t\t{:>10.5f}\t\t{:>10.5f}",
            n,
            time * omega,
            exectime.cycle(),
            printtime.cycle());
    {
      const StopwatchCycle cycle{exectime};
      timeint.step(dt, particles, adjacent_particles);
    }
    if (n % 200 == 0 && n != 0) {
      const StopwatchCycle cycle{printtime};
      const auto path =
          "output/test_output/particles-" + std::to_string(n / 200) + ".csv";
      particles.print(path);
      system(("ln -sf ./" + path + " particles.csv").c_str());
    }
  }

  particles.print("particles.csv");

  return 0;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#endif

int main(int argc, char** argv) {
  return tit::run_main(argc, argv, &sph_main<tit::real_t>);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
