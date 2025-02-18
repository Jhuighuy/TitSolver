/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <memory>

#include "tit/py/object.hpp"

namespace tit::py {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Reference to a Python capsule.
class Capsule final : public Object {
public:

  /// Get the type object of the `Capsule`.
  static auto type() -> Type;

  /// Check if the object is a subclass of `Capsule`.
  static auto isinstance(const Object& obj) -> bool;

  /// Construct a new capsule object from C++ data.
  template<class Data>
  explicit Capsule(std::unique_ptr<Data> data)
      : Capsule(data.get(), [](PyObject* self) {
          // Destructor may be called when the capsule reference count is zero.
          // If we increase and decrease the reference count, the capsule will
          // be destroyed twice. The shenanigans below are to prevent this.
          auto obj = steal(self); // NOLINTNEXTLINE(*-static-cast-downcast)
          const auto& capsule = static_cast<const Capsule&>(obj);
          std::default_delete<Data>{}(static_cast<Data*>(capsule.data()));
          obj.release();
        }) {
    data.release();
  }

  /// Access the capsule data.
  auto data() const -> void*;

private:

  // Capsule destructor.
  using CapsuleDestructor = void (*)(PyObject*);

  // Construct a new capsule object.
  Capsule(void* data, CapsuleDestructor destructor);

}; // class Capsule

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::py
