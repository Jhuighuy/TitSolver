/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <vector>

#include "tit/core/math.hpp"
#include "tit/core/vec.hpp"
#include "tit/sph/boundary_extrapolation.hpp"
#include "tit/testing/test.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

struct ScalarSample {
  double x;
  double weight;
  double value;
};

TEST_CASE("sph::robin_extrapolate") {
  const auto x = [](const ScalarSample& sample) { return sample.x; };
  const auto weight = [](const ScalarSample& sample) { return sample.weight; };
  const auto value = [](const ScalarSample& sample) { return sample.value; };

  SUBCASE("recovers wall value from an order-2 Robin fit") {
    constexpr double beta_0 = 3.0;
    constexpr double beta_2 = -2.0;
    constexpr double k = 2.0;
    constexpr double lambda = 0.5;
    constexpr double psi = 4.0;

    std::vector<ScalarSample> samples;
    for (const double x_i : {0.25, 0.5, 0.75, 1.0}) {
      samples.push_back({
          .x = x_i,
          .weight = 1.0,
          .value = beta_2 * pow2(x_i) + psi / k * x_i +
                   beta_0 * (1 - lambda / k * x_i),
      });
    }

    const auto result = sph::robin_extrapolate<2>(samples,
                                                  x,
                                                  weight,
                                                  value,
                                                  k,
                                                  lambda,
                                                  psi,
                                                  0.0);
    CHECK_APPROX_EQ(result, beta_0);
  }

  SUBCASE("falls back from order 2 to order 1") {
    const std::vector samples{
        ScalarSample{.x = 0.5, .weight = 1.0, .value = 7.0}};
    const auto result = sph::robin_extrapolate<2>(samples,
                                                  x,
                                                  weight,
                                                  value,
                                                  1.0,
                                                  0.0,
                                                  0.0,
                                                  -1.0);
    CHECK_APPROX_EQ(result, 7.0);
  }

  SUBCASE("uses caller fallback when no weighted samples exist") {
    const std::vector samples{
        ScalarSample{.x = 0.5, .weight = 0.0, .value = 7.0}};
    const auto result = sph::robin_extrapolate<2>(samples,
                                                  x,
                                                  weight,
                                                  value,
                                                  1.0,
                                                  0.0,
                                                  0.0,
                                                  -1.0);
    CHECK_APPROX_EQ(result, -1.0);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("sph::homogeneous_neumann_extrapolate") {
  const std::vector samples{
      ScalarSample{.x = 0.0, .weight = 2.0, .value = 1.0},
      ScalarSample{.x = 0.0, .weight = 3.0, .value = 5.0},
  };
  const auto result = sph::homogeneous_neumann_extrapolate(
      samples,
      [](const ScalarSample& sample) { return sample.weight; },
      [](const ScalarSample& sample) { return sample.value; },
      0.0);
  CHECK_APPROX_EQ(result, 3.4);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

struct VectorSample {
  double x;
  double weight;
  Vec<double, 2> value;
};

TEST_CASE("sph::robin_extrapolate supports vector values") {
  constexpr Vec beta_0{3.0, -1.0};
  constexpr Vec beta_2{-2.0, 4.0};
  constexpr double k = 2.0;
  constexpr double lambda = 0.5;
  constexpr double psi = 4.0;

  std::vector<VectorSample> samples;
  for (const double x_i : {0.25, 0.5, 0.75, 1.0}) {
    samples.push_back({
        .x = x_i,
        .weight = 1.0,
        .value = beta_2 * pow2(x_i) + Vec{psi / k * x_i, psi / k * x_i} +
                 beta_0 * (1 - lambda / k * x_i),
    });
  }

  const auto result = sph::robin_extrapolate<2>(
      samples,
      [](const VectorSample& sample) { return sample.x; },
      [](const VectorSample& sample) { return sample.weight; },
      [](const VectorSample& sample) { return sample.value; },
      k,
      lambda,
      psi,
      Vec<double, 2>{});
  CHECK_APPROX_EQ(result, beta_0);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
