/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <concepts>
#include <exception>
#include <iterator>
#include <memory>
#include <ranges>
#include <span>
#include <type_traits>
#include <utility>

#include "tit/core/basic_types.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/print.hpp"
#include "tit/core/type.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Abstract input stream.
template<class Item>
  requires std::is_object_v<Item>
class InputStream : public VirtualBase {
public:

  /// Item type.
  using item_type = Item;

  /// Read the next items from the stream.
  ///
  /// @param items Buffer to store the items. Buffer size is used as the
  ///              maximum number of items to read.
  ///
  /// @returns Number of items actually read.
  constexpr virtual auto read(std::span<Item> items) -> size_t = 0;

}; // class InputStream

/// Input stream deleter base class.
template<class Item>
using InputStreamDeleterBase = std::default_delete<InputStream<Item>>;

/// Input stream deleter.
template<class Item>
struct InputStreamDeleter final : public InputStreamDeleterBase<Item> {};

/// Abstract input stream pointer.
template<class Item>
  requires std::is_object_v<Item>
using InputStreamPtr =
    std::unique_ptr<InputStream<Item>, InputStreamDeleter<Item>>;

/// Make an input stream pointer.
template<class IS, class... Args>
constexpr auto make_input_stream(Args&&... args)
    -> InputStreamPtr<typename IS::item_type> {
  using ItemType = typename IS::item_type;
  static_assert(std::derived_from<IS, tit::InputStream<ItemType>>,
                "`IS` must be derived from `InputStream`!");
  return InputStreamPtr<ItemType>{new IS{std::forward<Args>(args)...}};
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Abstract output stream.
template<class Item>
  requires std::is_object_v<Item>
class OutputStream : public VirtualBase {
public:

  /// Item type.
  using item_type = Item;

  /// Write the next items to the stream.
  constexpr virtual void write(std::span<const Item> items) = 0;

  /// Flush the object.
  /// If an exception is thrown, the object must remain in a valid state.
  constexpr virtual void flush() {
    // Default implementation does nothing.
  }

}; // class OutputStream

/// Output stream deleter.
template<class Item>
using OutputStreamDeleterBase = std::default_delete<OutputStream<Item>>;

/// Output stream deleter.
template<class Item>
struct OutputStreamDeleter final : public OutputStreamDeleterBase<Item> {
  void operator()(OutputStream<Item>* ptr) const noexcept {
    terminate_on_exception([ptr] {
      try {
        if (ptr != nullptr) ptr->flush();
      } catch (const std::exception& e) {
        err("Failed to flush: {}", e.what());
      }
    });
    OutputStreamDeleterBase<Item>::operator()(ptr);
  }
};

/// Abstract output stream pointer.
template<class Item>
  requires std::is_object_v<Item>
using OutputStreamPtr =
    std::unique_ptr<OutputStream<Item>, OutputStreamDeleter<Item>>;

/// Make an output stream pointer.
template<class OS, class... Args>
constexpr auto make_output_stream(Args&&... args)
    -> OutputStreamPtr<typename OS::item_type> {
  using ItemType = typename OS::item_type;
  static_assert(std::derived_from<OS, tit::OutputStream<ItemType>>,
                "`OS` must be derived from `OutputStream<Item>`!");
  return OutputStreamPtr<ItemType>{new OS{std::forward<Args>(args)...}};
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Range input stream.
template<std::ranges::view Range>
class RangeInputStream final :
    public InputStream<std::ranges::range_value_t<Range>> {
public:

  /// Construct a range input stream.
  constexpr explicit RangeInputStream(Range range) noexcept
      : range_{std::move(range)}, iter_{std::ranges::begin(range_)} {}

  /// Read the next items from the stream.
  constexpr auto read(std::span<std::ranges::range_value_t<Range>> items)
      -> size_t override {
    auto out_iter = items.begin();
    while (out_iter != items.end() && iter_ != std::ranges::end(range_)) {
      *out_iter = *iter_;
      ++out_iter, ++iter_;
    }
    return out_iter - items.begin();
  }

private:

  Range range_;
  std::ranges::iterator_t<Range> iter_;

}; // class RangeInputStream

template<std::ranges::viewable_range Range>
RangeInputStream(Range&&) -> RangeInputStream<std::views::all_t<Range>>;

template<std::ranges::viewable_range Range>
constexpr auto make_range_input_stream(Range&& range)
    -> InputStreamPtr<std::ranges::range_value_t<Range>> {
  return make_input_stream<RangeInputStream<std::views::all_t<Range>>>(
      std::forward<Range>(range));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Iterator output stream.
template<class Item, std::output_iterator<Item> OutIter>
class IteratorOutputStream final : public OutputStream<Item> {
public:

  /// Construct an iterator output stream.
  constexpr explicit IteratorOutputStream(OutIter out) noexcept
      : out_{std::move(out)} {}

  /// Write the next items to the stream.
  constexpr void write(std::span<const Item> items) override {
    out_ = std::ranges::copy(items, out_).out;
  }

private:

  OutIter out_;

}; // class IteratorOutputStream

/// Make an iterator output stream.
template<class Item, std::output_iterator<Item> OutIter>
constexpr auto make_iterator_output_stream(OutIter out_iter)
    -> OutputStreamPtr<Item> {
  return make_output_stream<IteratorOutputStream<Item, OutIter>>(
      std::move(out_iter));
}

/// Make a container output stream.
template<class Container>
  requires std::is_object_v<Container>
constexpr auto make_container_output_stream(Container& container)
    -> OutputStreamPtr<typename Container::value_type> {
  return make_iterator_output_stream<typename Container::value_type>(
      std::back_inserter(container));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
