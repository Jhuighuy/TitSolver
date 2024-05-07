/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <future>

#include "tit/par/memory_pool.hpp"

#include "tit/testing/test.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

struct Node {
  int data = -1;
  Node* left = nullptr;
  Node* right = nullptr;
};

auto make_tree_async(int i, int n, par::MemoryPool<Node>* pool) -> Node* {
  // Allocate the node.
  auto* const node = pool->create(10 * i);
  if (n == 1) {
    // Reached leaf node.
    node->left = node->right = nullptr;
  } else {
    // Spawn new tasks.
    auto left_future = std::async(&make_tree_async, i * 2, n / 2, pool);
    auto right_future = std::async(&make_tree_async, i * 2 + 1, n / 2, pool);
    left_future.wait();
    right_future.wait();
    node->left = left_future.get();
    node->right = right_future.get();
  }
  return node;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("par::MemoryPool") {
  // Generate a tree in parallel.
  par::MemoryPool<Node> pool{};
  auto* const root = make_tree_async(0, 4, &pool);
  // Ensure all the memory is allocated correctly.
  REQUIRE(root != nullptr);
  REQUIRE(root->left != nullptr);
  REQUIRE(root->left->left != nullptr);
  REQUIRE(root->left->right != nullptr);
  REQUIRE(root->right != nullptr);
  REQUIRE(root->right->left != nullptr);
  REQUIRE(root->right->right != nullptr);
  // Check that all the values are correct.
  CHECK(root->data == 0);
  CHECK(root->left->data == 0);
  CHECK(root->left->left->data == 0);
  CHECK(root->left->right->data == 10);
  CHECK(root->right->data == 10);
  CHECK(root->right->left->data == 20);
  CHECK(root->right->right->data == 30);
  // Free the memory (most likely does nothing).
  pool.destroy(root);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
