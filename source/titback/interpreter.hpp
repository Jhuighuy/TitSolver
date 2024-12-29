/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <memory>

#include "tit/core/main_func.hpp"
#include "tit/core/str_utils.hpp"
#include "tit/core/utils.hpp"

struct _object; // NOLINT
using PyObject = _object;
struct PyConfig;

namespace tit::back {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Interpreter configuration.
class PythonConfig final : public NonCopyableBase {
public:

  /// Construct the configuration.
  PythonConfig();

  /// Get the underlying configuration object.
  auto base() const noexcept -> PyConfig*;

  /// Set the Python home directory.
  void set_home(CStrView home) const;

  /// Set the program name.
  void set_prog_name(CStrView name) const;

  /// Set the command line arguments.
  void set_cmd_args(CmdArgs args) const;

private:

  struct Cleaner_ final : public std::default_delete<PyConfig> {
    void operator()(PyConfig* config) const;
  };

  std::unique_ptr<PyConfig, Cleaner_> config_;

}; // class PythonConfig

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Embedded Python interpreter.
class PythonInterpreter final : public NonCopyableBase {
public:

  /// Construct the interpreter.
  explicit PythonInterpreter(PythonConfig config);

  /// Destruct the interpreter.
  ~PythonInterpreter();

  /// Append a search path to the Python path.
  void append_path(CStrView path) const;

  /// Execute the Python statement.
  auto exec(CStrView statement) const -> bool;

  /// Execute the Python file.
  auto exec_file(CStrView file_name) const -> bool;

private:

  PythonConfig config_;
  PyObject* globals_ = nullptr;

}; // class PythonInterpreter

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::back
