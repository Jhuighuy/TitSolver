/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <string>
#include <utility>

#include "tit/core/exception.hpp"
#include "tit/core/sys/utils.hpp"

#include "tit/py/embed.hpp"

#include "tit/py/_embed/interpreter.testing.hpp"

namespace tit::testing {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace {

auto test_interpreter = [] { // NOLINT(cert-*)
  const auto install_dir = get_env("INSTALL_DIR");
  if (!install_dir.has_value()) {
    TIT_THROW("Environment variable 'INSTALL_DIR' must be set when running "
              "`tit::py` tests.");
  }
  py::embed::Config config;
  config.set_home(std::string{install_dir.value()} + "/python");
  return py::embed::Interpreter{std::move(config)};
}();

} // namespace

auto interpreter() -> py::embed::Interpreter& {
  return test_interpreter;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::testing