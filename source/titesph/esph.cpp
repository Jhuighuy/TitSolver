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
#include "tit/sph/field.hpp"
#include "tit/sph/kernel.hpp"
#include "tit/sph/particle_array.hpp"
#include "tit/sph/particle_mesh.hpp"
#include "tit/sph/time_integrator.hpp"
#include "tit/sph/total_lagrangian.hpp"

namespace tit::sph::tlsph {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

template<class Real>
auto sph_main(int /*argc*/, char** /*argv*/) -> int {
  constexpr Real H = 0.01; // Bar height.
  constexpr Real L = 0.10; // Bar length.

  constexpr Real Y = 2.0e+6;     // Young's modulus.
  constexpr Real nu = 0.4;       // Poisson's ratio.
  constexpr Real rho_0 = 1400.0; // Reference density.

  constexpr Real g = 9.81;
  constexpr Real dr = H / 10.0;
  constexpr Real dt = 1.0e-5;
  constexpr Real h_0 = 1.3 * dr;
  constexpr Real m_0 = rho_0 * pow(dr, 2);

  constexpr auto N_FIXED = 1;
  constexpr auto BAR_M = int(round(L / dr));
  constexpr auto BAR_N = int(round(H / dr));

  // Setup the SPH equations.
  const TLElasticEquations equations{
      // Constitutive law.
      NeoHookean{Y, nu},
      // C2 Wendland's spline kernel.
      EighthOrderWendlandKernel{},
  };

  // Setup the time integrator.
  const RungeKuttaIntegrator time_integrator{equations};

  // Setup the particles array:
  ParticleArray particles{
      // 2D space.
      Space<Real, 2>{},
      // Set of fields is inferred from the equations.
      time_integrator,
  };

  // Generate individual particles.
  size_t num_fixed_particles = 0;
  size_t num_struct_particles = 0;
  for (auto i = -N_FIXED; i < BAR_M; ++i) {
    for (auto j = 0; j < BAR_N; ++j) {
      const bool is_fixed = i < 0;
      if (is_fixed) ++num_fixed_particles;
      else ++num_struct_particles;

      auto a = particles.append(is_fixed ? ParticleType::fixed :
                                           ParticleType::fluid);
      r[a] = dr * Vec{i + Real{0.5}, j + Real{0.5}};
      rho[a] = rho_0;
    }
  }
  log("Num. fixed particles: {}", num_fixed_particles);
  log("Num. elastic particles: {}", num_struct_particles);

  // Set global particle constants.
  m[particles] = m_0;
  h[particles] = h_0;

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
  data::DataStorage storage{"./particles.ttdb"};
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

    const auto end = time * sqrt(g / H) >= 6.9e+6;
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
} // namespace tit::sph::tlsph

TIT_IMPLEMENT_MAIN([](int argc, char** argv) {
  sph::tlsph::sph_main<float64_t>(argc, argv);
});
