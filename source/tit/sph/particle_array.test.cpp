/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <cstddef>
#include <ranges>

#include "tit/core/type.hpp"
#include "tit/core/vec.hpp"
#include "tit/sph/field.hpp"
#include "tit/sph/particle_array.hpp"
#include "tit/testing/test.hpp"

namespace tit::sph {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

struct TestFields final {
  static constexpr auto required_fields = TypeSet{h, m, r, v, rho};
  static constexpr auto modified_fields = TypeSet{m, r, v, rho};
};

using TestParticles = decltype(ParticleArray{Space<double, 2>{}, TestFields{}});

static_assert(TestParticles::uniform_fields == TypeSet{h});
static_assert(TestParticles::varying_fields == TypeSet{m, r, v, rho});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("sph::ParticleArray append and typed ranges") {
  TestParticles particles{Space<double, 2>{}, TestFields{}};
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
  CHECK(std::ranges::distance(particles.fluid()) == 2);
  CHECK(std::ranges::distance(particles.fixed()) == 1);
  CHECK(h[particles] == 0.25);

  CHECK(particles[0].is_fluid());
  CHECK(particles[1].is_fluid());
  CHECK(particles[2].is_fixed());
  CHECK(m[particles[0]] == 1.0);
  CHECK(m[particles[1]] == 3.0);
  CHECK(m[particles[2]] == 2.0);
  CHECK(r[particles[0]] == Vec{1.0, 2.0});
  CHECK(r[particles[1]] == Vec{5.0, 6.0});
  CHECK(r[particles[2]] == Vec{3.0, 4.0});
}

TEST_CASE("sph::ParticleArray copy preserves values") {
  TestParticles particles{Space<double, 2>{}, TestFields{}};
  h[particles] = 0.5;
  auto a = particles.append(ParticleType::fluid);
  m[a] = 2.0;
  r[a] = {1.0, 2.0};
  v[a] = {3.0, 4.0};
  rho[a] = 5.0;

  const auto copy = particles;
  REQUIRE(copy.size() == 1);
  const auto copy_a = copy[0];
  CHECK(h[copy] == 0.5);
  CHECK(m[copy_a] == 2.0);
  CHECK(r[copy_a] == Vec{1.0, 2.0});
  CHECK(v[copy_a] == Vec{3.0, 4.0});
  CHECK(rho[copy_a] == 5.0);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit::sph
