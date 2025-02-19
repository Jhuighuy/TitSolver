/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <utility>

#include "tit/core/containers/priority_queue.hpp"

#include "tit/testing/test.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("KeyedPriorityQueue") {
  KeyedPriorityQueue<int> queue{3};
  CHECK(queue.num_keys() == 3);
  CHECK(queue.empty());
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("KeyedPriorityQueue::empty") {
  // Ensure the queue is empty initially.
  KeyedPriorityQueue<int> queue{3};
  CHECK(queue.empty());

  // Add some elements.
  queue.emplace(0, 1);
  CHECK_FALSE(queue.empty());

  // Add more elements. Queue should not be empty.
  queue.emplace(1, 2);
  queue.emplace(2, 3);
  CHECK_FALSE(queue.empty());

  // Remove some elements. Queue still should not be empty.
  queue.erase(1);
  CHECK_FALSE(queue.empty());

  // Remove all the remaining elements. Queue should be empty.
  queue.erase(0);
  queue.erase(2);
  CHECK(queue.empty());
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("KeyedPriorityQueue::top_emplace_and_erase") {
  // Initialize the queue.
  KeyedPriorityQueue<int> queue{3};
  queue.emplace(0, 1);
  queue.emplace(1, 3);
  queue.emplace(2, 2);
  REQUIRE_FALSE(queue.empty());

  // Check the top element.
  CHECK(queue.top() == std::pair{1, 3});

  // Update the top element.
  queue.emplace(1, 4);
  CHECK(queue.top() == std::pair{1, 4});

  // Make the different element the top.
  queue.emplace(2, 5);
  CHECK(queue.top() == std::pair{2, 5});

  // Update the non-top element.
  queue.emplace(0, 0);
  CHECK(queue.top() == std::pair{2, 5});

  // Remove the top element.
  queue.erase(2);
  CHECK(queue.top() == std::pair{1, 4});
}

TEST_CASE("KeyedPriorityQueue::pop_emplace_and_erase") {
  // Initialize the queue.
  KeyedPriorityQueue<int> queue{3};
  queue.emplace(0, 1);
  queue.emplace(1, 3);
  queue.emplace(2, 2);
  REQUIRE_FALSE(queue.empty());

  // Pop the top element.
  CHECK(queue.pop() == std::pair{1, 3});
  CHECK(queue.top() == std::pair{2, 2});

  // Pop the top element again.
  CHECK(queue.pop() == std::pair{2, 2});
  CHECK(queue.top() == std::pair{0, 1});

  // Add some elements.
  queue.emplace(0, 5);
  queue.emplace(1, 3);
  queue.emplace(2, 4);

  // Pop the top element.
  CHECK(queue.pop() == std::pair{0, 5});
  CHECK(queue.top() == std::pair{2, 4});

  // Make the different element the top.
  queue.emplace(2, 6);
  CHECK(queue.pop() == std::pair{2, 6});
  CHECK(queue.top() == std::pair{1, 3});
}

TEST_CASE("KeyedPriorityQueue::top_key_hashing") {
  // Initialize the queue.
  KeyedPriorityQueue<int> queue{4};
  queue.emplace(0, 1);
  queue.emplace(1, 10);
  queue.emplace(2, 10);
  queue.emplace(3, 10);

  // Check the top element. It should not be the one with the smallest key
  // because the keys are randomly hashed.
  CHECK(queue.top().first != 1);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
