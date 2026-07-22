/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace tit::dist {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// MPI communicator with an implementation-hidden native handle.
class Communicator final {
public:

  /// Get the world communicator.
  static auto world() -> Communicator;

  /// Rank in the communicator.
  auto rank() const -> std::size_t;

  /// Number of ranks in the communicator.
  auto size() const -> std::size_t;

  /// Wait until all ranks reach the barrier.
  void barrier() const;

  /// Compute a communicator-wide minimum.
  /// @{
  auto all_reduce_min(float value) const -> float;
  auto all_reduce_min(double value) const -> double;
  /// @}

  /// Compute a communicator-wide unsigned sum.
  auto all_reduce_sum(std::uint64_t value) const -> std::uint64_t;

  /// Compute an exclusive prefix sum, returning zero on rank zero.
  auto exclusive_scan_sum(std::uint64_t value) const -> std::uint64_t;

  /// Exchange a variable-size byte buffer with every rank.
  auto all_to_all_bytes(std::span<const std::vector<std::byte>> send_buffers)
      const -> std::vector<std::vector<std::byte>>;

private:

  class State_;

  explicit Communicator(std::shared_ptr<State_> state) noexcept;

  std::shared_ptr<State_> state_;

}; // class Communicator

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::dist
