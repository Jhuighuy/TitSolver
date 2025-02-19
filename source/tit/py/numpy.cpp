/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <algorithm> // IWYU pragma: keep
#include <functional>
#include <span>
#include <utility>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/utils.hpp"

#include "tit/py/_python.hpp"
#include "tit/py/error.hpp"
#include "tit/py/numpy.hpp"
#include "tit/py/object.hpp"

#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <numpy/ndarrayobject.h> // IWYU pragma: keep

namespace tit::py {

// NOLINTBEGIN(*-include-cleaner,*-cstyle-cast,*-pointer-arithmetic)

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace {

// Ensure NumPy API is imported.
void ensure_numpy_imported() {
  ensure(PyArray_ImportNumPyAPI());
}

// Convert data king to NumPy type.
auto data_kind_to_numpy(data::DataKind kind) -> NPY_TYPES {
  return translate<NPY_TYPES>(kind)
      .option(data::kind_of<char>, NPY_BYTE)
      .option(data::kind_of<unsigned char>, NPY_UBYTE)
      .option(data::kind_of<short>, NPY_SHORT)
      .option(data::kind_of<unsigned short>, NPY_USHORT)
      .option(data::kind_of<int>, NPY_INT)
      .option(data::kind_of<unsigned int>, NPY_UINT)
      .option(data::kind_of<long>, NPY_LONG)
      .option(data::kind_of<unsigned long>, NPY_ULONG)
      .option(data::kind_of<long long>, NPY_LONGLONG)
      .option(data::kind_of<unsigned long long>, NPY_ULONGLONG)
      .option(data::kind_of<float>, NPY_FLOAT)
      .option(data::kind_of<double>, NPY_DOUBLE)
      .option(data::kind_of<long double>, NPY_LONGDOUBLE);
}

// Construct a data kind from NumPy type.
auto data_kind_from_numpy(NPY_TYPES dtype) -> data::DataKind {
  return translate<data::DataKind>(dtype)
      .option(NPY_BYTE, data::kind_of<char>)
      .option(NPY_UBYTE, data::kind_of<unsigned char>)
      .option(NPY_SHORT, data::kind_of<short>)
      .option(NPY_USHORT, data::kind_of<unsigned short>)
      .option(NPY_INT, data::kind_of<int>)
      .option(NPY_UINT, data::kind_of<unsigned int>)
      .option(NPY_LONG, data::kind_of<long>)
      .option(NPY_ULONG, data::kind_of<unsigned long>)
      .option(NPY_LONGLONG, data::kind_of<long long>)
      .option(NPY_ULONGLONG, data::kind_of<unsigned long long>)
      .option(NPY_FLOAT, data::kind_of<float>)
      .option(NPY_DOUBLE, data::kind_of<double>)
      .option(NPY_LONGDOUBLE, data::kind_of<long double>)
      .fallback([](NPY_TYPES t) {
        raise_type_error("Unsupported NumPy type '{}'.", std::to_underlying(t));
      });
}

} // namespace

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto NDArray::type() -> Type {
  ensure_numpy_imported();
  return borrow(&PyArray_Type);
}

auto NDArray::isinstance(const Object& obj) -> bool {
  ensure_numpy_imported();
  return ensure(py::type(obj).is_subtype_of(type()));
}

NDArray::NDArray(data::DataKind kind,
                 byte_t* data,
                 size_t num_bytes,
                 std::span<const size_t> shape) {
  const auto num_bytes_from_shape =
      std::ranges::fold_left(shape, kind.width(), std::multiplies{});
  TIT_ASSERT(num_bytes == num_bytes_from_shape, "Invalid number of bytes!");
  TIT_ASSERT(num_bytes == 0 || data != nullptr, "Invalid data pointer!");
  ensure_numpy_imported();
  reset(ensure(PyArray_SimpleNewFromData( //
      shape.size(),
      std::bit_cast<const ssize_t*>(shape.data()),
      data_kind_to_numpy(kind),
      data)));
}

auto NDArray::get_array() const -> PyArrayObject* {
  return std::bit_cast<PyArrayObject*>(get());
}

auto NDArray::rank() const -> size_t {
  return ensure<size_t>(PyArray_NDIM(get_array()));
}

auto NDArray::shape() const -> std::span<const size_t> {
  return std::span{ensure<size_t*>(PyArray_DIMS(get_array())), rank()};
}

auto NDArray::kind() const -> data::DataKind {
  return data_kind_from_numpy(ensure<NPY_TYPES>(PyArray_TYPE(get_array())));
}

auto NDArray::elem(std::span<const ssize_t> mdindex) const
    -> std::span<byte_t> {
  TIT_ASSERT(mdindex.size() == rank(), "Invalid index size!");
  return std::span{ensure<byte_t*>(PyArray_GetPtr(get_array(), mdindex.data())),
                   ensure<size_t>(PyArray_ITEMSIZE(get_array()))};
}

auto NDArray::base() const -> Object {
  return borrow<Object>(ensure(PyArray_BASE(get_array())));
}
void NDArray::set_base(Object base) const {
  ensure(PyArray_SetBaseObject(get_array(), base.release()));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// NOLINTEND(*-include-cleaner,*-cstyle-cast,*-pointer-arithmetic)

} // namespace tit::py
