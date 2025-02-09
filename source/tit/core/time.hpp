/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <chrono>
#include <utility>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/utils.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Basic stopwatch.
class Stopwatch final {
public:

  /// Start the new stopwatch cycle.
  void start() noexcept {
    start_ = std::chrono::steady_clock::now();
  }

  /// Stop the stopwatch cycle and update the measured delta time.
  void stop() noexcept {
    const auto stop = std::chrono::steady_clock::now();
    TIT_ASSERT(stop > start_, "Stopwatch was not started!");
    const auto delta = stop - start_;
    total_ += std::chrono::duration_cast<std::chrono::nanoseconds>(delta);
    cycles_ += 1;
  }

  /// Get the total measured time (in nanoseconds).
  constexpr auto total_ns() const noexcept -> size_t {
    return total_.count();
  }

  /// Get the total measured time (in seconds).
  constexpr auto total() const noexcept -> real_t {
    return 1.0e-9 * static_cast<real_t>(total_ns());
  }

  /// Get the average cycle time (in nanoseconds).
  constexpr auto cycle_ns() const noexcept -> size_t {
    return cycles_ > 0 ? total_ns() / cycles_ : 0;
  }

  /// Get the average cycle time (in seconds).
  constexpr auto cycle() const noexcept -> real_t {
    return 1.0e-9 * static_cast<real_t>(cycle_ns());
  }

  /// Amount of cycles.
  constexpr auto cycles() const noexcept -> size_t {
    return cycles_;
  }

  /// Reset the stopwatch.
  void reset() noexcept {
    total_ = std::chrono::nanoseconds{};
    cycles_ = 0;
  }

private:

  std::chrono::time_point<std::chrono::steady_clock> start_;
  std::chrono::nanoseconds total_{};
  size_t cycles_{};

}; // class Stopwatch

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Scoped stopwatch cycle.
class StopwatchCycle final {
public:

  TIT_MOVE_ONLY(StopwatchCycle);

  /// Start the new stopwatch cycle.
  explicit StopwatchCycle(Stopwatch& stopwatch) noexcept
      : stopwatch_{&stopwatch} {
    stopwatch_->start();
  }

  /// Stop the stopwatch cycle and update the measured delta time.
  ~StopwatchCycle() noexcept {
    if (stopwatch_ != nullptr) stopwatch_->stop();
  }

  /// Move-construct the stopwatch cycle.
  constexpr StopwatchCycle(StopwatchCycle&& other) noexcept
      : stopwatch_{std::exchange(other.stopwatch_, nullptr)} {}

  /// Move-assign the stopwatch cycle.
  constexpr auto operator=(StopwatchCycle&& other) noexcept -> StopwatchCycle& {
    if (this != &other) stopwatch_ = std::exchange(other.stopwatch_, nullptr);
    return *this;
  }

private:

  Stopwatch* stopwatch_;

}; // class StopwatchCycle

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
