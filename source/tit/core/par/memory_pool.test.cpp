/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "tit/core/par/memory_pool.hpp"

#include "tit/testing/test.hpp"

namespace tit {
namespace {

// Disclaimer: Since this submodule is no more that a simple wrapper around the
// Intel TBB library, there is no need to test it in detail. The only thing we
// need to test is that our wrappers are working correctly.

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("par::MemoryPool") {
  struct Struct {
    int data_1;
    int data_2;
  };
  par::MemoryPool<Struct> pool{};
  auto* const root = pool.create(10, 20);
  CHECK(root->data_1 == 10);
  CHECK(root->data_2 == 20);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
