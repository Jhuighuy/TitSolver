/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <cstddef>
#include <cstdint>
#include <vector>

#include "tit/core/float.hpp"
#include "tit/core/logging.hpp"
#include "tit/core/main.hpp"
#include "tit/core/time.hpp"
#include "tit/core/type.hpp"
#include "tit/core/vec.hpp"
#include "tit/dist/communicator.hpp"
#include "tit/dist/environment.hpp"
#include "tit/geom/face_search.hpp"
#include "tit/geom/search.hpp"
#include "tit/geom/surface.hpp"
#include "tit/geom/tessellation.hpp"
#include "tit/geom/winding.hpp"
#include "tit/geom/winding/fast_winding.hpp"
#include "tit/io/parallel_run.hpp"
#include "tit/par/control.hpp"
#include "tit/sph/distributed_particles.hpp"
#include "tit/sph/equation_of_state.hpp"
#include "tit/sph/field.hpp"
#include "tit/sph/fluid_equations.hpp"
#include "tit/sph/kernel.hpp"
#include "tit/sph/particle_array.hpp"
#include "tit/sph/particle_mesh.hpp"
#include "tit/sph/simulation.hpp"
#include "tit/sph/time_integrator.hpp"

namespace tit::sph::wcsph {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

template<class Particles>
void write_snapshot(io::ParallelRunWriter& run,
                    const Particles& particles,
                    std::uint64_t step,
                    double time) {
  const auto count = particles.num_owned();
  const std::vector<ParticleType> kinds(count, ParticleType::fluid);
  auto frame = run.begin_frame(step, time);
  frame.write("id", particles.ids().first(count));
  frame.write("kind", kinds);
  frame.write("r", r[particles].first(count));
  frame.write("v", v[particles].first(count));
  frame.write("rho", rho[particles].first(count));
  frame.commit();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

template<class Real>
auto sph_main(const dist::Communicator& communicator) -> int {
  constexpr Real H = 0.6;   // Water column height.
  constexpr Real L = 2 * H; // Water column length.

  constexpr Real POOL_WIDTH = 5.366 * H; // Pool width.
  constexpr Real POOL_HEIGHT = 4.0 * H;  // Pool height.

  constexpr Real dr = H / 80.0; // Initial particle spacing.
  constexpr auto WATER_M = int(round(L / dr));
  constexpr auto WATER_N = int(round(H / dr));

  constexpr Real g = 9.81;
  constexpr Real rho_0 = 1000.0;
  constexpr Real cs_0 = 20 * sqrt(g * H);
  constexpr Real h_0 = 2.0 * dr;
  constexpr Real m_0 = rho_0 * pow(dr, 2);
  constexpr Real mu = 0.001;

  // Setup the SPH equations.
  geom::Surface<Vec<Real, 2>> domain;
  domain.append_vert({0.0, POOL_HEIGHT});
  domain.append_vert({POOL_WIDTH, POOL_HEIGHT});
  domain.append_vert({POOL_WIDTH, 0.0});
  domain.append_vert({0.0, 0.0});
  domain.append_face({0, 1});
  domain.append_face({1, 2});
  domain.append_face({2, 3});
  domain.append_face({3, 0});
  domain = geom::tessellate(domain, dr);

  // Another domain for containment tests.
  geom::Surface<Vec<Real, 2>> domain2;
  domain2.append_vert({0.0, 0.0});
  domain2.append_vert({POOL_WIDTH, 0.0});
  domain2.append_vert({POOL_WIDTH, POOL_HEIGHT});
  domain2.append_vert({0.0, POOL_HEIGHT});
  domain2.append_face({0, 1});
  domain2.append_face({1, 2});
  domain2.append_face({2, 3});
  domain2.append_face({3, 0});
  const geom::MakeFastWinding<Real> make_winding;
  const auto containment = make_winding(domain2);

  const SixthOrderWendlandKernel kernel{};
  const SlabParticleTopology topology{communicator,
                                      Real{0},
                                      POOL_WIDTH,
                                      kernel.radius(h_0)};
  const Simulation simulation{
      FluidEquations{
          // Constants.
          g,
          mu,
          // Wall boundary.
          domain,
          containment,
          // Weakly compressible equation of state.
          TaitEquationOfState{cs_0, rho_0},
          // C4 Wendland's spline kernel.
          kernel,
      },
      SSPRKIntegrator{SSPRKOrder::three},
      communicator,
      topology,
  };

  // Setup the particles array:
  ParticleArray particles{
      // 2D space.
      Space<Real, 2>{},
      // Store kernel width uniformly and all other integrator fields per
      // particle.
      ParticleLayout{TypeSet{h},
                     decltype(simulation)::required_fields - TypeSet{h}},
  };

  // Generate individual particles.
  for (auto i = 0; i < WATER_M; ++i) {
    for (auto j = 0; j < WATER_N; ++j) {
      const auto position = dr * Vec{i + Real{1.0}, j + Real{1.0}};
      if (topology.owner(position) != communicator.rank()) continue;
      const auto id = ParticleID{static_cast<std::uint64_t>(i) *
                                     static_cast<std::uint64_t>(WATER_N) +
                                 static_cast<std::uint64_t>(j)};
      auto a = particles.append(ParticleType::fluid, id);
      r[a] = position;
    }
  }
  const auto first_fixed_id =
      static_cast<std::uint64_t>(WATER_M) * static_cast<std::uint64_t>(WATER_N);
  for (std::size_t i = 0; i < domain.num_verts(); ++i) {
    auto a =
        particles.append(ParticleType::fixed, ParticleID{first_fixed_id + i});
    r[a] = domain.vert(i);
  }

  // Set global particle constants.
  h[particles] = h_0;
  for (const auto a : particles.all()) {
    m[a] = m_0;
    rho[a] = rho_0;
  }

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
    for (std::size_t k = 1; k < 100; k += 2) {
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
      // Search for the boundary faces using the grid search.
      geom::GridFaceSearch{h_0},
  };

  // Initialize the particles.
  simulation.initialize(mesh, particles);

  // Publish immutable snapshots directly from the owned SoA spans.
  io::ParallelRunWriter run{"./particles.tit-run",
                            io::RunMetadata{"dam-breaking", 2},
                            communicator};
  write_snapshot(run, particles, 0, 0.0);

  // Run the simulation.
  Real time{};
  Stopwatch exec_time{};
  Stopwatch print_time{};
  for (std::size_t step = 1;; ++step) {
    const auto scaled_time = time * sqrt(g / H);
    if (communicator.rank() == 0) {
      log("{:>15}\t\t{:>10.5f}\t\t{:>10.5f}\t\t{:>10.5f}",
          step,
          scaled_time,
          exec_time.cycle(),
          print_time.cycle());
    }

    Real dt{};
    {
      const StopwatchCycle cycle{exec_time};
      dt = simulation.step(mesh, particles);
    }
    time += dt;

    const auto end_time = 10.0;
    const auto next_scaled_time = time * sqrt(g / H);
    const auto end = next_scaled_time >= end_time;
    if ((step % 100 == 0) || end) {
      const StopwatchCycle cycle{print_time};
      write_snapshot(run, particles, step, next_scaled_time);
    }

    if (end) break;
  }

  return 0;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit::sph::wcsph

TIT_IMPLEMENT_MAIN([](int argc, char** argv) {
  const dist::Environment environment{argc, argv};
  const auto communicator = dist::Communicator::world();
  par::init();
  sph::wcsph::sph_main<tit::float64_t>(communicator);
});
