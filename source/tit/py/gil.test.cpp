/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <string>
#include <thread>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/missing.hpp" // IWYU pragma: keep

#include "tit/py/gil.hpp"
#include "tit/py/sequence.hpp"

#include "tit/testing/test.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("py::ReleaseGIL") {
  const py::ReleaseGIL release_gil{};
  std::vector<std::jthread> threads(4);
  for (size_t i = 0; i < threads.size(); ++i) {
    threads[i] = std::jthread{[i] {
      const py::AcquireGIL acquire_gil{};
      CHECK(py::Str{std::to_string(i)} + py::Str{"_test"} ==
            py::Str{std::to_string(i) + "_test"});
    }};
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
