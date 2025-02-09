/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <utility>

#include "tit/core/basic_types.hpp"
#include "tit/core/utils.hpp"

using PyThreadState = struct _ts; // NOLINT(*-reserved-identifier, cert-*)

namespace tit::py {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Release the Python GIL for the current scope.
class ReleaseGIL final {
public:

  TIT_MOVE_ONLY(ReleaseGIL);

  /// Release the Python GIL.
  ReleaseGIL();

  /// Reacquire the Python GIL.
  ~ReleaseGIL() noexcept;

  /// Move-construct a `ReleaseGIL`.
  ReleaseGIL(ReleaseGIL&& other) noexcept
      : state_{std::exchange(other.state_, nullptr)} {}

  /// Move-assign a `ReleaseGIL`.
  auto operator=(ReleaseGIL&& other) noexcept -> ReleaseGIL& {
    if (this != &other) state_ = std::exchange(other.state_, nullptr);
    return *this;
  }

private:

  PyThreadState* state_ = nullptr;

}; // class ReleaseGIL

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Reacquire the Python GIL for the current scope.
class AcquireGIL final {
public:

  TIT_MOVE_ONLY(AcquireGIL);

  /// Reacquire the Python GIL.
  AcquireGIL();

  /// Release the Python GIL.
  ~AcquireGIL() noexcept;

  /// Move-construct an `AcquireGIL`.
  AcquireGIL(AcquireGIL&& other) noexcept
      : state_{std::exchange(other.state_, -1)} {}

  /// Move-assign an `AcquireGIL`.
  auto operator=(AcquireGIL&& other) noexcept -> AcquireGIL& {
    if (this != &other) state_ = std::exchange(other.state_, -1);
    return *this;
  }

private:

  int64_t state_ = -1;

}; // class AcquireGIL

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::py
