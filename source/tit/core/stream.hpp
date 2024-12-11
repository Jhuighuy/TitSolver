/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <exception>
#include <iterator>
#include <memory>
#include <span>
#include <type_traits>
#include <utility>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/log.hpp"
#include "tit/core/utils.hpp"

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
      TIT_ERROR("Failed to flush: {}", e.what());
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

/// Read the items from the input stream into the container.
template<class Item, class Container>
constexpr auto read_from(InputStreamPtr<Item> stream,
                         Container& container,
                         size_t chunk_size) -> Container& {
  TIT_ASSERT(stream != nullptr, "Stream is null!");
  TIT_ASSERT(chunk_size > 0, "Chunk size must be positive!");
  while (true) {
    const auto size = std::size(container);
    container.resize(size + chunk_size);
    const auto copied = stream->read(std::span{container}.subspan(size));
    if (copied < chunk_size) {
      container.resize(size + copied);
      break;
    }
  }
  return container;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
