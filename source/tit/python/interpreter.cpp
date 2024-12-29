/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

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
#include "tit/python/nb_config.hpp"

TIT_NANOBIND_INCLUDE_BEGIN
#include <nanobind/nanobind.h>
TIT_NANOBIND_INCLUDE_END

namespace tit::python {

namespace nb = nanobind;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// NOLINTBEGIN(*-include-cleaner)

Config::Config() : config_{new PyConfig{}} {  // NOLINT(*-include-cleaner)
  PyConfig_InitIsolatedConfig(config_.get()); // NOLINT(*-include-cleaner)
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

// NOLINTEND(*-include-cleaner)

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

bool Interpreter::initialized_ = false;

// NOLINTBEGIN(*-include-cleaner)

Interpreter::Interpreter(Config config) : config_{std::move(config)} {
  TIT_ASSERT(!initialized_, "Python interpreter was already initialized!");
  initialized_ = true;

  // Initialize the Python interpreter.
  const auto status = Py_InitializeFromConfig(config_.base());
  if (PyStatus_IsError(status) != 0) {
    TIT_THROW("Failed to initialize Python interpreter: {}: {}.",
              status.func,
              status.err_msg);
  }

  // Get the globals of the main module.
  const auto main_module = nb::module_::import_("__main__");
  const auto globals = main_module.attr("__dict__");
  globals_ = globals.ptr();

#ifdef TIT_HAVE_GCOV
  // Start the coverage report.
  start_coverage_report_();
#endif
}

Interpreter::~Interpreter() {
#ifdef TIT_HAVE_GCOV
  // Finalize the coverage report.
  stop_coverage_report_();
#endif

  // Finalize the Python interpreter.
  Py_Finalize();
  initialized_ = false;
}

void Interpreter::append_path(CStrView path) const {
  static_cast<void>(this);
  const auto sys = nb::module_::import_("sys");
  auto sys_path = nb::cast<nb::list>(sys.attr("path"));
  sys_path.append(path.c_str());
}

auto Interpreter::exec(CStrView statement) const -> bool {
  const auto result = nb::steal(PyRun_String(statement.c_str(),
                                             /*start=*/Py_file_input,
                                             /*globals=*/globals_,
                                             /*locals=*/globals_));
  if (!result.is_valid()) {
    PyErr_Print();
    return false;
  }
  return true;
}

auto Interpreter::exec_file(CStrView file_name) const -> bool {
  const auto file = open_file(file_name, "r");
  const auto result = nb::steal(PyRun_File(/*fp=*/file.get(),
                                           /*filename=*/file_name.c_str(),
                                           /*start=*/Py_file_input,
                                           /*globals=*/globals_,
                                           /*locals=*/globals_));
  if (!result.is_valid()) {
    PyErr_Print();
    return false;
  }
  return true;
}

// NOLINTEND(*-include-cleaner)

void Interpreter::start_coverage_report_() const {
  // Locate the configuration file.
  const auto source_dir = get_env("SOURCE_DIR");
  if (!source_dir.has_value()) {
    TIT_THROW("Environment variable 'SOURCE_DIR' must be set when "
              "running `titback` compiled for coverage analysis.");
  }
  const auto config_file = std::string{*source_dir} + "/pyproject.toml";

  // Create the coverage report and start it.
  const auto coverage = nb::module_::import_("coverage");
  const auto coverage_report = coverage.attr("Coverage")( //
      nb::arg("branch") = true,
      nb::arg("config_file") = config_file.c_str());
  coverage_report.attr("start")();
  const auto globals = nb::cast<nb::dict>(nb::borrow(globals_));
  globals["__coverage_report"] = coverage_report;
}

void Interpreter::stop_coverage_report_() const {
  // Some of our tests emit warnings for missing coverage data, ignore them.
  const auto warnings = nb::module_::import_("warnings");
  warnings.attr("filterwarnings")("ignore");

  // Stop the coverage report and save it.
  const auto globals = nb::cast<nb::dict>(nb::borrow(globals_));
  const auto coverage_report = globals["__coverage_report"];
  coverage_report.attr("stop")();
  coverage_report.attr("save")();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::python
