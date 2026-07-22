/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <array>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <span>
#include <utility>
#include <vector>

#include <mpi.h>

#include "tit/core/assert.hpp"
#include "tit/core/exception.hpp"
#include "tit/dist/communicator.hpp"
#include "tit/dist/environment.hpp"
#include "tit/dist/mpi.hpp"

// OpenMPI's predefined handle macros intentionally cast through void pointers.
// NOLINTBEGIN(bugprone-casting-through-void)

namespace tit::dist {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void check_mpi(int status, const char* operation) {
  if (status == MPI_SUCCESS) return;
  std::array<char, MPI_MAX_ERROR_STRING> message{};
  int size = 0;
  MPI_Error_string(status, message.data(), &size);
  TIT_THROW("{} failed: {}", operation, message.data());
}

template<class Val>
auto all_reduce(MPI_Comm communicator,
                Val value,
                MPI_Datatype datatype,
                MPI_Op operation) -> Val {
  Val result{};
  check_mpi(
      MPI_Allreduce(&value, &result, 1, datatype, operation, communicator),
      "MPI_Allreduce");
  return result;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace

class Communicator::State_ final {
public:

  explicit State_(MPI_Comm communicator) noexcept
      : communicator_{communicator} {}

  auto get() const noexcept -> MPI_Comm {
    return communicator_;
  }

private:

  MPI_Comm communicator_;

}; // class Communicator::State_

auto MPICommunicatorAccess::get(const Communicator& communicator) -> MPI_Comm {
  TIT_ASSERT(communicator.state_ != nullptr, "Communicator is null.");
  return communicator.state_->get();
}

Communicator::Communicator(std::shared_ptr<State_> state) noexcept
    : state_{std::move(state)} {}

auto Communicator::world() -> Communicator {
  TIT_ENSURE(Environment::active(),
             "Cannot access MPI communicator outside an active environment.");
  return Communicator{std::make_shared<State_>(MPI_COMM_WORLD)};
}

auto Communicator::rank() const -> std::size_t {
  TIT_ASSERT(state_ != nullptr, "Communicator is null.");
  int result = 0;
  check_mpi(MPI_Comm_rank(state_->get(), &result), "MPI_Comm_rank");
  TIT_ENSURE(result >= 0, "MPI returned a negative rank.");
  return static_cast<std::size_t>(result);
}

auto Communicator::size() const -> std::size_t {
  TIT_ASSERT(state_ != nullptr, "Communicator is null.");
  int result = 0;
  check_mpi(MPI_Comm_size(state_->get(), &result), "MPI_Comm_size");
  TIT_ENSURE(result > 0, "MPI returned an invalid communicator size.");
  return static_cast<std::size_t>(result);
}

void Communicator::barrier() const {
  TIT_ASSERT(state_ != nullptr, "Communicator is null.");
  check_mpi(MPI_Barrier(state_->get()), "MPI_Barrier");
}

[[noreturn]] void Communicator::abort(int error_code) const noexcept {
  if (state_ != nullptr) {
    static_cast<void>(MPI_Abort(state_->get(), error_code));
  }
  std::abort();
}

auto Communicator::all_reduce_min(float value) const -> float {
  TIT_ASSERT(state_ != nullptr, "Communicator is null.");
  return all_reduce(state_->get(), value, MPI_FLOAT, MPI_MIN);
}

auto Communicator::all_reduce_min(double value) const -> double {
  TIT_ASSERT(state_ != nullptr, "Communicator is null.");
  return all_reduce(state_->get(), value, MPI_DOUBLE, MPI_MIN);
}

auto Communicator::all_reduce_min(std::uint64_t value) const -> std::uint64_t {
  TIT_ASSERT(state_ != nullptr, "Communicator is null.");
  return all_reduce(state_->get(), value, MPI_UINT64_T, MPI_MIN);
}

auto Communicator::all_reduce_max(float value) const -> float {
  TIT_ASSERT(state_ != nullptr, "Communicator is null.");
  return all_reduce(state_->get(), value, MPI_FLOAT, MPI_MAX);
}

auto Communicator::all_reduce_max(double value) const -> double {
  TIT_ASSERT(state_ != nullptr, "Communicator is null.");
  return all_reduce(state_->get(), value, MPI_DOUBLE, MPI_MAX);
}

auto Communicator::all_reduce_max(std::uint64_t value) const -> std::uint64_t {
  TIT_ASSERT(state_ != nullptr, "Communicator is null.");
  return all_reduce(state_->get(), value, MPI_UINT64_T, MPI_MAX);
}

auto Communicator::all_reduce_sum(std::uint64_t value) const -> std::uint64_t {
  TIT_ASSERT(state_ != nullptr, "Communicator is null.");
  return all_reduce(state_->get(), value, MPI_UINT64_T, MPI_SUM);
}

auto Communicator::exclusive_scan_sum(std::uint64_t value) const
    -> std::uint64_t {
  TIT_ASSERT(state_ != nullptr, "Communicator is null.");
  std::uint64_t result = 0;
  check_mpi(
      MPI_Exscan(&value, &result, 1, MPI_UINT64_T, MPI_SUM, state_->get()),
      "MPI_Exscan");
  if (rank() == 0) result = 0;
  return result;
}

auto Communicator::all_to_all_bytes(
    std::span<const std::vector<std::byte>> send_buffers) const
    -> std::vector<std::vector<std::byte>> {
  TIT_ASSERT(state_ != nullptr, "Communicator is null.");
  const auto num_ranks = size();
  TIT_ENSURE(send_buffers.size() == num_ranks,
             "All-to-all exchange requires one send buffer per rank.");

  std::vector<int> send_counts(num_ranks);
  std::vector<int> send_offsets(num_ranks);
  int send_size = 0;
  for (std::size_t rank = 0; rank < num_ranks; ++rank) {
    TIT_ENSURE(send_buffers[rank].size() <= INT_MAX,
               "All-to-all send buffer is too large.");
    send_counts[rank] = static_cast<int>(send_buffers[rank].size());
    send_offsets[rank] = send_size;
    TIT_ENSURE(send_counts[rank] <= INT_MAX - send_size,
               "All-to-all send payload is too large.");
    send_size += send_counts[rank];
  }

  std::vector<int> receive_counts(num_ranks);
  check_mpi(MPI_Alltoall(send_counts.data(),
                         1,
                         MPI_INT,
                         receive_counts.data(),
                         1,
                         MPI_INT,
                         state_->get()),
            "MPI_Alltoall");

  std::vector<int> receive_offsets(num_ranks);
  int receive_size = 0;
  for (std::size_t rank = 0; rank < num_ranks; ++rank) {
    TIT_ENSURE(receive_counts[rank] >= 0 &&
                   receive_counts[rank] <= INT_MAX - receive_size,
               "All-to-all receive payload is too large.");
    receive_offsets[rank] = receive_size;
    receive_size += receive_counts[rank];
  }

  std::vector<std::byte> send_data(static_cast<std::size_t>(send_size));
  for (std::size_t rank = 0; rank < num_ranks; ++rank) {
    if (send_buffers[rank].empty()) continue;
    std::memcpy(send_data.data() + send_offsets[rank],
                send_buffers[rank].data(),
                send_buffers[rank].size());
  }
  std::vector<std::byte> receive_data(static_cast<std::size_t>(receive_size));
  check_mpi(MPI_Alltoallv(send_data.data(),
                          send_counts.data(),
                          send_offsets.data(),
                          MPI_BYTE,
                          receive_data.data(),
                          receive_counts.data(),
                          receive_offsets.data(),
                          MPI_BYTE,
                          state_->get()),
            "MPI_Alltoallv");

  std::vector<std::vector<std::byte>> receive_buffers(num_ranks);
  for (std::size_t rank = 0; rank < num_ranks; ++rank) {
    const auto count = static_cast<std::size_t>(receive_counts[rank]);
    const auto offset = static_cast<std::size_t>(receive_offsets[rank]);
    const auto* const begin = receive_data.data() + offset;
    receive_buffers[rank].assign(begin, begin + count);
  }
  return receive_buffers;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::dist

// NOLINTEND(bugprone-casting-through-void)
