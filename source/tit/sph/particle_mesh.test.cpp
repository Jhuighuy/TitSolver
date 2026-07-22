/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <ranges>
#include <vector>

#include "tit/core/type.hpp"
#include "tit/core/vec.hpp"
#include "tit/geom/face_search.hpp"
#include "tit/geom/search.hpp"
#include "tit/geom/surface.hpp"
#include "tit/sph/field.hpp"
#include "tit/sph/particle_array.hpp"
#include "tit/sph/particle_mesh.hpp"
#include "tit/testing/test.hpp"

namespace tit::sph {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("sph::ParticleMesh builds active target rows with ghost neighbors") {
  ParticleArray particles{Space<double, 2>{},
                          ParticleLayout{TypeSet{h}, TypeSet{r}}};
  h[particles] = 1.0;

  auto a = particles.append(ParticleType::fluid, ParticleID{1});
  r[a] = {0.0, 0.0};
  auto b = particles.append(ParticleType::fluid, ParticleID{4});
  r[b] = {0.75, 0.0};
  auto fixed = particles.append(ParticleType::fixed, ParticleID{3});
  r[fixed] = {10.0, 0.0};
  auto ghost = particles.append_ghost(ParticleID{2});
  r[ghost] = {0.25, 0.0};

  const geom::Surface<Vec<double, 2>> domain{};
  ParticleMesh mesh{geom::GridSearch{1.0}, geom::GridFaceSearch{1.0}};
  mesh.update(domain, particles, [](auto /*particle*/) { return 1.0; });

  const auto neighbor_ids = [](auto neighbors) {
    return neighbors |
           std::views::transform([](auto particle) { return particle.id(); }) |
           std::ranges::to<std::vector>();
  };
  CHECK_RANGE_EQ(neighbor_ids(mesh[a]), {ParticleID{2}, ParticleID{4}});
  CHECK_RANGE_EQ(neighbor_ids(mesh[b]), {ParticleID{1}, ParticleID{2}});
  CHECK_RANGE_EMPTY(mesh[fixed]);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit::sph
