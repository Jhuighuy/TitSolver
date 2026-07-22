/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <array>
#include <cstdlib>

#include <mpi.h>

#include "tit/core/exception.hpp"
#include "tit/dist/environment.hpp"

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

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace

Environment::Environment() {
  initialize_(nullptr, nullptr);
}

Environment::Environment(int& argc, char**& argv) {
  initialize_(&argc, &argv);
}

void Environment::initialize_(int* argc, char*** argv) {
  int initialized = 0;
  check_mpi(MPI_Initialized(&initialized), "MPI_Initialized");
  if (initialized != 0) {
    int finalized = 0;
    check_mpi(MPI_Finalized(&finalized), "MPI_Finalized");
    TIT_ENSURE(finalized == 0, "MPI environment was already finalized.");
    int provided = MPI_THREAD_SINGLE;
    check_mpi(MPI_Query_thread(&provided), "MPI_Query_thread");
    TIT_ENSURE(provided >= MPI_THREAD_FUNNELED,
               "MPI environment does not provide funneled thread support.");
    return;
  }

  int provided = MPI_THREAD_SINGLE;
  check_mpi(MPI_Init_thread(argc, argv, MPI_THREAD_FUNNELED, &provided),
            "MPI_Init_thread");
  owns_environment_ = true;
  if (provided < MPI_THREAD_FUNNELED) {
    const auto status = MPI_Finalize();
    owns_environment_ = false;
    check_mpi(status, "MPI_Finalize");
    TIT_THROW("MPI implementation does not provide funneled thread support.");
  }
}

Environment::~Environment() {
  if (!owns_environment_) return;
  int finalized = 0;
  if (MPI_Finalized(&finalized) != MPI_SUCCESS) std::abort();
  if (finalized == 0 && MPI_Finalize() != MPI_SUCCESS) std::abort();
}

auto Environment::active() noexcept -> bool {
  int initialized = 0;
  int finalized = 0;
  if (MPI_Initialized(&initialized) != MPI_SUCCESS || initialized == 0) {
    return false;
  }
  if (MPI_Finalized(&finalized) != MPI_SUCCESS) return false;
  return finalized == 0;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::dist
