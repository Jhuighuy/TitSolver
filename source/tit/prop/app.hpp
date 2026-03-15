/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include "tit/core/main.hpp"
#include "tit/prop/spec.hpp"
#include "tit/prop/tree.hpp"

namespace tit::prop {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Abstract base class for command-line applications.
class App {
public:

  /// Construct an app.
  App() = default;

  /// Move-construct an app.
  App(App&&) = default;

  /// App is not copy-constructible.
  App(const App&) = delete;

  /// Move-assign an app.
  auto operator=(App&&) -> App& = default;

  /// App is not copy-assignable.
  auto operator=(const App&) -> App& = delete;

  /// Destruct the app.
  virtual ~App() = default;

  /// Return the application specification.
  virtual auto spec() const -> AppSpec = 0;

  /// Run the application with the parsed properties.
  virtual void run(Tree props) = 0;

}; // class App

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Execute a command-line application from `main(argc, argv)`.
void exec_app(App& app, int argc, char** argv);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::prop

/// Implementation of the entry point for the command-line application.
#define TIT_IMPLEMENT_APP(AppClass)                                            \
  TIT_IMPLEMENT_MAIN([](int app_argc_, char** app_argv_) {                     \
    AppClass app_;                                                             \
    tit::prop::exec_app(app_, app_argc_, app_argv_);                           \
  })

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
