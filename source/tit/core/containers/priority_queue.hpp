/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <functional>
#include <queue>
#include <utility>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/rand_utils.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Priority queue over key-value pairs.
///
/// Queue elements are compared by values, while keys are compared using a
/// randomized hash function.
template<std::totally_ordered Val>
class KeyedPriorityQueue final {
public:

  /// Construct a priority queue with the given number of keys.
  constexpr explicit KeyedPriorityQueue(size_t num_keys) : hashes_(num_keys) {}

  /// Get the number of keys.
  constexpr auto num_keys() const noexcept -> size_t {
    return hashes_.size();
  }

  /// Check if the queue is empty.
  constexpr auto empty() -> bool {
    remove_invalid_entries_();
    return queue_.empty();
  }

  /// Get the largest value and it's key.
  constexpr auto top() noexcept -> std::pair<size_t, const Val&> {
    remove_invalid_entries_();
    TIT_ASSERT(!queue_.empty(), "Queue is empty!");
    const auto& [key, val] = queue_.top();
    return {key, val};
  }

  /// Get the largest value it's key and remove it from the queue.
  constexpr auto pop() -> std::pair<size_t, Val> {
    remove_invalid_entries_();
    TIT_ASSERT(!queue_.empty(), "Queue is empty!");
    auto [key, val] = queue_.top();
    queue_.pop();
    return {key, std::move(val)};
  }

  /// Emplace or update a value at the given key.
  template<class... Args>
    requires std::constructible_from<Val, Args&&...>
  constexpr void emplace(size_t key, Args&&... args) {
    TIT_ASSERT(key < num_keys(), "Key is out of range!");
    Val val{std::forward<Args>(args)...};
    hashes_[key] = std::hash<Val>{}(val);
    queue_.emplace(key, std::move(val));
  }

  /// Erase the value with the given key from the queue.
  constexpr void erase(size_t key) noexcept {
    TIT_ASSERT(key < num_keys(), "Key is out of range!");
    hashes_[key] = npos;
  }

private:

  struct Elem_ {
    size_t key;
    Val val;
    friend constexpr auto operator<=>(const Elem_& a, const Elem_& b) noexcept {
      if (const auto cmp = a.val <=> b.val; cmp != 0) return cmp;
      return randomized_hash(a.key) <=> randomized_hash(b.key);
    }
  };

  // Remove the top queue entries that are no longer valid.
  void remove_invalid_entries_() noexcept {
    while (!queue_.empty()) {
      const auto& [key, val] = queue_.top();
      if (hashes_[key] == std::hash<Val>{}(val)) break;
      queue_.pop();
    }
  }

  std::priority_queue<Elem_> queue_;
  std::vector<size_t> hashes_;

}; // class PriorityQueue

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
