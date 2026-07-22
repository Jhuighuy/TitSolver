/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>

#include <mpi.h>

#include "tit/core/assert.hpp"
#include "tit/core/exception.hpp"
#include "tit/dist/communicator.hpp"
#include "tit/dist/environment.hpp"

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

auto Communicator::all_reduce_min(float value) const -> float {
  TIT_ASSERT(state_ != nullptr, "Communicator is null.");
  return all_reduce(state_->get(), value, MPI_FLOAT, MPI_MIN);
}

auto Communicator::all_reduce_min(double value) const -> double {
  TIT_ASSERT(state_ != nullptr, "Communicator is null.");
  return all_reduce(state_->get(), value, MPI_DOUBLE, MPI_MIN);
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

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::dist

// NOLINTEND(bugprone-casting-through-void)
