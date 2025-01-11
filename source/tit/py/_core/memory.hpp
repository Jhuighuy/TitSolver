/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <utility>

#include "tit/py/_core/_python.hpp"
#include "tit/py/_core/objects.hpp"

namespace tit::py {

// NOLINTBEGIN(*-include-cleaner)

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Reference to a Python capsule.
class Capsule final : public Object {
public:

  /// Check if the object is a subclass of `Capsule`.
  static constexpr auto isinstance(const Object& obj) {
    return ensure<bool>(PyCapsule_CheckExact(obj.get()));
  }

  /// Construct a new capsule object from C++ data.
  template<class Data>
    requires std::is_object_v<Data>
  explicit Capsule(Data data)
      : Object{PyCapsule_New( //
            new Data{std::move(data)},
            nullptr,
            [](PyObject* capsule) {
              // NOLINTNEXTLINE(*-owning-memory)
              delete static_cast<Data*>(PyCapsule_GetPointer(capsule, nullptr));
            })} {}

}; // class Capsule

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Reference to a Python memory view.
class MemoryView final : public Object {
public:

  /// Check if the object is a subclass of `MemoryView`.
  static constexpr auto isinstance(const Object& obj) {
    return ensure<bool>(PyMemoryView_Check(obj.get()));
  }

  /// Construct a new memory view object from C++ byte span.
  explicit MemoryView(std::span<const byte_t> data)
      : Object{PyMemoryView_FromMemory(std::bit_cast<char*>(data.data()),
                                       to_signed(data.size()),
                                       PyBUF_READ)} {}

}; // class MemoryView

/// Construct a tuple object a memory view and a capsule owning the data.
inline auto make_memory(std::vector<byte_t> data) {
  const std::span data_span{data};
  return make_tuple(MemoryView{data_span}, Capsule{std::move(data)});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// NOLINTEND(*-include-cleaner)

} // namespace tit::py
