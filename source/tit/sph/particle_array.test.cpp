/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <ranges>
#include <vector>

#include "tit/core/type.hpp"
#include "tit/core/vec.hpp"
#include "tit/sph/field.hpp"
#include "tit/sph/particle_array.hpp"
#include "tit/testing/test.hpp"

namespace tit {
namespace {

// Minimal set of equations to instantiate a particle array with.
struct TestEquations {
  static constexpr auto required_fields = TypeSet{h, m, sph::r, rho};
  static constexpr auto modified_fields = TypeSet{sph::r, rho};
};

auto make_test_array() {
  return sph::ParticleArray{Space<double, 2>{}, TestEquations{}};
}

using TestArray = decltype(make_test_array());

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("sph::ParticleArray::append") {
  auto particles = make_test_array();

  // Append some particles: fluid first, then fixed, then fluid again to
  // exercise the insertion in the middle of the storage.
  for (const auto index : std::views::iota(0, 3)) {
    const auto a = particles.append(sph::ParticleType::fluid);
    rho[a] = 100.0 + index;
  }
  for (const auto index : std::views::iota(0, 2)) {
    const auto a = particles.append(sph::ParticleType::fixed);
    rho[a] = 200.0 + index;
  }
  const auto a = particles.append(sph::ParticleType::fluid);
  rho[a] = 103.0;

  // Check the sizes and the segment views.
  CHECK(particles.size() == 6);
  CHECK(particles.num_owned() == 6);
  CHECK(std::ranges::size(particles.fluid()) == 4);
  CHECK(std::ranges::size(particles.fixed()) == 2);
  CHECK(std::ranges::size(particles.all()) == 6);
  CHECK(std::ranges::size(particles.owned()) == 6);
  CHECK(std::ranges::empty(particles.ghost()));

  // Check that the fluid particles precede the fixed particles, and the
  // insertion in the middle did not corrupt the fixed particle fields.
  for (const auto b : particles.fluid()) {
    CHECK(b.is_fluid());
    CHECK_FALSE(b.is_fixed());
    CHECK(b.is_owned());
    CHECK(rho[b] >= 100.0);
    CHECK(rho[b] < 200.0);
  }
  for (const auto b : particles.fixed()) {
    CHECK(b.is_fixed());
    CHECK(rho[b] >= 200.0);
  }

  // Check that the global identifiers are unique and survive reordering:
  // the last appended fluid particle is stored before the fixed particles,
  // but its identifier must be the largest.
  std::vector<std::uint64_t> gids{};
  for (const auto b : particles.all()) gids.push_back(gid[b]);
  std::ranges::sort(gids);
  CHECK(std::ranges::adjacent_find(gids) == gids.end());
  const auto last_fluid = particles[3];
  CHECK(last_fluid.is_fluid());
  CHECK(gid[last_fluid] == 5);
}

TEST_CASE("sph::ParticleArray::append_n") {
  auto particles = make_test_array();

  // Bulk-append some particles.
  for (const auto [index, a] :
       std::views::enumerate(particles.append_n(sph::ParticleType::fluid, 4))) {
    rho[a] = static_cast<double>(index);
  }
  CHECK(particles.size() == 4);
  CHECK(std::ranges::size(particles.fluid()) == 4);
  for (const auto [index, a] : std::views::enumerate(particles.fluid())) {
    CHECK(rho[a] == static_cast<double>(index));
    CHECK(gid[a] == static_cast<std::uint64_t>(index));
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("sph::ParticleArray::append_ghosts_n") {
  auto particles = make_test_array();
  particles.append_n(sph::ParticleType::fluid, 3);
  particles.append_n(sph::ParticleType::fixed, 2);

  // Append the ghost particles.
  particles.append_ghosts_n(sph::ParticleType::fluid, 2);
  particles.append_ghosts_n(sph::ParticleType::fixed, 1);
  CHECK(particles.size() == 8);
  CHECK(particles.num_owned() == 5);
  CHECK(std::ranges::size(particles.ghost()) == 3);

  // Owned segment views must not be affected by the ghost particles.
  CHECK(std::ranges::size(particles.fluid()) == 3);
  CHECK(std::ranges::size(particles.fixed()) == 2);

  // Ghost particles carry types, and are reported as ghosts.
  const auto ghosts = particles.ghost();
  const auto ghost_fluid = *std::ranges::begin(ghosts);
  CHECK(ghost_fluid.is_ghost());
  CHECK_FALSE(ghost_fluid.is_owned());
  CHECK(ghost_fluid.is_fluid());
  const auto ghost_fixed = *std::ranges::prev(std::ranges::end(ghosts));
  CHECK(ghost_fixed.is_ghost());
  CHECK(ghost_fixed.is_fixed());

  // Clear the ghosts.
  particles.clear_ghosts();
  CHECK(particles.size() == 5);
  CHECK(particles.num_owned() == 5);
  CHECK(std::ranges::empty(particles.ghost()));
  CHECK(std::ranges::size(particles.fluid()) == 3);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("sph::ParticleArray::pack") {
  auto particles = make_test_array();
  for (const auto [index, a] :
       std::views::enumerate(particles.append_n(sph::ParticleType::fluid, 4))) {
    sph::r[a] = Vec{1.0 * static_cast<double>(index), 2.0};
    rho[a] = 1000.0 + static_cast<double>(index);
  }

  // Pack a couple of particles.
  constexpr auto fields = TypeSet{sph::r, rho};
  constexpr auto width = TestArray::pack_width(fields);
  CHECK(width == sizeof(Vec<double, 2>) + sizeof(double));
  const auto indices = std::to_array<std::size_t>({3, 1});
  std::vector<std::byte> buffer(indices.size() * width);
  particles.pack(indices, fields, buffer);

  // Unpack them into freshly-appended ghost particles.
  particles.append_ghosts_n(sph::ParticleType::fluid, 2);
  particles.unpack(4, 2, fields, buffer);
  const auto b = particles[4];
  CHECK(sph::r[b] == Vec{3.0, 2.0});
  CHECK(rho[b] == 1003.0);
  const auto c = particles[5];
  CHECK(sph::r[c] == Vec{1.0, 2.0});
  CHECK(rho[c] == 1001.0);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
