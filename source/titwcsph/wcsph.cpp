/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "tit/core/basic_types.hpp"
#include "tit/core/main.hpp"
#include "tit/core/print.hpp"
#include "tit/core/time.hpp"
#include "tit/core/vec.hpp"
#include "tit/data/storage.hpp"
#include "tit/geom/partition.hpp"
#include "tit/geom/search.hpp"
#include "tit/par/control.hpp"
#include "tit/sph/artificial_viscosity.hpp"
#include "tit/sph/equation_of_state.hpp"
#include "tit/sph/field.hpp"
#include "tit/sph/fluid_equations.hpp"
#include "tit/sph/kernel.hpp"
#include "tit/sph/particle_array.hpp"
#include "tit/sph/particle_mesh.hpp"
#include "tit/sph/time_integrator.hpp"

namespace tit::sph::wcsph {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

template<class Real>
auto sph_main(int /*argc*/, char** /*argv*/) -> int {
  constexpr Real H = 0.6;   // Water column height.
  constexpr Real L = 2 * H; // Water column length.

  constexpr Real POOL_WIDTH = 5.366 * H; // Pool width.
  constexpr Real POOL_HEIGHT = 2.5 * H;  // Pool height.

  constexpr Real dr = H / 80.0; // Initial particle spacing.
  constexpr auto WATER_M = int(round(L / dr));
  constexpr auto WATER_N = int(round(H / dr));
  constexpr auto POOL_M = int(round(POOL_WIDTH / dr));
  constexpr auto POOL_N = int(round(POOL_HEIGHT / dr));
  constexpr auto N_FIXED = 4;

  constexpr Real g = 9.81;
  constexpr Real rho_0 = 1000.0;
  constexpr Real cs_0 = 20 * sqrt(g * H);
  constexpr Real h_0 = 2.0 * dr;
  constexpr Real m_0 = rho_0 * pow(dr, 2);
  constexpr Real R = 0.2;
  constexpr Real Ma = 0.1;
  constexpr Real CFL = 0.8;
  constexpr Real dt = std::min(CFL * h_0 / cs_0, Real{0.25} * sqrt(h_0 / g));

  // Setup the SPH equations.
  const FluidEquations equations{
      // Constants.
      rho_0,
      cs_0,
      g,
      R,
      Ma,
      CFL,
      // Weakly compressible equation of state.
      LinearTaitEquationOfState{cs_0, rho_0},
      // δ-SPH artificial viscosity formulation.
      DeltaSPHArtificialViscosity{cs_0, rho_0},
      // C2 Wendland's spline kernel.
      QuarticWendlandKernel{},
  };

  // Setup the time integrator.
  const RungeKuttaIntegrator time_integrator{equations};

  // Setup the particles array:
  ParticleArray particles{
      // 2D space.
      Space<Real, 2>{},
      // Set of fields is inferred from the time integrator.
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
      r[a] = dr * Vec{i + Real{0.5}, j + Real{0.5}};
    }
  }
  log("Num. fixed particles: {}", num_fixed_particles);
  log("Num. fluid particles: {}", num_fluid_particles);

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
    auto p_a = rho_0 * g * (H - y);
    for (size_t k = 1; k < 100; k += 2) {
      constexpr auto pi = std::numbers::pi_v<Real>;
      const auto k_pi = static_cast<Real>(k) * pi;
      p_a -= 8 * rho_0 * g * H / pow2(k_pi) *
             (exp(k_pi * (x - L) / (2 * H)) * cos(k_pi * y / (2 * H)));
    }

    // Recalculate density.
    rho[a] = rho_0 + p_a / pow2(cs_0);
  }

  // Setup the particle mesh structure.
  ParticleMesh mesh{
      // Search for the particles using the grid search.
      geom::GridSearch{h_0},
      // Use RIB as the primary partitioning method.
      geom::RecursiveInertialBisection{},
      // Use pixelated K-means as the interface partitioning method.
      geom::PixelatedPartition{2 * h_0, geom::KMeansClustering{}},
  };

  // Create a data storage to store the particles. We'll store only one last
  // run result, all the previous runs will be discarded.
  data::Storage storage{"./particles.ttdb"};
  storage.set_max_series(1);
  const auto series = storage.create_series();
  particles.write(0.0, series);

  // Run the simulation.
  Real time{};
  Stopwatch exec_time{};
  Stopwatch print_time{};
  for (size_t step = 0;; ++step) {
    const auto scaled_time = time * sqrt(g / H);
    log("{:>15}\t\t{:>10.5f}\t\t{:>10.5f}\t\t{:>10.5f}",
        step,
        scaled_time,
        exec_time.cycle(),
        print_time.cycle());

    {
      const StopwatchCycle cycle{exec_time};
      time_integrator.step(dt, mesh, particles);
    }

    const auto end_time = 10.0;
    const auto end = scaled_time >= end_time;
    if ((step % 100 == 0 && step != 0) || end) {
      const StopwatchCycle cycle{print_time};
      particles.write(scaled_time, series);
    }

    if (end) break;
    time += dt;
  }

  return 0;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit::sph::wcsph

TIT_IMPLEMENT_MAIN([](int argc, char** argv) {
  par::init();
  sph::wcsph::sph_main<float64_t>(argc, argv);
});
