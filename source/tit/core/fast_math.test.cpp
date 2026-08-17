/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <numbers>

#include "tit/core/fast_math.hpp"
#include "tit/testing/test.hpp"

namespace tit {
namespace {

#define FLOAT_TYPES float, double

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE_TEMPLATE("fast_atan2", Float, FLOAT_TYPES) {
  const auto pi = std::numbers::pi_v<Float>;
  CHECK_APPROX_EQ(fast_atan2(Float{0.0}, Float{1.0}), Float{0.0});
  CHECK_APPROX_EQ(fast_atan2(Float{0.0}, Float{-1.0}), pi);
  CHECK_APPROX_EQ(fast_atan2(Float{1.0}, Float{0.0}), pi / 2);
  CHECK_APPROX_EQ(fast_atan2(Float{1.0}, Float{1.0}), pi / 4);
  CHECK_APPROX_EQ(fast_atan2(Float{1.0}, Float{1.0}), pi / 4);
  CHECK_APPROX_EQ(fast_atan2(Float{-1.0}, Float{-1.0}), -3 * pi / 4);
  CHECK_APPROX_EQ(fast_atan2(Float{-1.0}, Float{0.0}), -pi / 2);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE_TEMPLATE("fast_log1p", Float, FLOAT_TYPES) {
  CHECK_APPROX_EQ(fast_log1p(Float{0.0}), Float{0.0});
  CHECK_APPROX_EQ(fast_log1p(Float{1.0}), std::numbers::ln2_v<Float>);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
