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
#include "tit/geom/polygon.hpp"
#include "tit/geom/search.hpp"
#include "tit/par/control.hpp"
#include "tit/sph/artificial_viscosity.hpp"
#include "tit/sph/continuity_equation.hpp"
#include "tit/sph/energy_equation.hpp"
#include "tit/sph/equation_of_state.hpp"
#include "tit/sph/field.hpp"
#include "tit/sph/fluid_equations.hpp"
#include "tit/sph/fluid_equations_riemann.hpp"
#include "tit/sph/kernel.hpp"
#include "tit/sph/momentum_equation.hpp"
#include "tit/sph/motion_equation.hpp"
#include "tit/sph/particle_array.hpp"
#include "tit/sph/particle_mesh.hpp"
#include "tit/sph/reconstruction.hpp"
#include "tit/sph/riemann_solver.hpp"
#include "tit/sph/time_integrator.hpp"
#include "tit/sph/viscosity.hpp"

namespace tit::sph::wcsph {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

template<class Real, bool Riemann>
auto sph_main(int /*argc*/, char** /*argv*/) -> int {
  constexpr Real H = 0.6;   // Water column height.
  constexpr Real L = 2 * H; // Water column length.

  constexpr Real POOL_WIDTH = 5.366 * H;
  constexpr Real POOL_HEIGHT = 2.5 * H;

  constexpr Real dr = H / 80.0;

  constexpr auto N_FIXED = 16;
  constexpr auto WATER_M = size_t(round(L / dr));
  constexpr auto WATER_N = size_t(round(H / dr));
  constexpr auto WALL_THICKNESS = dr * N_FIXED;

  constexpr Real g = 9.81;
  constexpr Real rho_0 = 1000.0;
  constexpr Real cs_0 = 20 * sqrt(g * H);
  constexpr Real h_0 = 2.0 * dr;
  constexpr Real m_0 = rho_0 * pow(dr, 2);

  [[maybe_unused]] constexpr Real R = 0.2;
  [[maybe_unused]] constexpr Real Ma = 0.1;
  constexpr Real CFL = 0.8;
  constexpr Real dt = std::min(CFL * h_0 / cs_0, Real{0.25} * sqrt(h_0 / g));

  // Parameters for the heat equation. Unused for now.
  [[maybe_unused]] constexpr Real kappa_0 = 0.6;
  [[maybe_unused]] constexpr Real c_v = 4184.0;

  // Setup the SPH equations.
  const auto equations = [&] {
    if constexpr (Riemann) {
      return FluidEquationsRiemann{
          // Continuity equation with no source terms.
          ContinuityEquation{},
          // Momentum equation with gravity source term.
          MomentumEquation{
              // Inviscid flow.
              NoViscosity{},
              NoArtificialViscosity{},
              // Gravity source term.
              GravitySource{g},
          },
          // Weakly compressible equation of state.
          LinearTaitEquationOfState{cs_0, rho_0},
          // C2 Wendland's spline kernel.
          QuarticWendlandKernel{},
          // Riemann solver for the fluid equations.
          ZhangRiemannSolver{cs_0},
          // WENO-3 reconstruction scheme.
          WENO3Reconstruction{},
      };
    } else {
      return FluidEquations{
          // Standard motion equation.
          MotionEquation{
              // Enabled particle shifting technique.
              ParticleShiftingTechnique{R, Ma, CFL},
          },
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
    }
  }();

  // Setup the time integrator.
  const RungeKuttaIntegrator time_integrator{equations};

  // Setup the particles array:
  ParticleArray particles{
      // 2D space.
      Space<Real, 2>{},
      // Set of fields is inferred from the equations.
      time_integrator,
  };

  // Generate fixed particles.
  size_t num_fixed_particles = 0;
  geom::poly::Polygon polygon{
      {0.0, 0.0},
      {POOL_WIDTH, 0.0},
      {POOL_WIDTH, POOL_HEIGHT},
      {POOL_WIDTH + WALL_THICKNESS, POOL_HEIGHT},
      {POOL_WIDTH + WALL_THICKNESS, -WALL_THICKNESS},
      {-WALL_THICKNESS, -WALL_THICKNESS},
      {-WALL_THICKNESS, POOL_HEIGHT},
      {0.0, POOL_HEIGHT},
  };
  const auto initial_polygon = polygon;
  for (auto i = 0; i < N_FIXED; ++i) {
    polygon = polygon.offset(-dr);
    const auto fine_polygon = polygon.subdivide_edges(dr);
    for (size_t path_index = 0; path_index < fine_polygon.num_paths();
         ++path_index) {
      for (size_t point_index = 0;
           point_index < fine_polygon.path_num_points(path_index);
           ++point_index) {
        const auto a = particles.append(ParticleType::fixed);
        r[a] =
            fine_polygon.point(path_index, point_index) + Vec{dr / 2, dr / 2};
        r_wall[a] = initial_polygon.closest_point(r[a]);
        ++num_fixed_particles;
      }
    }
  }
  log("Num. fixed particles: {}", num_fixed_particles);

  // Generate fluid particles.
  size_t num_fluid_particles = 0;
  for (size_t i = 0; i < WATER_M; ++i) {
    for (size_t j = 0; j < WATER_N; ++j) {
      const auto a = particles.append(ParticleType::fluid);
      r[a] = dr * Vec{i + Real{0.5}, j + Real{0.5}};
    }
  }
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
    for (size_t N = 1; N < 100; N += 2) {
      constexpr auto pi = std::numbers::pi_v<Real>;
      const auto n = static_cast<Real>(N);
      p_a -= 8 * rho_0 * g * H / pow2(pi) *
             (exp(n * pi * (x - L) / (2 * H)) * cos(n * pi * y / (2 * H))) /
             pow2(n);
    }

    // Recalculate density from EOS.
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

  // Create a data storage to store the particles.  We'll store only one last
  // run result, all the previous runs will be discarded.
  data::Storage storage{"./particles.ttdb"};
  storage.set_max_series(1);
  const auto series = storage.create_series();
  particles.write(0.0, series);

  // Run the simulation.
  Real time{};
  Stopwatch exectime{};
  Stopwatch printtime{};
  for (size_t n = 0;; ++n) {
    log("{:>15}\t\t{:>10.5f}\t\t{:>10.5f}\t\t{:>10.5f}",
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
} // namespace tit::sph::wcsph

TIT_IMPLEMENT_MAIN([](int argc, char** argv) {
  par::init();
  if (argc > 1 && std::string_view{argv[1]} == "riemann") {
    sph::wcsph::sph_main<float64_t, true>(argc, argv);
  } else {
    sph::wcsph::sph_main<float64_t, false>(argc, argv);
  }
});
