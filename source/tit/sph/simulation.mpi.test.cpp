/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "tit/core/type.hpp"
#include "tit/core/vec.hpp"
#include "tit/dist/communicator.hpp"
#include "tit/dist/environment.hpp"
#include "tit/geom/face_search.hpp"
#include "tit/geom/search.hpp"
#include "tit/sph/field.hpp"
#include "tit/sph/particle_array.hpp"
#include "tit/sph/particle_mesh.hpp"
#include "tit/sph/simulation.hpp"
#include "tit/sph/time_integrator.hpp"
#include "tit/testing/test.hpp"

namespace tit::sph {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

[[maybe_unused]] const dist::Environment environment{};

class ConstantEquations final {
public:

  static constexpr auto required_fields = TypeSet{h, r, v, rho, dv_dt, drho_dt};
  static constexpr auto modified_fields = TypeSet{r, v, rho, dv_dt, drho_dt};

  explicit ConstantEquations(double local_dt) noexcept : local_dt_{local_dt} {}

  template<class ParticleMesh, class ParticleArray>
  void initialize(ParticleMesh& /*mesh*/, ParticleArray& /*particles*/) const {}

  template<class ParticleMesh, class ParticleArray>
  void prepare(ParticleMesh& /*mesh*/, ParticleArray& /*particles*/) const {}

  template<class ParticleArray>
  auto compute_time_step(ParticleArray& /*particles*/) const -> double {
    return local_dt_;
  }

  template<class ParticleMesh, class ParticleArray>
  void compute_continuity(ParticleMesh& /*mesh*/,
                          ParticleArray& particles) const {
    for (const auto a : particles.owned()) drho_dt[a] = 0.0;
  }

  template<class ParticleMesh, class ParticleArray>
  void compute_momentum(ParticleMesh& /*mesh*/,
                        ParticleArray& particles) const {
    for (const auto a : particles.owned()) dv_dt[a] = {};
  }

  template<class ParticleMesh, class ParticleArray>
  void compute_shift_fields(ParticleMesh& /*mesh*/,
                            ParticleArray& /*particles*/) const {}

  template<class ParticleMesh, class ParticleArray>
  void classify_free_surface(ParticleMesh& /*mesh*/,
                             ParticleArray& /*particles*/) const {}

  template<class ParticleMesh, class ParticleArray>
  void classify_near_surface(ParticleMesh& /*mesh*/,
                             ParticleArray& /*particles*/) const {}

  template<class ParticleArray>
  void apply_particle_shifts(ParticleArray& /*particles*/) const {}

  template<class ParticleArray>
  void prepare_density_correction(ParticleArray& /*particles*/) const {}

  template<class ParticleMesh, class ParticleArray>
  void apply_density_correction(ParticleMesh& /*mesh*/,
                                ParticleArray& /*particles*/) const {}

private:

  double local_dt_;

}; // class ConstantEquations

TEST_CASE("sph::Simulation reduces the timestep across ranks") {
  const auto communicator = dist::Communicator::world();
  const auto local_dt = static_cast<double>(communicator.rank()) + 1.0;
  const Simulation simulation{ConstantEquations{local_dt},
                              SSPRKIntegrator{SSPRKOrder::three},
                              communicator};

  ParticleArray particles{
      Space<double, 2>{},
      ParticleLayout{TypeSet{h},
                     decltype(simulation)::required_fields - TypeSet{h}}};
  h[particles] = 1.0;
  const auto a = particles.append(ParticleType::fluid);
  r[a] = {};
  v[a] = {1.0, 0.0};
  rho[a] = 1.0;

  ParticleMesh mesh{geom::GridSearch{1.0}, geom::GridFaceSearch{1.0}};
  const auto dt = simulation.step(mesh, particles);

  CHECK(dt == 1.0);
  CHECK(r[a] == Vec{1.0, 0.0});
  CHECK(rho[a] == 1.0);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit::sph
