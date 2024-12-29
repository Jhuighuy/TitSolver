/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <memory>

#include "tit/core/main_func.hpp"
#include "tit/core/str_utils.hpp"
#include "tit/core/utils.hpp"

struct PyConfig;
using PyObject = struct _object; // NOLINT(*-reserved-identifier, cert-*)

namespace tit::python {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Interpreter configuration.
class Config final : public NonCopyableBase {
public:

  /// Construct the configuration.
  Config();

  /// Get the underlying configuration object.
  auto base() const noexcept -> PyConfig*;

  /// Set the Python home directory.
  void set_home(CStrView home) const;

  /// Set the program name.
  void set_prog_name(CStrView name) const;

  /// Parse according to Python conventions and set the command line arguments.
  void set_cmd_args(CmdArgs args) const;

private:

  struct Cleaner_ final : public std::default_delete<PyConfig> {
    void operator()(PyConfig* config) const;
  };

  std::unique_ptr<PyConfig, Cleaner_> config_;

}; // class Config

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Embedded Python interpreter.
class Interpreter final : public NonCopyableBase {
public:

  /// Construct the interpreter.
  explicit Interpreter(Config config);

  /// Destruct the interpreter.
  ~Interpreter();

  /// Append a search path to the Python path.
  void append_path(CStrView path) const;

  /// Execute the Python statement.
  auto exec(CStrView statement) const -> bool;

  /// Execute the Python file.
  auto exec_file(CStrView file_name) const -> bool;

private:

  // Start the coverage report.
  void start_coverage_report_() const;

  // Stop the coverage report.
  void stop_coverage_report_() const;

  static bool initialized_;
  Config config_;
  PyObject* globals_ = nullptr;

}; // class Interpreter

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::python
