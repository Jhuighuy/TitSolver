/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <type_traits>

#include "tit/core/basic_types.hpp"
#include "tit/core/par/atomic.hpp"
#include "tit/core/type.hpp"

namespace tit::par {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Thread-safe arena.
template<class Val>
  requires std::is_default_constructible_v<Val> &&
           std::is_trivially_destructible_v<Val>
class Arena final {
public:

  TIT_MOVE_ONLY(Arena);

  /// Construct an empty arena.
  constexpr Arena() = default;

  /// Move-construct an arena.
  constexpr Arena(Arena&& other) noexcept
      : head_{std::exchange(other.head_, nullptr)} {}

  /// Move-assign an arena.
  constexpr auto operator=(Arena&& other) noexcept -> Arena& {
    head_ = std::exchange(other.head_, nullptr);
    return *this;
  }

  /// Destroy the arena.
  ~Arena() {
    for (Page* page = head_; page != nullptr;) {
      Page* next = page->next;
      operator delete[](page);
      page = next;
    }
  }

  /// Reset the arena.
  constexpr void reset() {
    for (Page* page = head_; page != nullptr; page = page->next) {
      page->size = 0;
    }
  }

  /// Allocate a range of values.
  constexpr auto allocate(size_t count) -> std::span<Val> {
    TIT_ASSERT(count <= page_capacity_, "Count exceeds the page capacity!");
    for (Page* page = load(head_); page != nullptr; page = page->next) {
      if (const auto old_size = fetch_and_add(page->size, count);
          old_size + count <= page_capacity_) {
        const auto index = page->size;
        return std::span{&page->first_item, page_capacity_}.subspan(index,
                                                                    count);
      }
      fetch_and_sub(page->size, count);
    }

    Page* new_page = allocate_page();
    new_page->size = count;

    Page* old_head = nullptr;
    do {
      old_head = load(head_);
      new_page->next = old_head;
    } while (!compare_exchange(head_, old_head, new_page));

    return std::span{&new_page->first_item, page_capacity_}.subspan(0, count);
  }

private:

  struct Page {
    Page* next;
    size_t size;
    Val first_item;
  };

  static auto allocate_page() -> Page* {
    Page* page = static_cast<Page*>(operator new[](
        sizeof(Page) + sizeof(Val) * (page_capacity_ - 1)));
    page->next = nullptr;
    page->size = 0;
    return page;
  }

  Page* head_ = allocate_page();
  static constexpr size_t page_capacity_ = 40960000 / sizeof(Val);

}; // class Arena

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::par
