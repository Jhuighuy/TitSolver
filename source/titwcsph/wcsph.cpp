#include "tit/core/basic_types.hpp"
#include "tit/core/log.hpp"
#include "tit/core/main_func.hpp"
#include "tit/core/time.hpp"
#include "tit/core/vec.hpp"

#include "tit/geom/partition.hpp"
#include "tit/geom/search.hpp"

#include "tit/data/storage.hpp"

#include "tit/sph/artificial_viscosity.hpp"
#include "tit/sph/continuity_equation.hpp"
#include "tit/sph/energy_equation.hpp"
#include "tit/sph/equation_of_state.hpp"
#include "tit/sph/field.hpp"
#include "tit/sph/fluid_equations.hpp"
#include "tit/sph/kernel.hpp"
#include "tit/sph/momentum_equation.hpp"
#include "tit/sph/motion_equation.hpp"
#include "tit/sph/particle_array.hpp"
#include "tit/sph/particle_mesh.hpp"
#include "tit/sph/time_integrator.hpp"
#include "tit/sph/viscosity.hpp"

namespace tit::sph {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

template<class Real>
auto sph_main(CmdArgs /*args*/) -> int {
  constexpr Real H = 0.6;   // Water column height.
  constexpr Real L = 2 * H; // Water column length.

  constexpr Real POOL_WIDTH = 5.366 * H;
  constexpr Real POOL_HEIGHT = 2.5 * H;

  constexpr Real dr = H / 80.0;

  constexpr auto N_FIXED = 4;
  constexpr auto WATER_M = int(round(L / dr));
  constexpr auto WATER_N = int(round(H / dr));
  constexpr auto POOL_M = int(round(POOL_WIDTH / dr));
  constexpr auto POOL_N = int(round(POOL_HEIGHT / dr));

  constexpr Real g = 9.81;
  constexpr Real rho_0 = 1000.0;
  constexpr Real cs_0 = 20 * sqrt(g * H);
  constexpr Real h_0 = 2.0 * dr;
  constexpr Real m_0 = rho_0 * pow(dr, 2);

  constexpr Real CFL = 0.8;
  constexpr Real dt = std::min(CFL * h_0 / cs_0, 0.25 * sqrt(h_0 / g));

  // Parameters for the heat equation. Unused for now.
  [[maybe_unused]] constexpr Real kappa_0 = 0.6;
  [[maybe_unused]] constexpr Real c_v = 4184.0;

  // Setup the SPH equations.
  const FluidEquations equations{
      // Standard motion equation.
      MotionEquation{},
      // Continuity equation with no source terms.
      ContinuityEquation{},
      // Momentum equation with gravity source term.
      MomentumEquation{
          // Inviscid flow.
          NoViscosity{},
          // δ-SPH artificial viscosity formulation.
          DeltaSPHArtificialViscosity{cs_0, rho_0},
          // Gravity source term.
          GravitySource{g},
      },
      // No energy equation.
      NoEnergyEquation{},
      // Weakly compressible equation of state.
      LinearTaitEquationOfState{cs_0, rho_0},
      // C2 Wendland's spline kernel.
      QuarticWendlandKernel{},
  };

  // Setup the time integrator.
  RungeKuttaIntegrator time_integrator{equations};

  // Setup the particles array:
  ParticleArray particles{
      // 2D space.
      Space<Real, 2>{},
      // Set of fields is inferred from the equations.
      time_integrator,
  };

  // Generate individual particles.
  size_t num_fixed_particles = 0;
  size_t num_fluid_particles = 0;
  for (auto i = -N_FIXED; i < POOL_M + N_FIXED; ++i) {
    for (auto j = -N_FIXED; j < POOL_N; ++j) {
      const bool is_fixed = (i < 0 || i >= POOL_M) || (j < 0);
      const bool is_fluid = (i < WATER_M) && (j < WATER_N);

      if (is_fixed) num_fixed_particles += 1;
      else if (is_fluid) num_fluid_particles += 1;
      else continue;

      auto a = particles.append(is_fixed ? ParticleType::fixed :
                                           ParticleType::fluid);
      r[a] = dr * Vec{i + 0.5, j + 0.5};
    }
  }
  TIT_INFO("Num. fixed particles: {}", num_fixed_particles);
  TIT_INFO("Num. fluid particles: {}", num_fluid_particles);

  // Set global particle constants.
  m[particles] = m_0;
  h[particles] = h_0;

  // Density hydrostatic initialization.
  for (const auto a : particles.all()) {
    if (a.has_type(ParticleType::fixed)) {
      rho[a] = rho_0;
      continue;
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
  }

  // Setup the particle mesh structure.
  ParticleMesh mesh{
      // Search for the particles using the grid search.
      geom::GridSearch{h_0},
      // Use RIB as the primary partitioning method.
      geom::RecursiveInertialBisection{},
      // Use graph partitioning with larger cell size as the interface
      // partitioning method.
      geom::GridGraphPartition{/* 2* */ h_0},
  };

  // Create a data storage to store the particles.  We'll store only one last
  // run result, all the previous runs will be discarded.
  data::DataStorage storage{"./particles.ttdb"};
  storage.set_max_series(1);
  const auto series = storage.create_series();
  particles.write(0.0, series);

  Real time{};
  Stopwatch exectime{};
  Stopwatch printtime{};
  for (size_t n = 0;; ++n) {
    TIT_INFO("{:>15}\t\t{:>10.5f}\t\t{:>10.5f}\t\t{:>10.5f}",
             n,
             time * sqrt(g / H),
             exectime.cycle(),
             printtime.cycle());
    {
      const StopwatchCycle cycle{exectime};
      time_integrator.step(dt, mesh, particles);
    }
    const auto end = time * sqrt(g / H) >= 6.9;
    if ((n % 100 == 0 && n != 0) || end) {
      const StopwatchCycle cycle{printtime};
      particles.write(time * sqrt(g / H), series);
    }
    if (end) break;
    time += dt;
  }

  return 0;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit::sph

TIT_IMPLEMENT_MAIN(sph::sph_main<tit::real_t>)
