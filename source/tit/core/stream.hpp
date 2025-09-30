/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
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
#include "tit/core/print.hpp"
#include "tit/core/type.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Abstract input stream.
template<class Item>
  requires std::is_object_v<Item>
class InputStream : public VirtualBase {
public:

  /// Read the next items from the stream.
  ///
  /// @param items Buffer to store the items. Buffer size is used as the
  ///              maximum number of items to read.
  ///
  /// @returns Number of items actually read.
  constexpr virtual auto read(std::span<Item> items) -> size_t = 0;

}; // class InputStream

/// Abstract input stream pointer.
template<class Item>
  requires std::is_object_v<Item>
using InputStreamPtr = std::unique_ptr<InputStream<Item>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Abstract flushable object.
class Flushable : public VirtualBase {
public:

  /// Flush the object.
  /// If an exception is thrown, the object must remain in a valid state.
  virtual void flush() = 0;

}; // class Flushable

/// Deleter that flushes the flushable object.
struct FlushDeleter : std::default_delete<Flushable> {
  constexpr void operator()(Flushable* stream) const noexcept {
    if (stream == nullptr) return;
    try {
      stream->flush();
    } catch (const std::exception& e) {
      err("Failed to flush: {}", e.what());
    }
    std::default_delete<Flushable>::operator()(stream);
  }
};

/// Flushable object pointer.
template<std::derived_from<Flushable> Derived>
using FlushablePtr = std::unique_ptr<Derived, FlushDeleter>;

/// Make a flushable object pointer.
template<std::derived_from<Flushable> Derived, class... Args>
constexpr auto make_flushable(Args&&... args) -> FlushablePtr<Derived> {
  return {new Derived{std::forward<Args>(args)...}, {}};
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Abstract output stream.
template<class Item>
  requires std::is_object_v<Item>
class OutputStream : public Flushable {
public:

  /// Write the next items to the stream.
  constexpr virtual void write(std::span<const Item> items) = 0;

}; // class OutputStream

/// Abstract output stream pointer.
template<class Item>
  requires std::is_object_v<Item>
using OutputStreamPtr = FlushablePtr<OutputStream<Item>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Range input stream.
template<std::ranges::view Items>
class RangeInputStream final :
    public InputStream<std::ranges::range_value_t<Items>> {
public:

  /// Construct a range input stream.
  constexpr explicit RangeInputStream(Items items) noexcept
      : items_{std::move(items)}, iter_{std::ranges::begin(items_)} {}

  /// Read the next items from the stream.
  constexpr auto read(std::span<std::ranges::range_value_t<Items>> items)
      -> size_t override {
    auto out_iter = items.begin();
    while (out_iter != items.end() && iter_ != std::ranges::end(items_)) {
      *out_iter = *iter_;
      ++out_iter, ++iter_;
    }
    return out_iter - items.begin();
  }

private:

  Items items_;
  std::ranges::iterator_t<Items> iter_;

}; // class RangeInputStream

template<std::ranges::viewable_range Items>
RangeInputStream(Items&&) -> RangeInputStream<std::views::all_t<Items>>;

template<std::ranges::viewable_range Items>
constexpr auto make_range_input_stream(Items&& items)
    -> InputStreamPtr<std::ranges::range_value_t<Items>> {
  using Result = RangeInputStream<std::views::all_t<Items>>;
  return std::make_unique<Result>(std::forward<Items>(items));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Iterator output stream.
template<class Item, std::output_iterator<Item> OutIter>
class IteratorOutputStream final : public OutputStream<Item> {
public:

  /// Construct an iterator output stream.
  constexpr explicit IteratorOutputStream(OutIter out_iter) noexcept
      : out_iter_{std::move(out_iter)} {}

  /// Write the next items to the stream.
  constexpr void write(std::span<const Item> items) override {
    out_iter_ = std::ranges::copy(items, out_iter_).out;
  }

  /// Flush the stream.
  constexpr void flush() override {
    // Nothing to do.
  }

private:

  OutIter out_iter_;

}; // class IteratorOutputStream

/// Make an iterator output stream.
template<class Item, std::output_iterator<Item> OutIter>
constexpr auto make_iterator_output_stream(OutIter out_iter)
    -> OutputStreamPtr<Item> {
  return make_flushable<IteratorOutputStream<Item, OutIter>>(
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
