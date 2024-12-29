/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <cstdio>
#include <format>
#include <memory>
#include <utility>

#include <Python.h> // IWYU pragma: keep

#include "tit/core/checks.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/main_func.hpp"
#include "tit/core/str_utils.hpp"
#include "tit/core/sys/utils.hpp"

#include "titback/interpreter.hpp"

namespace tit::back {

// NOLINTBEGIN(*-include-cleaner)

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

PythonConfig::PythonConfig() : config_{new PyConfig{}} {
  PyConfig_InitIsolatedConfig(config_.get());
}

void PythonConfig::Cleaner_::operator()(PyConfig* config) const {
  if (config != nullptr) PyConfig_Clear(config);
  std::default_delete<PyConfig>::operator()(config);
}

auto PythonConfig::base() const noexcept -> PyConfig* {
  TIT_ASSERT(config_ != nullptr, "Config is not initialized!");
  return config_.get();
}

void PythonConfig::set_home(CStrView home) const {
  const auto status =
      PyConfig_SetBytesString(base(), &base()->home, home.c_str());
  if (PyStatus_IsError(status) == 0) return;
  TIT_THROW("Failed to set Python home directory to '{}': {}: {}.",
            home,
            status.func,
            status.err_msg);
}

void PythonConfig::set_prog_name(CStrView name) const {
  const auto status =
      PyConfig_SetBytesString(base(), &base()->program_name, name.c_str());
  if (PyStatus_IsError(status) == 0) return;
  TIT_THROW("Failed to set Python program name to '{}': {}: {}.",
            name,
            status.func,
            status.err_msg);
}

void PythonConfig::set_cmd_args(CmdArgs args) const {
  const auto status = PyConfig_SetBytesArgv(
      base(),
      static_cast<Py_ssize_t>(args.size()),
      const_cast<char* const*>(args.data())); // NOLINT(*-const-cast)
  if (PyStatus_IsError(status) == 0) return;
  TIT_THROW("Failed to set Python command line arguments: {}: {}.",
            status.func,
            status.err_msg);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

PythonInterpreter::PythonInterpreter(PythonConfig config)
    : config_{std::move(config)} {
  // Initialize the Python interpreter.
  const auto status = Py_InitializeFromConfig(config_.base());
  if (PyStatus_IsError(status) != 0) {
    TIT_THROW("Failed to initialize Python interpreter: {}: {}.",
              status.func,
              status.err_msg);
  }

  // Get the globals of the main module.
  auto* const main_module = PyImport_AddModule("__main__");
  if (main_module == nullptr) {
    PyErr_Print();
    TIT_THROW("Failed to import the main module.");
  }
  globals_ = PyModule_GetDict(main_module);
  if (globals_ == nullptr) {
    PyErr_Print();
    TIT_THROW("Failed to get the main module globals.");
  }

  // Initialize the coverage report.
#ifdef TIT_HAVE_GCOV
  exec(R"PY(if True:
    import os
    import coverage

    # Start the coverage report.
    cov = coverage.Coverage(
        config_file=os.path.join(os.environ["SOURCE_DIR"], "pyproject.toml"),
        branch=True,
    )
    cov.start()
  )PY");
#endif
}

PythonInterpreter::~PythonInterpreter() {
  // Finalize the coverage report.
#ifdef TIT_HAVE_GCOV
  exec(R"PY(if True:
    # Some of our tests will emit warnings for missing coverage data.
    # This is expected, and we can safely ignore them.
    import warnings
    warnings.filterwarnings("ignore")

    # Write the coverage report.
    cov.stop()
    cov.save()
  )PY");
#endif

  // Finalize the Python interpreter.
  Py_Finalize();
}

void PythonInterpreter::append_path(CStrView path) const {
  exec(std::format("import sys; sys.path.append('{}')", path.c_str()));
}

auto PythonInterpreter::exec(CStrView statement) const -> bool {
  auto* const result = PyRun_StringFlags(statement.c_str(),
                                         /*start=*/Py_file_input,
                                         /*globals=*/globals_,
                                         /*locals=*/globals_,
                                         /*flags=*/nullptr);
  if (result != nullptr) {
    Py_DECREF(result);
    return true;
  }

  PyErr_Print();
  return false;
}

auto PythonInterpreter::exec_file(CStrView file_name) const -> bool {
  const auto file = open_file(file_name, "r");
  auto* const result = PyRun_File(/*fp=*/file.get(),
                                  /*filename=*/file_name.c_str(),
                                  /*start=*/Py_file_input,
                                  /*globals=*/globals_,
                                  /*locals=*/globals_);
  if (result != nullptr) {
    Py_DECREF(result);
    return true;
  }

  PyErr_Print();
  return false;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// NOLINTEND(*-include-cleaner)

} // namespace tit::back
