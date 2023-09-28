#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <vector>

#define WITH_GRAVITY 1

#include "tit/sph/TitParticle.hpp"
#include "tit/sph/equation_of_state.hpp"
#include "tit/sph/fsi.hpp"
#include "tit/sph/kernel.hpp"
#include "tit/sph/time_integrator.hpp"

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

template<class Real>
int sph_main() {
  using namespace tit;
  using namespace sph;

  constexpr Real H = 0.01; // Bar height.
  constexpr Real L = 0.10; // Bar length.

  constexpr Real E_s = 1.4e+6, nu_s = 0.4;
  constexpr Real K_s = E_s / (3.0 * (1.0 - 2.0 * nu_s));
  constexpr Real rho_0 = 1400.0;
  constexpr Real cs_0 = sqrt(K_s / rho_0);

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
      const bool is_fixed = i < 0;
      if (is_fixed) ++num_fixed_particles;
      else ++num_struct_particles;
      auto a = particles.append();
      fixed[a] = is_fixed;
      r[a] = dr * Vec{i + 0.5, j + 0.5};
    }
  }
  std::cout << "Num. fixed particles: " << num_fixed_particles << std::endl;
  std::cout << "Num. struct. particles: " << num_struct_particles << std::endl;

  // Set global particle constants.
  rho[particles] = rho_0;
  m[particles] = m_0;
  h[particles] = h_0;

  // Setup the particle adjacency structure.
  auto adjacent_particles = ParticleAdjacency{particles};

  particles.print("output/test_output/particles-0.csv");
  system("ln -sf output/test_output/particles-0.csv particles.csv");

  Real time = 0.0, exectime = 0.0, printtime = 0.0;
  for (size_t n = 0; time * sqrt(g / H) <= 6.90e+6; ++n, time += dt) {
    std::cout << n << "\t\t" << time * sqrt(g / H) << "\t\t" << exectime / n
              << "\t\t" << printtime / (n / 200) << std::endl;
    auto start = std::chrono::high_resolution_clock::now();
    timeint.step(dt, particles, adjacent_particles);
    auto delta = std::chrono::high_resolution_clock::now() - start;
    exectime +=
        1.0e-9 *
        std::chrono::duration_cast<std::chrono::nanoseconds>(delta).count();
    if (n % 200 == 0 && n != 0) {
      start = std::chrono::high_resolution_clock::now();
      const auto path =
          "output/test_output/particles-" + std::to_string(n / 200) + ".csv";
      particles.print(path);
      system(("ln -sf ./" + path + " particles.csv").c_str());
      auto delta = std::chrono::high_resolution_clock::now() - start;
      printtime +=
          1.0e-9 *
          std::chrono::duration_cast<std::chrono::nanoseconds>(delta).count();
    }
  }

  particles.print("particles-dam.csv");

  const auto totaltime = exectime + printtime;
  std::cout << "Total time: " << totaltime / 60.0
            << "m, exec: " << exectime * 100 / totaltime
            << "%, print: " << printtime * 100 / totaltime << "%" << std::endl;

  return 0;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

int main(int argc, char** argv) {
  return tit::par::main(argc, argv, [] { return sph_main<tit::real_t>(); });
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
