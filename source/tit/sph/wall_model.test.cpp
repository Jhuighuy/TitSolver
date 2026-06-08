/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <vector>

#include "tit/core/vec.hpp"
#include "tit/sph/wall_model.hpp"
#include "tit/testing/test.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

struct WallSample {
  double x;
  double weight;
  Vec<double, 2> v;
  double T;
};

TEST_CASE("sph::FreeSlipVelocityWallModel") {
  const std::vector samples{
      WallSample{.x = 0.0, .weight = 1.0, .v = Vec{2.0, 1.0}, .T = 0.0},
      WallSample{.x = 0.0, .weight = 3.0, .v = Vec{4.0, 3.0}, .T = 0.0},
  };
  const sph::FreeSlipVelocityWallModel<double> model;
  const auto value = model.velocity(
      samples,
      [](const WallSample& sample) { return sample.x; },
      [](const WallSample& sample) { return sample.weight; },
      [](const WallSample& sample) { return sample.v; },
      Vec{0.0, 1.0});
  CHECK_APPROX_EQ(value, Vec{3.5, 0.0});
  CHECK_APPROX_EQ(model.shear_stress(1000.0, 0.001, 0.5, Vec{3.0, 0.0}),
                  Vec<double, 2>{});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("sph::LaminarNoSlipVelocityWallModel") {
  const sph::LaminarNoSlipVelocityWallModel<double> model;
  const auto value = model.velocity(
      std::vector<WallSample>{},
      [](const WallSample& sample) { return sample.x; },
      [](const WallSample& sample) { return sample.weight; },
      [](const WallSample& sample) { return sample.v; },
      Vec{0.0, 1.0});
  CHECK_APPROX_EQ(value, Vec<double, 2>{});
  CHECK_APPROX_EQ(model.shear_stress(1000.0, 2.0, 0.5, Vec{3.0, 0.0}),
                  Vec{12.0, 0.0});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("sph::LogLawVelocityWallModel") {
  const sph::LogLawVelocityWallModel<double> model;
  const auto stress = model.shear_stress(1000.0, 0.001, 0.01, Vec{2.0, 0.0});
  CHECK(norm(stress) > 0.0);
  CHECK(dot(stress, Vec{2.0, 0.0}) > 0.0);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("sph::AdiabaticThermalWallModel") {
  const std::vector samples{
      WallSample{.x = 0.25, .weight = 1.0, .v = {}, .T = 300.0},
      WallSample{.x = 0.50, .weight = 1.0, .v = {}, .T = 300.0},
      WallSample{.x = 0.75, .weight = 1.0, .v = {}, .T = 300.0},
  };
  const sph::AdiabaticThermalWallModel<double> model;
  const auto T_wall = model.temperature(
      samples,
      [](const WallSample& sample) { return sample.x; },
      [](const WallSample& sample) { return sample.weight; },
      [](const WallSample& sample) { return sample.T; },
      0.6,
      0.0);
  CHECK_APPROX_EQ(T_wall, 300.0);
  CHECK_APPROX_EQ(
      model.heat_flux(0.6, 320.0, 300.0, Vec{2.0, 0.0}, Vec{1.0, 0.0}),
      0.0);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("sph::FixedTemperatureWallModel") {
  const sph::FixedTemperatureWallModel<double> model{300.0};
  CHECK_APPROX_EQ(model.temperature(
                      std::vector<WallSample>{},
                      [](const WallSample& sample) { return sample.x; },
                      [](const WallSample& sample) { return sample.weight; },
                      [](const WallSample& sample) { return sample.T; },
                      0.6,
                      0.0),
                  300.0);
  CHECK_APPROX_EQ(
      model.heat_flux(0.5, 320.0, 300.0, Vec{2.0, 0.0}, Vec{1.0, 0.0}),
      5.0);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("sph::HeatFluxThermalWallModel") {
  constexpr double q = 6.0;
  constexpr double k = 2.0;
  constexpr double T_0 = 10.0;
  const std::vector samples{
      WallSample{.x = 0.25, .weight = 1.0, .v = {}, .T = T_0 + q / k * 0.25},
      WallSample{.x = 0.50, .weight = 1.0, .v = {}, .T = T_0 + q / k * 0.50},
      WallSample{.x = 0.75, .weight = 1.0, .v = {}, .T = T_0 + q / k * 0.75},
  };
  const sph::HeatFluxThermalWallModel<double> model{q};
  const auto T_wall = model.temperature(
      samples,
      [](const WallSample& sample) { return sample.x; },
      [](const WallSample& sample) { return sample.weight; },
      [](const WallSample& sample) { return sample.T; },
      k,
      0.0);
  CHECK_APPROX_EQ(T_wall, T_0);
  CHECK_APPROX_EQ(model.heat_flux(k, 0.0, T_wall, Vec{1.0, 0.0}, Vec{1.0, 0.0}),
                  q);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("sph::RobinThermalWallModel") {
  const sph::RobinThermalWallModel<double> model{3.0, 11.0};
  CHECK_APPROX_EQ(model.heat_flux(2.0, 0.0, 2.0, Vec{1.0, 0.0}, Vec{1.0, 0.0}),
                  5.0);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
