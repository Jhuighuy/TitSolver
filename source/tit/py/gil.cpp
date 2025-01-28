/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "tit/core/exception.hpp"

#include "tit/py/_python.hpp"
#include "tit/py/gil.hpp"

namespace tit::py {

// NOLINTBEGIN(*-include-cleaner)

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

ReleaseGIL::ReleaseGIL() : state_{PyEval_SaveThread()} {
  if (state_ == nullptr) TIT_THROW("Failed to release the Python GIL.");
}

ReleaseGIL::~ReleaseGIL() noexcept {
  if (state_ != nullptr) PyEval_RestoreThread(state_);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

AcquireGIL::AcquireGIL() : state_{PyGILState_Ensure()} {
  if (state_ < 0) TIT_THROW("Failed to acquire the Python GIL.");
}

AcquireGIL::~AcquireGIL() noexcept {
  if (state_ >= 0) PyGILState_Release(static_cast<PyGILState_STATE>(state_));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// NOLINTEND(*-include-cleaner)

} // namespace tit::py
