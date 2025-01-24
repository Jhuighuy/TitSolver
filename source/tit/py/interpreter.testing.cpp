/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <string>
#include <utility>

#include "tit/core/exception.hpp"
#include "tit/core/sys/utils.hpp"

#include "tit/py/interpreter.hpp"

#include "tit/py/interpreter.testing.hpp"

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
