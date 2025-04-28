/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// IWYU pragma: private, include "tit/core/stream.hpp"
#pragma once

#include <concepts>
#include <exception>
#include <memory>
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

} // namespace tit
