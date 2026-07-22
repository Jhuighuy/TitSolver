/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <cstddef>
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
  REQUIRE(communicator.size() >= 2);
  const auto rank = communicator.rank();
  const auto size = communicator.size();
  REQUIRE(size % 2 == 0);
  const auto slab_width = 1.0 / static_cast<double>(size);
  const SlabParticleTopology topology{communicator,
                                      0.0,
                                      1.0,
                                      0.51 * slab_width};

  ParticleArray particles{Space<double, 2>{},
                          ParticleLayout{TypeSet{h}, TypeSet{r, v, rho}}};
  h[particles] = 0.1;
  const auto id = ParticleID{static_cast<std::uint64_t>(rank)};
  const auto particle = particles.append(ParticleType::fluid, id);
  r[particle] = {(static_cast<double>(rank) + 0.5) * slab_width, 0.0};
  v[particle] = {static_cast<double>(rank), 0.0};
  rho[particle] = 1000.0 + static_cast<double>(rank);

  topology.exchange_halos(particles);
  const auto expected_ghosts = static_cast<std::size_t>(rank > 0) +
                               static_cast<std::size_t>(rank + 1 < size);
  CHECK(particles.num_ghosts() == expected_ghosts);
  if (rank > 0) {
    CHECK(particles.find(ParticleID{static_cast<std::uint64_t>(rank - 1)})
              .has_value());
  }
  if (rank + 1 < size) {
    CHECK(particles.find(ParticleID{static_cast<std::uint64_t>(rank + 1)})
              .has_value());
  }

  const auto destination = rank % 2 == 0 ? rank + 1 : rank - 1;
  r[particles[0]] = {(static_cast<double>(destination) + 0.5) * slab_width,
                     0.0};

  topology.migrate(particles);
  CHECK(particles.num_owned() == 1);
  CHECK(particles.num_ghosts() == 0);
  const auto source = rank % 2 == 0 ? rank + 1 : rank - 1;
  const auto incoming_id = ParticleID{static_cast<std::uint64_t>(source)};
  const auto incoming_index = particles.find(incoming_id);
  REQUIRE(incoming_index.has_value());
  const auto incoming = particles[incoming_index.value_or(particles.size())];
  CHECK(incoming.is_owned());
  CHECK(rho[incoming] == 1000.0 + static_cast<double>(source));

  topology.exchange_halos(particles);
  CHECK(particles.num_ghosts() == expected_ghosts);
  const auto global_owned = communicator.all_reduce_sum(
      static_cast<std::uint64_t>(particles.num_owned()));
  CHECK(global_owned == size);
}

TEST_CASE("sph::SlabParticleTopology assigns shared faces consistently") {
  const auto communicator = dist::Communicator::world();
  REQUIRE(communicator.size() >= 2);
  const auto size = communicator.size();
  const SlabParticleTopology topology{communicator, 0.0, 1.0, 0.01};

  CHECK(topology.owner(Vec{-0.1, 0.0}) == 0);
  CHECK(topology.owner(Vec{0.0, 0.0}) == 0);
  for (std::size_t rank = 1; rank < size; ++rank) {
    const auto face = static_cast<double>(rank) / static_cast<double>(size);
    CHECK(topology.owner(Vec{face, 0.0}) == rank);
  }
  CHECK(topology.owner(Vec{1.0, 0.0}) == size - 1);
  CHECK(topology.owner(Vec{1.1, 0.0}) == size - 1);
}

TEST_CASE("sph::SlabParticleTopology supports empty ranks") {
  const auto communicator = dist::Communicator::world();
  REQUIRE(communicator.size() >= 2);
  const SlabParticleTopology topology{communicator, 0.0, 1.0, 0.01};
  ParticleArray particles{Space<double, 2>{},
                          ParticleLayout{TypeSet{h}, TypeSet{r, v, rho}}};
  h[particles] = 0.1;
  if (communicator.rank() == 0) {
    const auto particle = particles.append(ParticleType::fluid, ParticleID{42});
    r[particle] = {0.99, 0.0};
    v[particle] = {4.2, 0.0};
    rho[particle] = 1042.0;
  }

  topology.migrate(particles);
  const auto owning_rank = communicator.size() - 1;
  CHECK(particles.num_owned() ==
        static_cast<std::size_t>(communicator.rank() == owning_rank));
  if (communicator.rank() == owning_rank) {
    const auto index = particles.find(ParticleID{42});
    REQUIRE(index.has_value());
    CHECK(v[particles[index.value_or(particles.size())]] == Vec{4.2, 0.0});
  }
  topology.exchange_halos(particles);
  CHECK(particles.num_ghosts() == 0);
  CHECK(communicator.all_reduce_sum(particles.num_owned()) == 1);
}

TEST_CASE("sph::SlabParticleTopology rebalances a concentrated distribution") {
  const auto communicator = dist::Communicator::world();
  REQUIRE(communicator.size() >= 2);
  SlabParticleTopology topology{communicator, 0.0, 1.0, 0.05};
  ParticleArray particles{Space<double, 2>{},
                          ParticleLayout{TypeSet{h}, TypeSet{r, v, rho}}};
  h[particles] = 0.1;
  constexpr std::size_t particles_per_rank = 8;
  const auto global_size = particles_per_rank * communicator.size();
  if (communicator.rank() == 0) {
    for (std::size_t index = 0; index < global_size; ++index) {
      const auto particle =
          particles.append(ParticleType::fluid,
                           ParticleID{static_cast<std::uint64_t>(index)});
      r[particle] = {(static_cast<double>(index) + 0.5) /
                         static_cast<double>(global_size),
                     0.0};
      v[particle] = {static_cast<double>(index), 0.0};
      rho[particle] = 1000.0 + static_cast<double>(index);
    }
  }

  topology.rebalance(particles);
  CHECK(particles.num_owned() == particles_per_rank);
  CHECK(communicator.all_reduce_sum(particles.num_owned()) == global_size);
  for (const auto particle : particles.owned()) {
    CHECK(topology.owner(r[particle]) == communicator.rank());
    CHECK(rho[particle] ==
          1000.0 + static_cast<double>(std::to_underlying(particle.id())));
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit::sph
