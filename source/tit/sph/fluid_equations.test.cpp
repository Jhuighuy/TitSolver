/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <algorithm>
#include <limits>

#include "tit/core/math.hpp"
#include "tit/core/type.hpp"
#include "tit/core/vec.hpp"
#include "tit/geom/face_search.hpp"
#include "tit/geom/search.hpp"
#include "tit/geom/surface.hpp"
#include "tit/sph/equation_of_state.hpp"
#include "tit/sph/field.hpp"
#include "tit/sph/fluid_equations.hpp"
#include "tit/sph/kernel.hpp"
#include "tit/sph/particle_array.hpp"
#include "tit/sph/particle_mesh.hpp"
#include "tit/testing/test.hpp"

namespace tit::sph {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

using TestVec = Vec<double, 2>;

struct AllSpace final {
  static constexpr auto contains(const TestVec& /*point*/) noexcept -> bool {
    return true;
  }
};

struct EquationFixture {
  using Equations = FluidEquations<double,
                                   LinearTaitEquationOfState<double>,
                                   SixthOrderWendlandKernel,
                                   geom::Surface<TestVec>,
                                   AllSpace>;

  geom::Surface<TestVec> domain{};
  LinearTaitEquationOfState<double> eos{20.0, 1000.0};
  Equations equations{9.81, 0.001, domain, AllSpace{}, eos, {}};
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE_FIXTURE(EquationFixture, "sph::FluidEquations compute_time_step") {
  ParticleArray particles{
      Space<double, 2>{},
      ParticleLayout{TypeSet{h}, Equations::required_fields - TypeSet{h}}};
  h[particles] = 0.5;

  auto a = particles.append(ParticleType::fluid);
  rho[a] = 1000.0;
  v[a] = {3.0, 4.0};
  dv_dt[a] = {0.0, 16.0};

  auto b = particles.append(ParticleType::fluid);
  rho[b] = 1100.0;
  v[b] = {0.0, 1.0};
  dv_dt[b] = {0.0, 4.0};

  // Fixed particles must not constrain the fluid timestep.
  auto e = particles.append(ParticleType::fixed);
  rho[e] = 1000.0;
  v[e] = {std::numeric_limits<double>::max(), 0.0};
  dv_dt[e] = {std::numeric_limits<double>::max(), 0.0};

  const auto expected_for = [this](auto particle) {
    const auto dt_acoustic =
        0.4 * h[particle] /
        (eos.sound_speed_from_density(rho[particle]) + norm(v[particle]));
    const auto dt_visc = 0.125 * pow2(h[particle]) * rho[particle] / 0.001;
    const auto dt_force =
        0.25 * sqrt(h[particle] / std::max(norm(dv_dt[particle]), 9.81));
    return std::min({dt_acoustic, dt_visc, dt_force});
  };

  CHECK_APPROX_EQ(equations.compute_time_step(particles),
                  std::min(expected_for(a), expected_for(b)));
}

TEST_CASE("sph::FluidEquations pair momentum is conservative") {
  const geom::Surface<TestVec> domain{};
  const LinearTaitEquationOfState<double> eos{20.0, 1000.0};
  const FluidEquations equations{0.0,
                                 0.001,
                                 domain,
                                 AllSpace{},
                                 eos,
                                 SixthOrderWendlandKernel{}};
  using Equations = decltype(equations);
  ParticleArray particles{
      Space<double, 2>{},
      ParticleLayout{TypeSet{h}, Equations::required_fields - TypeSet{h}}};
  h[particles] = 1.0;

  auto a = particles.append(ParticleType::fluid);
  m[a] = 2.0;
  r[a] = {-0.25, 0.0};
  v[a] = {1.0, 0.5};
  rho[a] = 1000.0;

  auto b = particles.append(ParticleType::fluid);
  m[b] = 3.0;
  r[b] = {0.25, 0.0};
  v[b] = {-0.5, -1.0};
  rho[b] = 1000.0;

  ParticleMesh mesh{geom::GridSearch{1.0}, geom::GridFaceSearch{1.0}};
  equations.prepare(mesh, particles);
  equations.compute_momentum(mesh, particles);

  CHECK_APPROX_EQ(m[a] * dv_dt[a] + m[b] * dv_dt[b], TestVec{});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit::sph
