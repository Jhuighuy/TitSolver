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
#ifdef TIT_HAVE_GCOV
#include "tit/core/log.hpp"
#endif

#include "tit/py/_python.hpp"
#include "tit/py/error.hpp"
#include "tit/py/interpreter.hpp"
#include "tit/py/module.hpp"
#include "tit/py/object.hpp"

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

namespace {

// Dedent the string.
auto dedent(CStrView str) -> std::string {
  if (!str.starts_with('\n')) return std::string{str};
  const auto textwrap = import_("textwrap");
  const auto result = textwrap.attr("dedent")(str);
  return extract<std::string>(result);
}

} // namespace

Interpreter::Interpreter(Config config)
    : BasicInterpreter{std::move(config)},
      globals_{import_("__main__").dict()} {
#ifdef TIT_HAVE_GCOV
  start_coverage_report_();
#endif
}

Interpreter::~Interpreter() {
#ifdef TIT_HAVE_GCOV
  try {
    stop_coverage_report_();
  } catch (const ErrorException& e) {
    TIT_ERROR("Failed to finalize Python coverage report: {}.", e.what());
  }
#endif
}

// NOLINTNEXTLINE(*-convert-member-functions-to-static)
void Interpreter::append_path(CStrView path) const {
  const auto sys = import_("sys");
  const auto sys_path = expect<List>(sys.attr("path"));
  sys_path.append(path);
}

auto Interpreter::globals() const -> const Dict& {
  return globals_;
}

auto Interpreter::eval(CStrView expr) const -> Object {
  return steal(ensure(PyRun_String(dedent(expr).c_str(),
                                   /*start=*/Py_eval_input,
                                   /*globals=*/globals_.get(),
                                   /*locals=*/globals_.get())));
}

auto Interpreter::exec(CStrView stmt) const -> bool {
  auto* const result = PyRun_String(dedent(stmt).c_str(),
                                    /*start=*/Py_file_input,
                                    /*globals=*/globals_.get(),
                                    /*locals=*/globals_.get());
  if (result == nullptr) {
    PyErr_Print();
    return false;
  }
  Py_DECREF(result);
  return true;
}

auto Interpreter::exec_file(CStrView file_name) const -> bool {
  const auto file = open_file(file_name, "r");
  globals()["__file__"] = file_name;
  auto* const result = PyRun_File(/*fp=*/file.get(),
                                  /*filename=*/file_name.c_str(),
                                  /*start=*/Py_file_input,
                                  /*globals=*/globals_.get(),
                                  /*locals=*/globals_.get());
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
  const auto coverage = import_("coverage");
  const auto Coverage = coverage.attr("Coverage");
  const auto coverage_report = Coverage(py::kwarg("branch", true),
                                        py::kwarg("config_file", config_file));
  coverage_report.attr("start")();
  globals()["__coverage_report"] = coverage_report;
}

void Interpreter::stop_coverage_report_() const {
  // Some of our tests emit warnings for missing coverage data, ignore them.
  const auto warnings = import_("warnings");
  warnings.attr("filterwarnings")("ignore");

  // Stop the coverage report and save it.
  const Object coverage_report = globals()["__coverage_report"];
  coverage_report.attr("stop")();
  coverage_report.attr("save")();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// NOLINTEND(*-include-cleaner)

} // namespace tit::py::embed
