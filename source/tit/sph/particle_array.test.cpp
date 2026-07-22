/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <array>
#include <cstddef>
#include <iterator>

#include "tit/core/type.hpp"
#include "tit/core/vec.hpp"
#include "tit/sph/field.hpp"
#include "tit/sph/particle_array.hpp"
#include "tit/testing/test.hpp"

namespace tit::sph {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

struct TestFields final {
  static constexpr auto modified_fields = TypeSet{m, r, v, rho};
};

using TestParticles = decltype(ParticleArray{
    Space<double, 2>{},
    ParticleLayout{TypeSet{h}, TestFields::modified_fields}});

static_assert(TestParticles::uniform_fields == TypeSet{h});
static_assert(TestParticles::varying_fields == TypeSet{m, r, v, rho});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("sph::ParticleArray append and typed ranges") {
  TestParticles particles{
      Space<double, 2>{},
      ParticleLayout{TypeSet{h}, TestFields::modified_fields}};
  h[particles] = 0.25;

  auto a = particles.append(ParticleType::fluid);
  m[a] = 1.0;
  r[a] = {1.0, 2.0};

  auto e = particles.append(ParticleType::fixed);
  m[e] = 2.0;
  r[e] = {3.0, 4.0};

  // Fluid particles remain before fixed particles even when appended later.
  auto b = particles.append(ParticleType::fluid);
  m[b] = 3.0;
  r[b] = {5.0, 6.0};

  CHECK(particles.size() == 3);
  CHECK(particles.num_owned() == 2);
  CHECK(particles.num_fixed() == 1);
  CHECK(particles.num_ghosts() == 0);
  CHECK(std::ranges::distance(particles.fluid()) == 2);
  CHECK(std::ranges::distance(particles.fixed()) == 1);
  CHECK(h[particles] == 0.25);

  CHECK(particles[0].is_fluid());
  CHECK(particles[0].is_owned());
  CHECK(particles[1].is_fluid());
  CHECK(particles[1].is_owned());
  CHECK(particles[2].is_fixed());
  CHECK_FALSE(particles[2].is_owned());
  CHECK(particles[0].id() == ParticleID{0});
  CHECK(particles[1].id() == ParticleID{2});
  CHECK(particles[2].id() == ParticleID{1});
  CHECK(m[particles[0]] == 1.0);
  CHECK(m[particles[1]] == 3.0);
  CHECK(m[particles[2]] == 2.0);
  CHECK(r[particles[0]] == Vec{1.0, 2.0});
  CHECK(r[particles[1]] == Vec{5.0, 6.0});
  CHECK(r[particles[2]] == Vec{3.0, 4.0});
}

TEST_CASE("sph::ParticleArray ghost lifetime and lookup") {
  TestParticles particles{
      Space<double, 2>{},
      ParticleLayout{TypeSet{h}, TestFields::modified_fields}};
  const auto owned = particles.append(ParticleType::fluid, ParticleID{7});
  const auto fixed = particles.append(ParticleType::fixed, ParticleID{11});
  const auto ghost = particles.append_ghost(ParticleID{42});

  CHECK(owned.is_owned());
  CHECK_FALSE(fixed.is_owned());
  CHECK(ghost.is_ghost());
  CHECK(ghost.is_fluid());
  CHECK(particles.num_owned() == 1);
  CHECK(particles.num_fixed() == 1);
  CHECK(particles.num_ghosts() == 1);
  CHECK(std::ranges::distance(particles.fluid()) == 1);
  CHECK(std::ranges::distance(particles.ghosts()) == 1);
  CHECK(particles.find(ParticleID{7}) == 0);
  CHECK(particles.find(ParticleID{11}) == 1);
  CHECK(particles.find(ParticleID{42}) == 2);
  CHECK_FALSE(particles.find(ParticleID{100}).has_value());

  particles.clear_ghosts();
  CHECK(particles.size() == 2);
  CHECK(particles.num_ghosts() == 0);
  CHECK_FALSE(particles.find(ParticleID{42}).has_value());
}

TEST_CASE("sph::ParticleArray copy preserves values") {
  TestParticles particles{
      Space<double, 2>{},
      ParticleLayout{TypeSet{h}, TestFields::modified_fields}};
  h[particles] = 0.5;
  auto a = particles.append(ParticleType::fluid);
  m[a] = 2.0;
  r[a] = {1.0, 2.0};
  v[a] = {3.0, 4.0};
  rho[a] = 5.0;

  const auto copy = particles;
  REQUIRE(copy.size() == 1);
  const auto copy_a = copy[0];
  CHECK(copy_a.id() == ParticleID{0});
  CHECK(h[copy] == 0.5);
  CHECK(m[copy_a] == 2.0);
  CHECK(r[copy_a] == Vec{1.0, 2.0});
  CHECK(v[copy_a] == Vec{3.0, 4.0});
  CHECK(rho[copy_a] == 5.0);
}

TEST_CASE("sph::ParticleArray packs migration and halo records") {
  TestParticles source{Space<double, 2>{},
                       ParticleLayout{TypeSet{h}, TestFields::modified_fields}};
  const auto a = source.append(ParticleType::fluid, ParticleID{17});
  m[a] = 2.0;
  r[a] = {1.0, 2.0};
  v[a] = {3.0, 4.0};
  rho[a] = 5.0;

  std::array<std::byte, TestParticles::packed_particle_size> buffer{};
  source.pack(a.index(), buffer);

  TestParticles destination{
      Space<double, 2>{},
      ParticleLayout{TypeSet{h}, TestFields::modified_fields}};
  const auto ghost = destination.append_ghost_packed(buffer);
  CHECK(ghost.id() == ParticleID{17});
  CHECK(ghost.is_ghost());
  CHECK(m[ghost] == 2.0);
  CHECK(r[ghost] == Vec{1.0, 2.0});
  CHECK(v[ghost] == Vec{3.0, 4.0});
  CHECK(rho[ghost] == 5.0);

  destination.clear_ghosts();
  const auto owned = destination.append_packed(ParticleType::fluid, buffer);
  CHECK(owned.id() == ParticleID{17});
  CHECK(owned.is_owned());
  destination.erase_owned(owned.index());
  CHECK(destination.size() == 0);
  CHECK_FALSE(destination.find(ParticleID{17}).has_value());
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit::sph
