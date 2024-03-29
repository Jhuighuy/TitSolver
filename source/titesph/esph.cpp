#include <algorithm>
#include <cassert>
#include <cmath>
#include <fstream>
#include <vector>

#define WITH_GRAVITY 1

#include "tit/core/io_utils.hpp"
#include "tit/core/main_func.hpp"
#include "tit/core/time_utils.hpp"

#include "tit/sph/TitParticle.hpp"
#include "tit/sph/equation_of_state.hpp"
#include "tit/sph/fsi.hpp"
#include "tit/sph/kernel.hpp"
#include "tit/sph/time_integrator.hpp"

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

template<class Real>
int sph_main(int /*argc*/, char** /*argv*/) {
  using namespace tit;
  using namespace sph;

  constexpr Real H = 0.01; // Bar height.
  constexpr Real L = 0.10; // Bar length.

  // constexpr Real E_s = 1.4e+6, nu_s = 0.4;
  // constexpr Real K_s = E_s / (3.0 * (1.0 - 2.0 * nu_s));
  constexpr Real rho_0 = 1400.0;
  // constexpr Real cs_0 = sqrt(K_s / rho_0);

  constexpr Real dr = H / 10.0;
  constexpr Real h_0 = 1.3 * dr;
  constexpr Real dt = /*0.008 * h_0 / cs_0*/ 1.0e-5;

  constexpr auto N_fixed = 1;
  constexpr auto BAR_M = int(round(L / dr));
  constexpr auto BAR_N = int(round(H / dr));

  constexpr Real g = 9.81;
  constexpr Real m_0 = rho_0 * pow(dr, 2);

  // Setup the SPH equations:
  auto equations = fsi::StructureEquations{
      fsi::HookesLaw{},
      // C2 Wendland's spline kernel.
      EighthOrderWendlandKernel{}, AlphaBetaArtificialViscosity{0.2, 0.4}};

  // Setup the time itegrator:
  // auto timeint = EulerIntegrator{std::move(equations), SIZE_MAX};
  auto timeint = RungeKuttaIntegrator{std::move(equations), SIZE_MAX};

  // Setup the particles array:
  ParticleArray particles{// 2D space.
                          Space<Real, 2>{},
                          // Fields that are required by the equations.
                          timeint.required_fields,
                          // Set of whole system constants.
                          m, // Particle mass assumed constant.
                          h, // Particle width assumed constant.
                          rho};

  // Generate individual particles.
  auto num_fixed_particles = 0, num_struct_particles = 0;
  for (auto i = -N_fixed; i < BAR_M; ++i) {
    for (auto j = 0; j < BAR_N; ++j) {
      bool const is_fixed = i < 0;
      if (is_fixed) ++num_fixed_particles;
      else ++num_struct_particles;
      auto a = particles.append();
      fixed[a] = is_fixed;
      r[a] = dr * Vec{i + 0.5, j + 0.5};
    }
  }
  println("Num. fixed particles: {}", num_fixed_particles);
  println("Num. struct. particles: {}", num_struct_particles);

  // Set global particle constants.
  rho[particles] = rho_0;
  m[particles] = m_0;
  h[particles] = h_0;

  // Setup the particle adjacency structure.
  auto adjacent_particles = ParticleAdjacency{particles, geom::KDTreeFactory{}};

  particles.print("output/test_output/particles-0.csv");
  system("ln -sf output/test_output/particles-0.csv particles.csv");

  Real time{};
  Stopwatch exectime{}, printtime{};
  for (size_t n = 0; time * sqrt(g / H) <= 6.90e+6; ++n, time += dt) {
    println("{:>15}\t\t{:>10.5f}\t\t{:>10.5f}\t\t{:>10.5f}", //
            n, time * sqrt(g / H), exectime.cycle(), printtime.cycle());
    {
      StopwatchCycle const cycle{exectime};
      timeint.step(dt, particles, adjacent_particles);
    }
    if (n % 200 == 0 && n != 0) {
      StopwatchCycle const cycle{printtime};
      auto const path =
          "output/test_output/particles-" + std::to_string(n / 200) + ".csv";
      particles.print(path);
      system(("ln -sf ./" + path + " particles.csv").c_str());
    }
  }

  particles.print("particles-dam.csv");

  return 0;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

int main(int argc, char** argv) {
  return tit::run_main(argc, argv, &sph_main<tit::real_t>);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
