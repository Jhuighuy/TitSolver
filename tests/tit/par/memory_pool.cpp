/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <algorithm>
#include <array>
#include <limits>
#include <thread>
#include <utility>

#include <doctest/doctest.h>

#include "tit/core/basic_types.hpp"
#include "tit/par/memory_pool.hpp"

namespace {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

TEST_CASE("tit::par::MemoryPool") {
  SUBCASE("basic") {
    // Create memory pool.
    tit::par::MemoryPool<int> pool{};
    // Check that basic allocations work.
    constexpr auto count = 1024uz;
    auto* data = pool.allocate(count);
    CHECK(data != nullptr);
    // Check that the memory is accessible.
    std::fill_n(data, count, 1234);
    // Deallocate the data.
    pool.deallocate(data);
  }

  /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

  SUBCASE("no construction") {
    // A class that triggers failure inside of it's constructors or destructor.
    struct NonConstructible {
      int payload;
      NonConstructible() {
        CHECK(!"Cannot construct!");
      }
      ~NonConstructible() {
        CHECK(!"Cannot destruct!");
      }
    };
    // Create memory pool.
    tit::par::MemoryPool<NonConstructible> pool{};
    // Allocate the data.
    auto* data = pool.allocate(1);
    // Deallocate the data.
    pool.deallocate(data);
  }

  /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

  SUBCASE("allocate too much") {
    // Create memory pool.
    tit::par::MemoryPool<int> pool{};
    // Try to allocate too much memory and expect `nullptr` to be returned.
    constexpr auto too_much = std::numeric_limits<tit::size_t>::max();
    CHECK(pool.allocate(too_much) == nullptr);
  }

  /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

  SUBCASE("move") {
    // Create memory pool.
    tit::par::MemoryPool<int> pool{};
    // Allocate a single value and assign the value to it.
    constexpr auto val = 1234;
    auto* data = pool.allocate(1);
    *data = val;
    // Move pool into a different variable and check if the data is
    // still accessible.
    auto pool2(std::move(pool));
    CHECK(*data == val);
    // Move pool back and check the data again.
    pool = std::move(pool2);
    CHECK(*data == val);
  }

  /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

  SUBCASE("parallel linked list tests") {
    // Pool allocators are mostly used for building some tree-like structures
    // in parallel. This test is an attempt to recreate this use case: generate
    // a linked list in parallel storing the N consecutive numbers.
    struct ListNode {
      tit::size_t magic;
      tit::size_t value;
      ListNode* next;
    };
    // Creates threads that generate a linked list storing a portion of
    // consecutive numbers. The actual nodes will be stored inside of a pool
    // allocator. Each node is also equipped with the magic number.
    constexpr auto magic = 15241094284759029579UZ;
    constexpr auto num_threads = 4UZ;
    constexpr auto num_nodes = 1024UZ;
    tit::par::MemoryPool<ListNode> pool{};
    std::array<ListNode*, num_threads> all_lists{};
    std::array<std::thread, num_threads> threads{};
    const auto main_thread_id = std::this_thread::get_id();
    for (auto thread_index = 0UZ; thread_index < num_threads; ++thread_index) {
      threads[thread_index] = std::thread([&, thread_index] {
        // Check that we are actually inside of the separate thread.
        const auto current_thread_id = std::this_thread::get_id();
        CHECK(main_thread_id != current_thread_id);
        // Generate nodes that store our portion of the consecutive numbers.
        ListNode* list = nullptr;
        for (auto node_index = 0UZ; node_index < num_nodes; ++node_index) {
          /// Allocate the node.
          auto* node = pool.allocate(1);
          /// Assign the magic number.
          node->magic = magic;
          /// Assign the next consecutive number.
          node->value = thread_index * num_nodes + node_index;
          /// Append the current node into the linked list.
          node->next = list;
          list = node;
        }
        // Append our sublist into the global list.
        all_lists[thread_index] = list;
      });
    }
    // Join the threads once they finish.
    for (auto& thread : threads) thread.join();
    // Check that amount of nodes and sum of the list elements matches the
    // expected results. Expected sum is just a sum of the arithmetic
    // progression. Also check that the magic number is preserved.
    constexpr auto expected_amount = num_threads * num_nodes;
    constexpr auto expected_sum = expected_amount * (expected_amount - 1) / 2;
    auto actual_amount = 0UZ;
    auto actual_sum = 0UZ;
    for (auto* list : all_lists) {
      CHECK(list != nullptr);
      for (auto* node = list; node != nullptr; node = node->next) {
        CHECK(node->magic == magic);
        actual_amount += 1;
        actual_sum += node->value;
      }
    }
    CHECK(actual_amount == expected_amount);
    CHECK(actual_sum == expected_sum);
  }
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace
