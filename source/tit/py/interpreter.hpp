/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <memory>

#include "tit/core/main_func.hpp"
#include "tit/core/str_utils.hpp"
#include "tit/core/utils.hpp"

#include "tit/py/mapping.hpp"
#include "tit/py/object.hpp"

struct PyConfig; // Not available under limited API.

namespace tit::py::embed {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Interpreter configuration.
class Config final {
public:

  /// Construct the configuration.
  Config();

  /// Get the underlying configuration object.
  auto get() const noexcept -> PyConfig*;

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

/// Basic embedded Python interpreter.
class BasicInterpreter {
public:

  TIT_NOT_COPYABLE_OR_MOVABLE(BasicInterpreter);

  /// Construct the interpreter.
  explicit BasicInterpreter(Config config = {});

  /// Destroy the interpreter.
  ~BasicInterpreter();

private:

  Config config_;

}; // class BasicInterpreter

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Embedded Python interpreter.
class Interpreter final : public BasicInterpreter {
public:

  TIT_NOT_COPYABLE_OR_MOVABLE(Interpreter);

  /// Construct the interpreter.
  explicit Interpreter(Config config = {});

  /// Destruct the interpreter.
  ~Interpreter();

  /// Append a search path to the Python path.
  void append_path(CStrView path) const;

  /// Get the global variables.
  auto globals() const -> const Dict&;

  /// Evaluate the Python expression.
  /// If evaluation fails, an exception is thrown.
  auto eval(CStrView expr) const -> Object;

  /// Execute the Python statement.
  /// If execution fails, an error is printed and `false` is returned.
  auto exec(CStrView stmt) const -> bool;

  /// Execute the Python file.
  /// If execution fails, an error is printed and `false` is returned.
  auto exec_file(CStrView file_name) const -> bool;

private:

  // Start the coverage report.
  void start_coverage_report_() const;

  // Stop the coverage report.
  void stop_coverage_report_() const;

  Dict globals_;

}; // class Interpreter

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::py::embed
