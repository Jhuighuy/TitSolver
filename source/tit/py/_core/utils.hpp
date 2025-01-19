/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// IWYU pragma: private, include "tit/py/core.hpp"
#pragma once

#include <concepts>
#include <utility>

#include "tit/core/checks.hpp"

#include "tit/py/_core/_python.hpp"

namespace tit::py {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// NOLINTBEGIN(*-include-cleaner)

/// Smart pointer to a Python object.
class ObjPtr {
public:

  /// Construct a null pointer.
  constexpr ObjPtr() = default;

  /// Construct a pointer to the existing object.
  explicit ObjPtr(PyObject* ptr) : ptr_{ptr} {
    if (is_error_set()) raise();
  }

  /// Move-construct a pointer.
  ObjPtr(ObjPtr&& other) noexcept : ptr_{other.release()} {}

  /// Copy-construct a pointer.
  ObjPtr(const ObjPtr& other) noexcept : ptr_{Py_XNewRef(other.ptr_)} {}

  /// Move-assign a pointer.
  auto operator=(ObjPtr&& other) noexcept -> ObjPtr& {
    if (this != &other) {
      Py_CLEAR(ptr_);
      ptr_ = other.release();
    }
    return *this;
  }

  /// Copy-assign a pointer.
  auto operator=(const ObjPtr& other) -> ObjPtr& {
    if (this != &other) {
      Py_CLEAR(ptr_);
      ptr_ = Py_XNewRef(other.ptr_);
    }
    return *this;
  }

  /// Destroy the pointer.
  ~ObjPtr() noexcept {
    Py_CLEAR(ptr_);
  }

  /// Check if the pointer is not null.
  auto valid() const noexcept -> bool {
    return ptr_ != nullptr;
  }

  /// Get pointer to the object.
  auto get() const noexcept -> PyObject* {
    TIT_ASSERT(valid(), "Object is not valid!");
    return ptr_;
  }

  /// Release the pointer.
  auto release() noexcept -> PyObject* {
    return std::exchange(ptr_, nullptr);
  }

  /// Reset the pointer.
  void reset(PyObject* ptr) {
    if (ptr_ == ptr) return;
    Py_CLEAR(ptr_);
    ptr_ = ptr;
    if (is_error_set()) raise();
  }

private:

  PyObject* ptr_ = nullptr;

}; // class ObjPtr

// NOLINTEND(*-include-cleaner)

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Reference wrapper to the sequence item or slice.
template<class Self, class Index>
class ItemAt {
public:

  /// Item type.
  using Item = decltype(std::declval<Self&>().at(std::declval<Index>()));

  /// Construct a wrapper.
  constexpr ItemAt(const Self& self, Index index)
      : self_{&self}, index_{std::move(index)} {}

  /// Get the item or slice.
  template<std::constructible_from<Item> Value>
  constexpr explicit(false) operator Value() const {
    TIT_ASSERT(self_ != nullptr, "Self is invalid!");
    return Value{self_->at(index_)};
  }

  /// Assign the item or slice.
  template<class Value>
  auto operator=(Value&& value) -> ItemAt& {
    TIT_ASSERT(self_ != nullptr, "Self is invalid!");
    self_->set_at(index_, std::forward<Value>(value));
    return *this;
  }

private:

  const Self* self_;
  Index index_;

}; // class ItemAt

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::py
