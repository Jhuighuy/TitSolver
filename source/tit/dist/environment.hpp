/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

namespace tit::dist {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// MPI process environment using funneled thread support.
class Environment final {
public:

  /// Initialize MPI without forwarding application arguments.
  Environment();

  /// Initialize MPI and allow it to consume application arguments.
  Environment(int& argc, char**& argv);

  Environment(const Environment&) = delete;
  Environment(Environment&&) = delete;
  auto operator=(const Environment&) -> Environment& = delete;
  auto operator=(Environment&&) -> Environment& = delete;

  /// Finalize MPI if this object initialized it.
  ~Environment();

  /// Check whether MPI is initialized and not finalized.
  static auto active() noexcept -> bool;

private:

  void initialize_(int* argc, char*** argv);

  bool owns_environment_ = false;

}; // class Environment

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::dist
