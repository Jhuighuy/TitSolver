/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define TIT_PYTHON_INTERPRETER
#include "tit/python/python_h.hpp" // must be first.

#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include "tit/core/checks.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/main_func.hpp"
#include "tit/core/str_utils.hpp"
#include "tit/core/sys/utils.hpp"

#include "tit/python/interpreter.hpp"
#include "tit/python/objects.hpp"

namespace tit::py::embed {

// NOLINTBEGIN(*-include-cleaner)

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Config::Config() : config_{new PyConfig{}} {
  PyConfig_InitIsolatedConfig(config_.get());
}

void Config::Cleaner_::operator()(PyConfig* config) const {
  if (config != nullptr) PyConfig_Clear(config);
  std::default_delete<PyConfig>::operator()(config);
}

auto Config::base() const noexcept -> PyConfig* {
  TIT_ASSERT(config_ != nullptr, "Config is not initialized!");
  return config_.get();
}

void Config::set_home(CStrView home) const {
  const auto status =
      PyConfig_SetBytesString(base(), &base()->home, home.c_str());
  if (PyStatus_IsError(status) == 0) return;
  TIT_THROW("Failed to set Python home directory to '{}': {}: {}.",
            home,
            status.func,
            status.err_msg);
}

void Config::set_prog_name(CStrView name) const {
  const auto status =
      PyConfig_SetBytesString(base(), &base()->program_name, name.c_str());
  if (PyStatus_IsError(status) == 0) return;
  TIT_THROW("Failed to set Python program name to '{}': {}: {}.",
            name,
            status.func,
            status.err_msg);
}

void Config::set_cmd_args(CmdArgs args) const {
  // Enable parsing of the command line arguments according to the conventions:
  // see https://docs.python.org/3/using/cmdline.html
  base()->parse_argv = 1;

  // Set the command line arguments.
  const auto status = PyConfig_SetBytesArgv(
      base(),
      static_cast<int>(args.size()),
      const_cast<char* const*>(args.data())); // NOLINT(*-const-cast)
  if (PyStatus_IsError(status) == 0) return;
  TIT_THROW("Failed to set Python command line arguments: {}: {}.",
            status.func,
            status.err_msg);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

bool BasicInterpreter::initialized_ = false;

BasicInterpreter::BasicInterpreter(Config config) : config_{std::move(config)} {
  TIT_ASSERT(!initialized_, "Python interpreter was already initialized!");
  initialized_ = true;

  // Initialize the Python interpreter.
  const auto status = Py_InitializeFromConfig(config_.base());
  if (PyStatus_IsError(status) != 0) {
    TIT_THROW("Failed to initialize Python interpreter: {}: {}.",
              status.func,
              status.err_msg);
  }
}

BasicInterpreter::~BasicInterpreter() {
  Py_Finalize();
  initialized_ = false;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Interpreter::Interpreter(Config config)
    : BasicInterpreter{std::move(config)},
      globals_{import_("__main__").dict().release()} {
#ifdef TIT_HAVE_GCOV
  start_coverage_report_();
#endif
}

Interpreter::~Interpreter() {
#ifdef TIT_HAVE_GCOV
  stop_coverage_report_();
#endif
}

void Interpreter::append_path(CStrView path) {
  static_cast<void>(this);
  const auto sys = import_("sys");
  const auto sys_path = cast<List>(sys.attr("path"));
  sys_path.append(Str(path));
}

auto Interpreter::globals() const -> Dict {
  return cast<Dict>(Object{Py_XNewRef(globals_)});
}

auto Interpreter::exec(CStrView statement) -> bool {
  const auto* str = statement.c_str();
  const ObjectPtr result(PyRun_String(str,
                                      /*start=*/Py_file_input,
                                      /*globals=*/globals_,
                                      /*locals=*/globals_));
  if (!result.valid()) {
    PyErr_Print();
    return false;
  }
  return true;
}

auto Interpreter::exec_file(CStrView file_name) -> bool {
  const auto file = open_file(file_name, "r");
  const ObjectPtr result(PyRun_File(/*fp=*/file.get(),
                                    /*filename=*/file_name.c_str(),
                                    /*start=*/Py_file_input,
                                    /*globals=*/globals_,
                                    /*locals=*/globals_));
  if (!result.valid()) {
    PyErr_Print();
    return false;
  }
  return true;
}

void Interpreter::start_coverage_report_() const {
  // Locate the configuration file.
  const auto source_dir = get_env("SOURCE_DIR");
  if (!source_dir.has_value()) {
    TIT_THROW("Environment variable 'SOURCE_DIR' must be set when "
              "running `titback` compiled for coverage analysis.");
  }
  const auto config_file = std::string{*source_dir} + "/pyproject.toml";

  // Create the coverage report and start it.
  const auto coverage = import_("coverage");
  const Dict kwargs;
  kwargs["branch"] = true;
  kwargs["config_file"] = config_file;
  const auto coverage_report =
      coverage.attr("Coverage").tp_call(Tuple{}, kwargs);
  coverage_report.attr("start")();
  globals()["__coverage_report"] = coverage_report;
}

void Interpreter::stop_coverage_report_() const {
  // Some of our tests emit warnings for missing coverage data, ignore them.
  const auto warnings = import_("warnings");
  warnings.attr("filterwarnings").tp_call(py::make_tuple("ignore"));

  // Stop the coverage report and save it.
  const Object coverage_report = globals()["__coverage_report"];
  coverage_report.attr("stop")();
  coverage_report.attr("save")();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// NOLINTEND(*-include-cleaner)

} // namespace tit::py::embed
