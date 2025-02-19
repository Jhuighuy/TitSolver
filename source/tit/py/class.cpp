/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <forward_list>
#include <string>
#include <unordered_map>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/str_utils.hpp"
#include "tit/core/sys/utils.hpp"

#include "tit/py/_python.hpp"
#include "tit/py/class.hpp"
#include "tit/py/error.hpp"

namespace tit::py::impl {

// NOLINTBEGIN(*-include-cleaner)

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace {

std::forward_list<std::string> full_names;
std::forward_list<std::vector<PyType_Slot>> slot_arrays;
std::forward_list<PyType_Spec> specs;
std::unordered_map<size_t, Type> classes;

// Make a type specification.
auto make_type_spec(std::string_view full_name,
                    size_t basic_size,
                    DestructorPtr destructor) -> PyType_Spec& {
  TIT_ASSERT(!full_name.empty(), "Class name must not be null!");
  TIT_ASSERT(basic_size >= sizeof(PyObject), "Class basic size is invalid!");
  auto& slot_array = slot_arrays.emplace_front(std::vector<PyType_Slot>{
      {.slot = Py_tp_dealloc, .pfunc = std::bit_cast<void*>(destructor)},
      {.slot = 0, .pfunc = nullptr}, // sentinel.
  });
  return specs.emplace_front(PyType_Spec{
      .name = full_names.emplace_front(full_name).c_str(),
      .basicsize = static_cast<int>(basic_size),
      .itemsize = 0,
      .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HEAPTYPE,
      .slots = slot_array.data(),
  });
}

} // namespace

auto bind_class(const std::type_info& type_info,
                std::string_view name,
                size_t basic_size,
                DestructorPtr destructor,
                const Module& module_) -> Type {
  auto full_name = std::format("{}.{}", module_.name(), name);
  auto type = steal<Type>(PyType_FromSpec(
      &make_type_spec(std::move(full_name), basic_size, destructor)));
  if (classes.try_emplace(type_info.hash_code(), type).second) return type;
  raise_type_error("Duplicate class '{}' definition.", name);
}

auto lookup_type(const std::type_info& type_info) -> Type {
  const auto iter = classes.find(type_info.hash_code());
  if (iter != classes.end()) return iter->second;
  raise_type_error("Class '{}' is not bound", maybe_demangle(type_info.name()));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// NOLINTEND(*-include-cleaner)

} // namespace tit::py::impl
