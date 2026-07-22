/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <ranges>
#include <vector>

#include "tit/core/type.hpp"
#include "tit/core/vec.hpp"
#include "tit/dist/exchange.hpp"
#include "tit/mpi/mpi.hpp"
#include "tit/sph/field.hpp"
#include "tit/sph/particle_array.hpp"
#include "tit/testing/test.hpp"

namespace tit {
namespace {

// Minimal set of equations to instantiate a particle array with.
struct TestEquations {
  static constexpr auto required_fields = TypeSet{h, m, sph::r, rho, v};
  static constexpr auto modified_fields = TypeSet{sph::r, rho, v};
};

constexpr std::size_t N = 20;       // Particles per grid side.
constexpr double halo_radius = 2.5; // Grid spacing is 1.

// Generate the replicated test particle grid: `N x N` fluid particles at the
// unit spacing, plus a row of fixed particles below.
auto make_replicated_array() {
  sph::ParticleArray particles{Space<double, 2>{}, TestEquations{}};
  for (const auto a : particles.append_n(sph::ParticleType::fluid, N * N)) {
    const auto i = gid[a] % N;
    const auto j = gid[a] / N;
    sph::r[a] = Vec{static_cast<double>(i), static_cast<double>(j)};
    rho[a] = static_cast<double>(gid[a]);
  }
  for (const auto a : particles.append_n(sph::ParticleType::fixed, N)) {
    const auto i = gid[a] - N * N;
    sph::r[a] = Vec{static_cast<double>(i), -1.0};
    rho[a] = static_cast<double>(gid[a]);
  }
  return particles;
}

using TestArray = decltype(make_replicated_array());

// Reference positions of all the particles, by the global identifier.
auto reference_positions() -> std::vector<Vec<double, 2>> {
  std::vector<Vec<double, 2>> positions{};
  const auto reference = make_replicated_array();
  positions.resize(reference.size());
  for (const auto a : reference.all()) positions[gid[a]] = sph::r[a];
  return positions;
}

// Check the fundamental invariants of a distributed array:
//
// - the owned particles exactly partition the global set;
// - every particle within the halo radius of an owned particle is present
//   locally, either as owned or as a ghost;
// - the ghost positions match the reference.
void check_invariants(TestArray& particles) {
  CAPTURE(mpi::world.rank());

  // Global identifiers of the owned particles must form a partition.
  std::vector<std::uint64_t> owned_gids{};
  for (const auto a : particles.owned()) owned_gids.push_back(gid[a]);
  auto counts = mpi::world.all_gather(owned_gids.size());
  std::vector<std::uint64_t> num_globals(1, 0);
  for (const auto count : counts) num_globals[0] += count;
  REQUIRE(num_globals[0] == N * N + N);

  // Identifiers must be globally unique: local uniqueness plus the correct
  // global count implies a partition only if each identifier appears once.
  // Verify by summing the identifiers globally.
  std::uint64_t gid_sum = 0;
  for (const auto g : owned_gids) gid_sum += g;
  gid_sum = mpi::world.all_reduce(gid_sum, mpi::Op::sum);
  const std::uint64_t num_particles = N * N + N;
  CHECK(gid_sum == num_particles * (num_particles - 1) / 2);

  // Halo completeness: every reference particle within the halo radius of
  // an owned particle must be present locally.
  const auto reference = reference_positions();
  std::vector<bool> present(reference.size(), false);
  for (const auto a : particles.all()) {
    CHECK(gid[a] < present.size());
    present[gid[a]] = true;
    // Positions of all the local particles must match the reference.
    CHECK(sph::r[a] == reference[gid[a]]);
  }
  for (const auto a : particles.owned()) {
    for (const auto [g, ref_r] : std::views::enumerate(reference)) {
      if (norm(ref_r - sph::r[a]) <= halo_radius) {
        CAPTURE(g);
        CHECK(present[static_cast<std::size_t>(g)]);
      }
    }
  }

  // Types must be consistent: fluid identifiers are below `N * N`.
  for (const auto a : particles.all()) {
    CHECK(a.is_fluid() == (gid[a] < N * N));
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("dist::ParticleExchange") {
  dist::ParticleExchange exchange{halo_radius};
  auto particles = make_replicated_array();
  exchange.distribute(particles);

  SUBCASE("distribute") {
    check_invariants(particles);

    // The initial refresh must have brought the field values over.
    for (const auto a : particles.all()) {
      CHECK(rho[a] == static_cast<double>(gid[a]));
    }
  }

  SUBCASE("refresh") {
    // Modify a field on the owned particles, and refresh the ghosts.
    for (const auto a : particles.owned()) {
      rho[a] = 3.0 * static_cast<double>(gid[a]) + 7.0;
    }
    exchange.refresh(particles, TypeSet{rho});
    for (const auto a : particles.all()) {
      CHECK(rho[a] == 3.0 * static_cast<double>(gid[a]) + 7.0);
    }
  }

  SUBCASE("rebuild") {
    // Nudge the particles so that some of them change hands, and rebuild.
    // The shift pattern is identical on all the processes.
    for (const auto a : particles.owned()) {
      if (!a.is_fluid()) continue;
      const auto j = gid[a] / N;
      sph::r[a] += Vec{j % 3 == 0 ? 1.5 : -0.5, 0.0};
      rho[a] = 5.0 * static_cast<double>(gid[a]) + 1.0;
    }
    exchange.rebuild(particles);

    // Ghosts must carry the moved positions and the updated fields.
    const auto reference = [] {
      std::vector<Vec<double, 2>> positions{reference_positions()};
      for (const auto g : std::views::iota(std::size_t{0}, N * N)) {
        const auto j = g / N;
        positions[g] += Vec{j % 3 == 0 ? 1.5 : -0.5, 0.0};
      }
      return positions;
    }();
    for (const auto a : particles.all()) {
      CHECK(sph::r[a] == reference[gid[a]]);
      if (a.is_fluid()) {
        CHECK(rho[a] == 5.0 * static_cast<double>(gid[a]) + 1.0);
      }
    }

    // The canonical order must be restored: within each owned segment the
    // identifiers are sorted.
    const auto is_sorted_by_gid = [](auto&& segment) {
      return std::ranges::is_sorted(
          segment | std::views::transform([](auto a) { return gid[a]; }));
    };
    CHECK(is_sorted_by_gid(particles.fluid()));
    CHECK(is_sorted_by_gid(particles.fixed()));
  }

  SUBCASE("all_reduce_min") {
    const auto value = 10.0 + static_cast<double>(mpi::world.rank());
    CHECK(exchange.all_reduce_min(value) == 10.0);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
