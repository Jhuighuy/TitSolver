/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// IWYU pragma: private, include "tit/core/stream.hpp"
#pragma once

#include <type_traits>

#include "tit/core/_stream/stream.hpp"
#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Abstract input stream pointer.
template<class Item>
  requires std::is_object_v<Item>
class InputStreamReader final {
public:

  /// Iterator type.
  class Iterator final {
  public:

    // Required by iterator specification.
    using value_type = Item;
    using difference_type = ssize_t;

    /// Construct an iterator.
    constexpr explicit Iterator(InputStream<Item>* stream = nullptr)
        : stream_{stream} {
      if (stream_ != nullptr) ++(*this);
    }

    /// Increment the iterator.
    /// @{
    constexpr auto operator++() -> Iterator& {
      TIT_ASSERT(stream_ != nullptr, "Stream is null!");
      if (stream_->read({&item_, 1}) != 1) stream_ = nullptr;
      return *this;
    }
    constexpr auto operator++(int) -> Iterator {
      Iterator r{*this};
      ++(*this);
      return r;
    }
    /// @}

    /// Get the item referenced by the iterator.
    constexpr auto operator*() const -> const Item& {
      TIT_ASSERT(stream_ != nullptr, "Stream is null!");
      return item_;
    }

    /// Check if the iterator reached the end.
    friend constexpr auto operator==(Iterator a, Iterator b) noexcept -> bool {
      return a.stream_ == b.stream_;
    }

  private:

    InputStream<Item>* stream_;
    Item item_;

  }; // class Iterator

  /// Construct an input stream reader.
  constexpr explicit InputStreamReader(InputStreamPtr<Item> stream)
      : stream_{std::move(stream)} {}

  /// Iterator pointing to the first stream element.
  /// Please note that iteration over a stream consumes it.
  constexpr auto begin() -> Iterator {
    return Iterator{stream_.get()};
  }

  /// Iterator pointing to the element after the last stream element.
  constexpr auto end() const noexcept -> Iterator {
    static_cast<void>(this);
    return Iterator{};
  }

private:

  InputStreamPtr<Item> stream_ = nullptr;

}; // class InputStreamReader

template<class Item>
constexpr auto iter(InputStreamPtr<Item> stream) -> InputStreamReader<Item> {
  return InputStreamReader<Item>{std::move(stream)};
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
