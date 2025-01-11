/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// IWYU pragma: private, include "tit/py/core.hpp"
#pragma once

#include <bit>
#include <format>

#include "tit/core/basic_types.hpp"
#include "tit/core/str_utils.hpp"

#include "tit/py/_bind/func.hpp" // IWYU pragma: export
#include "tit/py/_bind/module.hpp"
#include "tit/py/core.hpp"

namespace tit::py::cpp {

// NOLINTBEGIN(*-include-cleaner)

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

template<class Self>
struct ClassData {
  PyObject_HEAD
  Self self;
};

template<class Self>
auto get_self(PyObject* obj) -> Self& {
  return std::bit_cast<ClassData<Self>*>(obj)->self;
}

#pragma GCC diagnostic push // allow `offsetof` for non-POD types.
#pragma GCC diagnostic ignored "-Winvalid-offsetof"

template<class Self>
auto find(const Self& self) -> Object {
  static const auto offset = offsetof(ClassData<Self>, self);
  return borrow(
      std::bit_cast<PyObject*>(std::bit_cast<const byte_t*>(&self) - offset));
}

#pragma GCC diagnostic pop

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

template<class Self>
class Class : public Object {
public:

  /// Check if the object is a subclass of `Class`.
  static constexpr auto isinstance(const Object& obj) {
    return ensure<bool>(PyType_Check(obj.get()));
  }

  /// Define a `__init__` method in the class that prevents the user from
  /// creating instances of the class.
  /// This method is called automatically once the class is defined.
  void def_noinit() const {
    static auto def = make_noinit_def<Self>();
    const auto descr = steal<Object>(PyDescr_NewMethod(get_type(), &def));
    set_attr("__init__", descr);
  }

  /// Define a `__init__` method in the class.
  template<param_spec... Params>
  void def_init() const {
    static auto def = make_init_def<Self, Params...>();
    const auto descr = steal<Object>(PyDescr_NewMethod(get_type(), &def));
    set_attr("__init__", descr);
  }

  /// Define a new method in the class.
  template<StrLiteral Name, auto Method, param_spec... Params>
    requires method_spec<Method, Self, Params...>
  void def() const {
    static auto def = make_method_def<Name, Method, Self, Params...>();
    const auto descr = steal<Object>(PyDescr_NewMethod(get_type(), &def));
    set_attr(Name.c_str(), descr);
  }

  /// Define a new property in the class.
  template<StrLiteral Name, auto Get, auto Set = nullptr>
    requires getset_spec<Self, Get, Set>
  void prop() const {
    static auto def = make_getset_def<Name, Self, Get, Set>();
    const auto descr = steal<Object>(PyDescr_NewGetSet(get_type(), &def));
    set_attr(Name.c_str(), descr);
  }

  /// Get the type object pointer.
  auto get_type() const -> PyTypeObject* {
    return std::bit_cast<PyTypeObject*>(get());
  }

  /// Create a new instance of the class.
  auto create(Self self) const -> Object {
    TIT_ASSERT(valid(), "Class is not initialized!");
    auto* pyself =
        static_cast<ClassData<Self>*>(PyObject_Malloc(sizeof(ClassData<Self>)));
    PyObject_Init(&pyself->ob_base, get_type());
    new (&pyself->self) Self{std::move(self)};
    impl::incref_parent(self);
    return Object{&pyself->ob_base};
  }

}; // class Class

template<class Self>
auto class_holder() -> Class<Self>& {
  static Class<Self> c;
  return c;
}

/// Create a new class object.
template<StrLiteral Name, class Self>
auto class_(const Module& m) -> Class<Self> {
  static auto full_name = std::format("{}.{}", m.name(), Name.c_str());
  static auto slots = std::to_array<PyType_Slot>({
      {.slot = 0, .pfunc = nullptr}, // End of slots
  });
  static PyType_Spec spec{
      .name = full_name.c_str(),
      .basicsize = sizeof(ClassData<Self>),
      .itemsize = 0,
      .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HEAPTYPE,
      .slots = slots.data(),
  };
  auto& c = class_holder<Self>();
  c = steal<Class<Self>>(PyType_FromSpec(&spec));
  c.def_noinit();
  m.add(Name.c_str(), c);
  return c;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// NOLINTEND(*-include-cleaner)

} // namespace tit::py::cpp

template<class Type>
struct tit::py::impl::Converter {
  static auto object(Type type) -> Object {
    return cpp::class_holder<Type>().create(std::move(type));
  }
  static auto extract(const Object& obj) -> Type& {
    return cpp::get_self<Type>(obj.get());
  }
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
