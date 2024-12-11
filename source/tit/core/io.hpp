/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <coroutine>
#include <format>
#include <iostream>
#include <iterator>
#include <optional>
#include <print>
#include <ranges>
#include <span>
#include <utility>

#include "tit/core/basic_types.hpp"
#include "tit/core/missing.hpp" // IWYU pragma: keep
#include "tit/core/utils.hpp"

namespace tit {

using std::print;
using std::println;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Print the formatted string to the standard output stream.
template<class... Args>
void eprint(std::format_string<Args...> fmt, Args&&... args) {
  print(std::cerr, fmt, std::forward<Args>(args)...);
}

/// Print the formatted string with a new line to the standard output stream.
template<class... Args>
void eprintln(std::format_string<Args...> fmt, Args&&... args) {
  println(std::cerr, fmt, std::forward<Args>(args)...);
}

/// Print the newline to the standard error stream.
inline void eprintln() {
  std::cerr << '\n';
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Abstract source of items.
template<class Item>
  requires std::is_object_v<Item>
class Source : public VirtualBase {
public:

  /// Pull the at most `count` items from the source.
  ///
  /// @returns A view of the pulled items. The data is owned by the data
  ///          source, and the view is valid until the next call to `pull` or
  ///          destruction of the source.
  constexpr virtual auto pull(size_t count) -> std::span<const Item> = 0;

}; // class Source

/// Abstract sink for items.
template<class Item>
  requires std::is_object_v<Item>
class Sink : public VirtualBase {
public:

  /// Push the items to the sink.
  virtual void push(std::span<const Item> items) = 0;

}; // class Sink

/// Abstract pipe to pump items from the source to the sink.
template<class SourceItem, class SinkItem>
  requires std::is_object_v<SourceItem> && std::is_object_v<SinkItem>
class Pipe : public VirtualBase {
public:

  /// Pipe the items from the source to the sink.
  constexpr virtual void pipe(Source<SourceItem>& source,
                              Sink<SinkItem>& sink) = 0;

}; // class Pipe

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Range of items as a source.
template<std::ranges::view Items>
class RangeSource final : public Source<std::ranges::range_value_t<Items>> {
public:

  /// Item type.
  using Item = std::ranges::range_value_t<Items>;

  /// Construct a range source.
  constexpr explicit RangeSource(Items items) noexcept
      : items_{std::move(items)} {}

  /// Pull the at most `count` items from the source.
  constexpr auto pull(size_t count) -> std::span<const Item> override {
    pulled_.clear(), pulled_.reserve(count);
    for (size_t i = 0; i < count && iter_ != std::ranges::end(items_); ++i) {
      pulled_.push_back(*iter_);
      ++iter_;
    }
    return pulled_;
  }

private:

  [[no_unique_address]] Items items_;
  [[no_unique_address]] std::ranges::iterator_t<Items> iter_;
  std::vector<Item> pulled_;

}; // class RangeSource

/// Range of items as a source (optimized for contiguous ranges).
template<std::ranges::view Items>
  requires std::ranges::contiguous_range<Items>
class RangeSource<Items> final :
    public Source<std::ranges::range_value_t<Items>> {
public:

  /// Item type.
  using Item = std::ranges::range_value_t<Items>;

  /// Construct a range source.
  constexpr explicit RangeSource(Items items) noexcept
      : items_{std::move(items)} {}

  /// Pull the at most `count` items from the source.
  constexpr auto pull(size_t count) -> std::span<const Item> override {
    count = std::min<size_t>(count, items_.end() - iter_);
    const std::span result{iter_, count};
    iter_ += count;
    return result;
  }

private:

  [[no_unique_address]] Items items_;
  [[no_unique_address]] std::ranges::iterator_t<Items> iter_;

}; // class RangeSource

template<std::ranges::viewable_range Items>
RangeSource(Items) -> RangeSource<std::views::all_t<Items>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Output iterator sink.
template<class Item, std::output_iterator<Item> OutIter>
class IterSink final : public Sink<Item> {
public:

  /// Construct an iterator sink.
  constexpr explicit IterSink(OutIter out_iter) noexcept
      : out_iter_{std::move(out_iter)} {}

  /// Push the items to the sink.
  constexpr void push(std::span<const Item> items) override {
    std::ranges::copy(items, out_iter_);
  }

private:

  [[no_unique_address]] OutIter out_iter_;

}; // class IterSink

/// Make an iterator sink.
template<class Item, std::output_iterator<Item> OutIter>
constexpr auto make_sink(OutIter out_iter) noexcept {
  return IterSink<Item, OutIter>{std::move(out_iter)};
}

/// Make a container sink.
template<class Container>
constexpr auto make_sink(Container& container) noexcept {
  return make_sink<std::ranges::range_value_t<Container>>(
      std::back_inserter(container));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

template<class SourceItem, class SinkItem>
class PipedSource : public Source<SinkItem> {
public:

  PipedSource(Source<SourceItem>& source, Pipe<SourceItem, SinkItem>& pipe)
      : source_{&source}, pipe_{&pipe} {}

  /// Pull the at most `count` items from the source.
  constexpr auto pull(size_t count) -> std::span<const SinkItem> override {
    buffer_.clear();
    while (buffer_.size() < count && coroutine_ && !coroutine_.done()) {
      coroutine_.resume();
      if (auto& item = coroutine_.promise().yielded_item; item.has_value()) {
        buffer_.push_back(std::move(*item));
      }
    }
    return buffer_;
  }

private:

  // NOLINTBEGIN(*-non-private-member-variables-in-classes)

  struct CoroutinePromise;

  using CoroutineHandle = std::coroutine_handle<CoroutinePromise>;

  struct CoroutinePromise {
    std::optional<SinkItem> yielded_item;
    std::coroutine_handle<> continuation;
    auto get_return_object() {
      return CoroutineHandle::from_promise(*this);
    }
    auto initial_suspend() {
      return std::suspend_always{};
    }
    auto final_suspend() noexcept {
      if (continuation) continuation.resume();
      return std::suspend_always{};
    }
    void return_void() {}
    void unhandled_exception() {
      throw;
    }
    auto yield_value(SinkItem item) {
      yielded_item.emplace(std::move(item));
      return std::suspend_always{};
    }
  };

  struct YieldingSink : public Sink<SinkItem> {
    CoroutinePromise* promise;
    YieldingSink(CoroutinePromise* p) : promise{p} {}
    void push(std::span<const SinkItem> items) override {
      for (const auto& item : items) promise->yield_value(item);
    }
  };

  // Coroutine that processes data through the pipe.
  auto start_pumping_() -> CoroutineHandle {
    YieldingSink sink{&coroutine_.promise()};
    pipe_->pipe(*source_, sink); // Pump data incrementally.
    co_return;
  }

  // NOLINTEND(*-non-private-member-variables-in-classes)

  Source<SourceItem>* source_;
  Pipe<SourceItem, SinkItem>* pipe_;
  CoroutineHandle coroutine_ = start_pumping_();
  std::vector<SinkItem> buffer_;

}; // class PipedSource

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
