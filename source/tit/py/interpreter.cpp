/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define TIT_PYTHON_INTERPRETER // Disable limited API.

#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include "tit/core/checks.hpp"
#include "tit/core/cmd.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/str_utils.hpp"
#include "tit/core/sys/utils.hpp"

#include "tit/py/_python.hpp"
#include "tit/py/interpreter.hpp"

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

auto Config::get() const noexcept -> PyConfig* {
  TIT_ASSERT(config_ != nullptr, "Config is not initialized!");
  return config_.get();
}

void Config::set_home(CStrView home) const {
  const auto status =
      PyConfig_SetBytesString(get(), &get()->home, home.c_str());
  if (PyStatus_IsError(status) == 0) return;
  TIT_THROW("Failed to set Python home directory to '{}': {}: {}.",
            home,
            status.func,
            status.err_msg);
}

void Config::set_prog_name(CStrView name) const {
  const auto status =
      PyConfig_SetBytesString(get(), &get()->program_name, name.c_str());
  if (PyStatus_IsError(status) == 0) return;
  TIT_THROW("Failed to set Python program name to '{}': {}: {}.",
            name,
            status.func,
            status.err_msg);
}

void Config::set_cmd_args(CmdArgs args) const {
  // Enable parsing of the command line arguments according to the
  // conventions. See https://docs.python.org/3/using/cmdline.html for details.
  get()->parse_argv = 1;

  // Set the command line arguments.
  const auto status = PyConfig_SetBytesArgv(get(), args.argc(), args.argv());
  if (PyStatus_IsError(status) == 0) return;
  TIT_THROW("Failed to set Python command line arguments: {}: {}.",
            status.func,
            status.err_msg);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

BasicInterpreter::BasicInterpreter(Config config) : config_{std::move(config)} {
  const auto status = Py_InitializeFromConfig(config_.get());
  if (PyStatus_IsError(status) != 0) {
    TIT_THROW("Failed to initialize Python interpreter: {}: {}.",
              status.func,
              status.err_msg);
  }
}

BasicInterpreter::~BasicInterpreter() {
  Py_Finalize();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Interpreter::Interpreter(Config config) : BasicInterpreter{std::move(config)} {
  // Get the globals of the main module.
  auto* const main_module = PyImport_AddModule("__main__");
  globals_ = PyModule_GetDict(main_module);

#ifdef TIT_HAVE_GCOV
  start_coverage_report_();
#endif
}

Interpreter::~Interpreter() {
#ifdef TIT_HAVE_GCOV
  stop_coverage_report_();
#endif
}

void Interpreter::append_path(CStrView path) const {
  static_cast<void>(this);
  auto* const sys = PyImport_ImportModule("sys");
  auto* const sys_path = PyObject_GetAttrString(sys, "path");
  auto* const path_str = PyUnicode_FromString(path.c_str());
  PyList_Append(sys_path, path_str);
  Py_DECREF(path_str);
  Py_DECREF(sys_path);
  Py_DECREF(sys);
}

auto Interpreter::exec(CStrView stmt) const -> bool {
  auto* const result = PyRun_String(stmt.c_str(),
                                    /*start=*/Py_file_input,
                                    /*globals=*/globals_,
                                    /*locals=*/globals_);
  if (result == nullptr) {
    PyErr_Print();
    return false;
  }
  Py_DECREF(result);
  return true;
}

auto Interpreter::exec_file(CStrView file_name) const -> bool {
  const auto file = open_file(file_name, "r");
  auto* const result = PyRun_File(/*fp=*/file.get(),
                                  /*filename=*/file_name.c_str(),
                                  /*start=*/Py_file_input,
                                  /*globals=*/globals_,
                                  /*locals=*/globals_);
  if (result == nullptr) {
    PyErr_Print();
    return false;
  }
  Py_DECREF(result);
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
  auto* const coverage = PyImport_ImportModule("coverage");
  auto* const coverage_class = PyObject_GetAttrString(coverage, "Coverage");
  auto* const kwargs = PyDict_New();
  {
    auto* const branch = PyBool_FromLong(1);
    PyDict_SetItemString(kwargs, "branch", branch);
    Py_DECREF(branch);
    auto* const config_file_str = PyUnicode_FromString(config_file.c_str());
    PyDict_SetItemString(kwargs, "config_file", config_file_str);
    Py_DECREF(config_file_str);
  }
  PyObject* coverage_report =
      PyObject_Call(coverage_class, PyTuple_New(0), kwargs);
  Py_DECREF(kwargs);
  Py_DECREF(coverage_class);
  Py_DECREF(coverage);
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  Py_DECREF(PyObject_CallMethod(coverage_report, "start", nullptr));
  PyDict_SetItemString(globals_, "__coverage_report", coverage_report);
  Py_DECREF(coverage_report);
}

void Interpreter::stop_coverage_report_() const {
  // Some of our tests emit warnings for missing coverage data, ignore them.
  auto* warnings = PyImport_ImportModule("warnings");
  auto* filterwarnings = PyObject_GetAttrString(warnings, "filterwarnings");
  auto* ignore_str = PyUnicode_FromString("ignore");
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  Py_DECREF(PyObject_CallFunctionObjArgs(filterwarnings, ignore_str, nullptr));
  Py_DECREF(ignore_str);
  Py_DECREF(filterwarnings);
  Py_DECREF(warnings);

  // Stop the coverage report and save it.
  auto* coverage_report = PyDict_GetItemString(globals_, "__coverage_report");
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  Py_DECREF(PyObject_CallMethod(coverage_report, "stop", nullptr));
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
  Py_DECREF(PyObject_CallMethod(coverage_report, "save", nullptr));
  Py_DECREF(coverage_report);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// NOLINTEND(*-include-cleaner)

} // namespace tit::py::embed
