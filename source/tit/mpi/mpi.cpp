/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <array>
#include <cstddef>
#include <string_view>
#include <utility>

#include <mpi.h>

#include "tit/core/assert.hpp"
#include "tit/core/exception.hpp"
#include "tit/mpi/mpi.hpp"

namespace tit::mpi {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace impl {

[[noreturn]] void raise_status(int status, std::string_view expression) {
  std::array<char, MPI_MAX_ERROR_STRING> message{};
  int message_size = 0;
  if (MPI_Error_string(status, message.data(), &message_size) != MPI_SUCCESS) {
    message_size = 0;
  }
  TIT_THROW(
      "MPI call '{}' failed with status {}: {}.",
      expression,
      status,
      std::string_view(message.data(), static_cast<std::size_t>(message_size)));
}

} // namespace impl

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto initialized() -> bool {
  int flag = 0;
  TIT_MPI_CALL(MPI_Initialized(&flag));
  return flag != 0;
}

void init(int& argc, char**& argv) {
  TIT_ASSERT(!initialized(), "MPI runtime is already initialized.");

  // Initialize the runtime in the "funneled" threading mode: MPI calls are
  // made from the main thread only, outside of the TBB parallel regions.
  int provided = MPI_THREAD_SINGLE;
  TIT_MPI_CALL(MPI_Init_thread(&argc, &argv, MPI_THREAD_FUNNELED, &provided));
  TIT_ENSURE(provided >= MPI_THREAD_FUNNELED,
             "MPI runtime does not support the funneled threading mode.");

  // Convert MPI errors into return codes (and, therefore, exceptions)
  // instead of aborting the program.
  TIT_MPI_CALL(MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN));
}

void finalize() {
  if (!initialized()) return;
  TIT_MPI_CALL(MPI_Finalize());
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto op_of(Op op) noexcept -> MPI_Op {
  switch (op) {
    case Op::min: return MPI_MIN;
    case Op::max: return MPI_MAX;
    case Op::sum: return MPI_SUM;
  }
  std::unreachable();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::mpi
