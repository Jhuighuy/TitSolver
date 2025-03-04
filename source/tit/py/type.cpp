/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <bit>
#include <forward_list>
#include <string>
#include <unordered_map>

#include <Python.h> // IWYU pragma: keep

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/str_utils.hpp"
#include "tit/core/sys/utils.hpp"

#include "tit/py/cast.hpp"
#include "tit/py/error.hpp"
#include "tit/py/module.hpp"
#include "tit/py/object.hpp"
#include "tit/py/type.hpp"

namespace tit::py {

// NOLINTBEGIN(*-include-cleaner)

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto Type::type() -> Type {
  return borrow(&PyType_Type);
}

auto Type::isinstance(const Object& obj) -> bool {
  return ensure(PyType_Check(obj.get()));
}

auto Type::get_type() const -> PyTypeObject* {
  return std::bit_cast<PyTypeObject*>(get());
}

/// @todo Use `PyType_GetName` once we have Python 3.11.
auto Type::name() const -> std::string {
  return cast<std::string>(attr("__name__"));
}

/// @todo Use `PyType_GetQualName` once we have Python 3.11.
auto Type::qualified_name() const -> std::string {
  return cast<std::string>(attr("__qualname__"));
}

/// @todo Use `PyType_GetFullyQualifiedName` once we have Python 3.13.
auto Type::fully_qualified_name() const -> std::string {
  const auto mod_name = module_name();
  const auto qual_name = qualified_name();
  return mod_name == "builtins" ? qual_name :
                                  std::format("{}.{}", mod_name, qual_name);
}

/// @todo Use `PyType_GetModuleName` once we have Python 3.13.
auto Type::module_name() const -> std::string {
  return cast<std::string>(attr("__module__"));
}

auto Type::is_subtype_of(const Type& other) const -> bool {
  return ensure(PyType_IsSubtype(get_type(), other.get_type()));
}

Type::Type(PyObject* ptr) : Object{ptr} {
  TIT_ASSERT(isinstance(*this), "Object is not a type!");
}

auto type(const Object& obj) -> Type {
  return steal<Type>(ensure(PyObject_Type(obj.get())));
}

auto borrow(PyTypeObject* type) -> Type {
  return borrow<Type>(std::bit_cast<PyObject*>(type));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace {

// Container for type definitions.
class HeapTypeDefs final {
public:

  // Emplace a type definition.
  static auto emplace(std::string full_name,
                      size_t basic_size,
                      DestructorPtr destructor) -> PyType_Spec& {
    TIT_ASSERT(!full_name.empty(), "Class name must not be empty!");
    TIT_ASSERT(basic_size >= sizeof(PyObject), "Class basic size is invalid!");
    auto& slots = slot_arrays_.emplace_front(std::vector<PyType_Slot>{
        {.slot = Py_tp_dealloc, .pfunc = std::bit_cast<void*>(destructor)},
        {.slot = 0, .pfunc = nullptr}, // sentinel.
    });
    return specs_.emplace_front(PyType_Spec{
        .name = full_names_.emplace_front(std::move(full_name)).c_str(),
        .basicsize = static_cast<int>(basic_size),
        .itemsize = 0,
        .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HEAPTYPE,
        .slots = slots.data(),
    });
  }

private:

  // Pointers to the class definitions must be kept alive until the extension
  // is unloaded, so we need to keep them in node-based containers.
  static std::forward_list<std::string> full_names_;
  static std::forward_list<std::vector<PyType_Slot>> slot_arrays_;
  static std::forward_list<PyType_Spec> specs_;

}; // class HeapTypeDefs

std::forward_list<std::string> HeapTypeDefs::full_names_;
std::forward_list<std::vector<PyType_Slot>> HeapTypeDefs::slot_arrays_;
std::forward_list<PyType_Spec> HeapTypeDefs::specs_;

} // namespace

std::unordered_map<size_t, HeapType> HeapType::types_;

HeapType::HeapType(const std::type_info& type_info,
                   std::string_view name,
                   size_t basic_size,
                   DestructorPtr destructor,
                   const Module& module_)
    : Type{ensure(PyType_FromSpec(
          &HeapTypeDefs::emplace(std::format("{}.{}", module_.name(), name),
                                 basic_size,
                                 destructor)))} {
  if (!types_.try_emplace(type_info.hash_code(), *this).second) {
    raise_type_error("Duplicate heap type '{}' definition.", name);
  }
}

auto HeapType::find(const std::type_info& type_info) -> const HeapType& {
  const auto iter = types_.find(type_info.hash_code());
  if (iter == types_.end()) {
    raise_type_error("Heap type '{}' is not defined",
                     maybe_demangle(type_info.name()));
  }
  return iter->second;
}

auto impl::lookup_type(const std::type_info& type_info) -> const Type& {
  return HeapType::find(type_info);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// NOLINTEND(*-include-cleaner)

} // namespace tit::py
