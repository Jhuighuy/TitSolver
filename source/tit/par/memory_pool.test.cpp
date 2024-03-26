/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <array>
#include <thread>

#include "tit/core/basic_types.hpp"

#include "tit/par/memory_pool.hpp"

#include "tit/testing//test.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("tit::MemoryPool") {
  // Pool allocators are mostly used for building some tree-like structures
  // in parallel. This test is an attempt to recreate this use case: generate
  // a linked list in parallel storing the N consecutive numbers.
  constexpr auto Magic = 15241094284759029579UZ;
  struct ListNode {
    size_t magic = Magic;
    size_t value = 0;
    ListNode* next = nullptr;
  };
  // Creates threads that generate a linked list storing a portion of
  // consecutive numbers. The actual nodes will be stored inside of a pool
  // allocator.
  constexpr size_t NumThreads = 4;
  constexpr size_t NumNodes = 1024;
  par::MemoryPool<ListNode> pool{};
  std::array<ListNode*, NumThreads> lists{};
  std::array<std::thread, NumThreads> threads{};
  for (size_t thread_index = 0; thread_index < NumThreads; ++thread_index) {
    threads[thread_index] = std::thread([&, thread_index] {
      // Generate nodes that store our portion of the consecutive numbers.
      ListNode* list = nullptr;
      for (size_t node_index = 0; node_index < NumNodes; ++node_index) {
        /// Create the node.
        auto* node = pool.create();
        /// Assign the next consecutive number.
        node->value = thread_index * NumNodes + node_index;
        /// Append the current node into the linked list.
        node->next = list;
        list = node;
      }
      // Append our sublist into the global list.
      lists[thread_index] = list;
    });
  }
  for (auto& thread : threads) thread.join();
  // Check that amount of nodes and sum of the list elements matches the
  // expected results. Expected sum is just a sum of the arithmetic
  // progression. Also check that the magic number is preserved.
  constexpr auto ExpectedAmount = NumThreads * NumNodes;
  constexpr auto ExpectedSum = ExpectedAmount * (ExpectedAmount - 1) / 2;
  size_t actual_amount = 0, actual_sum = 0;
  for (auto* list : lists) {
    REQUIRE(list != nullptr);
    for (auto* node = list; node != nullptr; node = node->next) {
      CHECK(node->magic == Magic);
      actual_amount += 1, actual_sum += node->value;
    }
  }
  CHECK(actual_amount == ExpectedAmount);
  CHECK(actual_sum == ExpectedSum);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
