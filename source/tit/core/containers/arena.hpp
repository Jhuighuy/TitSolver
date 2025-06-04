/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>

template<typename T>
class ArenaAllocator {
public:

  explicit ArenaAllocator(size_t page_capacity = 4096)
      : page_capacity_(page_capacity) {}

  ~ArenaAllocator() {
    for (Page* page : pages_) {
      operator delete[](page);
    }
  }

  T* allocate(size_t n) {
    assert(n <= page_capacity_); // can't fit in one page otherwise

    for (Page* page : pages_) {
      size_t index = page->next_index.fetch_add(n, std::memory_order_relaxed);
      if (index + n <= page_capacity_) {
        return reinterpret_cast<T*>(&page->storage[index]);
      } else {
        page->next_index.fetch_sub(
            n,
            std::memory_order_relaxed); // undo if overflown
      }
    }

    // Need a new page
    return allocate_in_new_page(n);
  }

  void reset() {
    for (Page* page : pages_) {
      page->next_index.store(0, std::memory_order_relaxed);
    }
  }

private:

  struct Page {
    std::atomic<size_t> next_index;
    alignas(T) std::byte storage[];

    static Page* allocate(size_t capacity) {
      size_t bytes = sizeof(Page) + sizeof(T) * capacity;
      void* mem = operator new[](bytes);
      Page* page = new (mem) Page();
      page->next_index.store(0, std::memory_order_relaxed);
      return page;
    }
  };

  T* allocate_in_new_page(size_t n) {
    std::lock_guard<std::mutex> lock(pages_mutex_);

    // Try again in case another thread already added a page
    for (Page* page : pages_) {
      size_t index = page->next_index.fetch_add(n, std::memory_order_relaxed);
      if (index + n <= page_capacity_) {
        return reinterpret_cast<T*>(&page->storage[index]);
      } else {
        page->next_index.fetch_sub(n, std::memory_order_relaxed);
      }
    }

    Page* new_page = Page::allocate(page_capacity_);
    pages_.push_back(new_page);

    T* ptr = reinterpret_cast<T*>(&new_page->storage[0]);
    new_page->next_index.store(n, std::memory_order_relaxed);
    return ptr;
  }

  const size_t page_capacity_;
  std::vector<Page*> pages_;
  std::mutex pages_mutex_; // only used when adding new pages
};
