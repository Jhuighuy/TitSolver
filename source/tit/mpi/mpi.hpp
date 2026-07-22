/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include <mpi.h>

#include "tit/core/assert.hpp"
#include "tit/core/exception.hpp"

namespace tit::mpi {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace impl {

// Report a failed MPI call and throw an exception.
[[noreturn]] void raise_status(int status, std::string_view expression);

} // namespace impl

/// Invoke an MPI function and throw an exception on failure.
#define TIT_MPI_CALL(...)                                                      \
  do {                                                                         \
    if (const int tit_mpi_status_ = (__VA_ARGS__);                             \
        tit_mpi_status_ != MPI_SUCCESS) {                                      \
      tit::mpi::impl::raise_status(tit_mpi_status_, #__VA_ARGS__);             \
    }                                                                          \
  } while (false)

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Runtime control.
//

/// Check if the MPI runtime is initialized.
auto initialized() -> bool;

/// Initialize the MPI runtime.
///
/// The runtime is initialized in the "funneled" threading mode: all MPI calls
/// must be made from the main thread, outside of the parallel regions.
void init(int& argc, char**& argv);

/// Finalize the MPI runtime.
void finalize();

/// Scoped MPI runtime.
class Runtime final {
public:

  /// Runtime is not copyable or movable.
  /// @{
  Runtime(const Runtime&) = delete;
  Runtime(Runtime&&) = delete;
  auto operator=(const Runtime&) -> Runtime& = delete;
  auto operator=(Runtime&&) -> Runtime& = delete;
  /// @}

  /// Initialize the MPI runtime.
  Runtime(int& argc, char**& argv) {
    init(argc, argv);
  }

  /// Finalize the MPI runtime.
  ~Runtime() noexcept {
    terminate_on_exception([] { finalize(); });
  }

}; // class Runtime

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Datatypes and operations.
//

/// Type that maps onto a predefined MPI datatype, and therefore can be used
/// in reduction operations.
template<class T>
concept reducible = std::same_as<T, float> || std::same_as<T, double> ||
                    (std::integral<T> && !std::same_as<T, bool> &&
                     (sizeof(T) == 4 || sizeof(T) == 8));

/// MPI datatype for the specified type.
template<reducible T>
auto datatype_of() noexcept -> MPI_Datatype {
  if constexpr (std::same_as<T, float>) {
    return MPI_FLOAT;
  } else if constexpr (std::same_as<T, double>) {
    return MPI_DOUBLE;
  } else if constexpr (sizeof(T) == 4) {
    return std::is_signed_v<T> ? MPI_INT32_T : MPI_UINT32_T;
  } else {
    return std::is_signed_v<T> ? MPI_INT64_T : MPI_UINT64_T;
  }
}

/// Reduction operation.
enum class Op : std::uint8_t {
  min, ///< Minimum value.
  max, ///< Maximum value.
  sum, ///< Sum of the values.
};

/// MPI operation for the specified reduction operation.
auto op_of(Op op) noexcept -> MPI_Op;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Communicator.
//

/// Rank of the main process.
inline constexpr std::size_t main_rank = 0;

/// Communicator.
///
/// This is a non-owning wrapper over an MPI communicator handle. The wrapped
/// communicator must outlive the wrapper.
class Comm final {
public:

  /// Wrap a communicator handle. World communicator is wrapped by default.
  constexpr explicit Comm(MPI_Comm comm = MPI_COMM_WORLD) noexcept
      : comm_{comm} {}

  /// Get the underlying communicator handle.
  constexpr auto base() const noexcept -> MPI_Comm {
    return comm_;
  }

  /// Rank of the current process.
  auto rank() const -> std::size_t {
    int rank = 0;
    TIT_MPI_CALL(MPI_Comm_rank(comm_, &rank));
    return static_cast<std::size_t>(rank);
  }

  /// Number of processes.
  auto size() const -> std::size_t {
    int size = 0;
    TIT_MPI_CALL(MPI_Comm_size(comm_, &size));
    return static_cast<std::size_t>(size);
  }

  /// Check if the current process is the main process.
  auto is_main() const -> bool {
    return rank() == main_rank;
  }

  /// Wait for all the processes to reach this point.
  void barrier() const {
    TIT_MPI_CALL(MPI_Barrier(comm_));
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  //
  // Collective operations.
  //

  /// Reduce a value over all the processes.
  template<reducible T>
  auto all_reduce(T value, Op op) const -> T {
    T result{};
    TIT_MPI_CALL(MPI_Allreduce(&value,
                               &result,
                               /*count=*/1,
                               datatype_of<T>(),
                               op_of(op),
                               comm_));
    return result;
  }

  /// Reduce values over all the processes in-place, element-wise.
  template<reducible T>
  void all_reduce(std::span<T> values, Op op) const {
    TIT_MPI_CALL(MPI_Allreduce(MPI_IN_PLACE,
                               values.data(),
                               to_int_(values.size()),
                               datatype_of<T>(),
                               op_of(op),
                               comm_));
  }

  /// Gather a value from all the processes on all the processes.
  template<class T>
    requires std::is_trivially_copyable_v<T>
  auto all_gather(const T& value) const -> std::vector<T> {
    std::vector<T> result(size());
    TIT_MPI_CALL(MPI_Allgather(&value,
                               /*sendcount=*/sizeof(T),
                               MPI_BYTE,
                               result.data(),
                               /*recvcount=*/sizeof(T),
                               MPI_BYTE,
                               comm_));
    return result;
  }

  /// Exchange a single count with every process.
  ///
  /// @param send_counts Per-process values to send, size of the communicator.
  ///
  /// @returns Per-process values received, size of the communicator.
  auto all_to_all(std::span<const std::size_t> send_counts) const
      -> std::vector<std::size_t> {
    TIT_ASSERT(send_counts.size() == size(),
               "Send counts must match the communicator size.");
    std::vector<std::size_t> recv_counts(send_counts.size());
    TIT_MPI_CALL(MPI_Alltoall(send_counts.data(),
                              /*sendcount=*/1,
                              datatype_of<std::size_t>(),
                              recv_counts.data(),
                              /*recvcount=*/1,
                              datatype_of<std::size_t>(),
                              comm_));
    return recv_counts;
  }

  /// Exchange byte buffers with every process.
  ///
  /// Buffers are laid out contiguously per destination process, sizes are
  /// given by the counts arguments.
  void all_to_all_v(std::span<const std::byte> send_bytes,
                    std::span<const std::size_t> send_counts,
                    std::span<std::byte> recv_bytes,
                    std::span<const std::size_t> recv_counts) const {
    const auto make_counts_and_displs =
        [this](std::span<const std::size_t> counts) {
          TIT_ASSERT(counts.size() == size(),
                     "Counts must match the communicator size.");
          std::pair<std::vector<int>, std::vector<int>> result{};
          auto& [ints, displs] = result;
          ints.reserve(counts.size());
          displs.reserve(counts.size());
          std::size_t offset = 0;
          for (const auto count : counts) {
            ints.push_back(to_int_(count));
            displs.push_back(to_int_(offset));
            offset += count;
          }
          return result;
        };
    const auto [send_ints, send_displs] = make_counts_and_displs(send_counts);
    const auto [recv_ints, recv_displs] = make_counts_and_displs(recv_counts);
    TIT_MPI_CALL(MPI_Alltoallv(send_bytes.data(),
                               send_ints.data(),
                               send_displs.data(),
                               MPI_BYTE,
                               recv_bytes.data(),
                               recv_ints.data(),
                               recv_displs.data(),
                               MPI_BYTE,
                               comm_));
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  //
  // Point-to-point operations.
  //

  /// Send a byte buffer to the specified process.
  void send(std::span<const std::byte> bytes,
            std::size_t dest,
            int tag = 0) const {
    TIT_MPI_CALL(MPI_Send(bytes.data(),
                          to_int_(bytes.size()),
                          MPI_BYTE,
                          to_int_(dest),
                          tag,
                          comm_));
  }

  /// Receive a byte buffer from the specified process.
  void recv(std::span<std::byte> bytes, std::size_t source, int tag = 0) const {
    TIT_MPI_CALL(MPI_Recv(bytes.data(),
                          to_int_(bytes.size()),
                          MPI_BYTE,
                          to_int_(source),
                          tag,
                          comm_,
                          MPI_STATUS_IGNORE));
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  static constexpr auto to_int_(std::size_t value) -> int {
    TIT_ASSERT(value <=
                   static_cast<std::size_t>(std::numeric_limits<int>::max()),
               "Value exceeds the MPI count limit.");
    return static_cast<int>(value);
  }

  MPI_Comm comm_;

}; // class Comm

/// World communicator.
inline const Comm world{};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::mpi
