/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <cstdint>
#include <utility>

#include "tit/core/type.hpp"
#include "tit/core/vec.hpp"
#include "tit/dist/communicator.hpp"
#include "tit/dist/environment.hpp"
#include "tit/geom/face_search.hpp"
#include "tit/geom/search.hpp"
#include "tit/sph/distributed_particles.hpp"
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

TEST_CASE("sph::SlabParticleTopology exchanges halos and migrates state") {
  const auto communicator = dist::Communicator::world();
  REQUIRE(communicator.size() == 2);
  const SlabParticleTopology topology{communicator, 0.0, 1.0, 0.11};

  ParticleArray particles{Space<double, 2>{},
                          ParticleLayout{TypeSet{h}, TypeSet{r, v, rho}}};
  h[particles] = 0.1;
  const auto append = [&](std::uint64_t id, double x) {
    const auto particle = particles.append(ParticleType::fluid, ParticleID{id});
    r[particle] = {x, 0.0};
    v[particle] = {static_cast<double>(id), 0.0};
    rho[particle] = 1000.0 + static_cast<double>(id);
  };

  if (communicator.rank() == 0) {
    append(10, 0.10);
    append(11, 0.45);
    append(12, 0.75); // Migrates to rank one.
  } else {
    append(20, 0.55);
    append(21, 0.90);
    append(22, 0.25); // Migrates to rank zero.
  }

  topology.exchange_halos(particles);
  CHECK(particles.num_ghosts() == 2);
  if (communicator.rank() == 0) {
    CHECK(particles.find(ParticleID{20}).has_value());
    CHECK(particles.find(ParticleID{22}).has_value());
  } else {
    CHECK(particles.find(ParticleID{11}).has_value());
    CHECK(particles.find(ParticleID{12}).has_value());
  }

  topology.migrate(particles);
  CHECK(particles.num_owned() == 3);
  CHECK(particles.num_ghosts() == 0);
  const auto incoming_id =
      communicator.rank() == 0 ? ParticleID{22} : ParticleID{12};
  const auto incoming_index = particles.find(incoming_id);
  REQUIRE(incoming_index.has_value());
  const auto incoming = particles[incoming_index.value_or(particles.size())];
  CHECK(incoming.is_owned());
  CHECK(rho[incoming] ==
        1000.0 + static_cast<double>(std::to_underlying(incoming_id)));

  topology.exchange_halos(particles);
  CHECK(particles.num_ghosts() == 1);
  const auto global_owned = communicator.all_reduce_sum(
      static_cast<std::uint64_t>(particles.num_owned()));
  CHECK(global_owned == 6);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit::sph
