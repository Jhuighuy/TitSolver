/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// IWYU pragma: private, include "tit/core/stream.hpp"
#pragma once

#include <algorithm>
#include <concepts>
#include <exception>
#include <functional>
#include <iterator>
#include <memory>
#include <ranges>
#include <span>
#include <type_traits>
#include <utility>
#include <vector>

#include "tit/core/_stream/stream.hpp"
#include "tit/core/basic_types.hpp"
#include "tit/core/containers/boost.hpp"
#include "tit/core/print.hpp"
#include "tit/core/type.hpp"

namespace tit {

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

/// Generator input stream.
template<class Item, std::invocable<Item&> Generator>
class GeneratorInputStream final : public InputStream<Item> {
public:

  /// Construct a generator input stream.
  constexpr explicit GeneratorInputStream(Generator generator) noexcept
      : generator_{std::move(generator)} {}

  /// Read the next items from the stream.
  constexpr auto read(std::span<Item> items) -> size_t override {
    size_t copied = 0;
    for (auto& item : items) {
      if (!std::invoke(generator_, item)) break;
      ++copied;
    }
    return copied;
  }

private:

  [[no_unique_address]] Generator generator_;

}; // class GeneratorStream

template<class Item, std::invocable<Item&> Generator>
constexpr auto make_generator_input_stream(Generator generator)
    -> InputStreamPtr<Item> {
  using Result = GeneratorInputStream<Item, Generator>;
  return std::make_unique<Result>(std::move(generator));
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

/// Output stream that counts the number of items written.
template<class Item, std::invocable<size_t> Callback>
class CountingOutputStream final : public OutputStream<Item> {
public:

  TIT_NOT_COPYABLE_OR_MOVABLE(CountingOutputStream);

  /// Construct a tracking output stream.
  explicit CountingOutputStream(OutputStreamPtr<Item> stream, Callback callback)
      : stream_{std::move(stream)}, callback_{std::move(callback)} {}

  /// Close the stream.
  ~CountingOutputStream() override {
    try {
      callback_(written_);
    } catch (const std::exception& e) {
      error("`CountingOutputStream` callback failed: {}", e.what());
    }
  }

  /// Write the next items to the stream.
  void write(std::span<const Item> items) override {
    stream_->write(items);
    written_ += items.size();
  }

  /// Flush the stream.
  void flush() override {
    stream_->flush();
  }

private:

  OutputStreamPtr<Item> stream_;
  size_t written_ = 0;
  [[no_unique_address]] Callback callback_;

}; // class CountingOutputStream

/// Make a tracking output stream.
template<class Item, class Callback>
auto make_counting_output_stream(OutputStreamPtr<Item> stream,
                                 Callback callback) -> OutputStreamPtr<Item> {
  using Result = CountingOutputStream<Item, Callback>;
  return make_flushable<Result>(std::move(stream), std::move(callback));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Input stream transformer.
template<class SourceItem, std::invocable<SourceItem&> Proj>
class ProjectedInputStream final :
    public InputStream<std::invoke_result_t<Proj&, SourceItem&>> {
public:

  /// Item type.
  using Item = std::invoke_result_t<Proj&, SourceItem&>;

  /// Construct a transform input stream.
  constexpr explicit ProjectedInputStream(InputStreamPtr<SourceItem> stream,
                                          Proj proj) noexcept
      : stream_{std::move(stream)}, proj_{std::move(proj)} {}

  /// Read the next items from the stream.
  constexpr auto read(std::span<Item> items) -> size_t override {
    buffer_.resize(items.size());
    const auto copied = stream_->read(buffer_);
    std::ranges::transform(buffer_ | std::views::take(copied),
                           std::begin(items),
                           proj_);
    return copied;
  }

private:

  InputStreamPtr<SourceItem> stream_;
  SmallVector<SourceItem, 4> buffer_;
  [[no_unique_address]] Proj proj_;

}; // class ProjInputStream

/// Transform an input stream.
template<class SourceItem, std::invocable<SourceItem&> Proj>
constexpr auto transform_stream(InputStreamPtr<SourceItem> stream, Proj proj)
    -> InputStreamPtr<std::invoke_result_t<Proj&, SourceItem&>> {
  using Result = ProjectedInputStream<SourceItem, Proj>;
  return std::make_unique<Result>(std::move(stream), std::move(proj));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
