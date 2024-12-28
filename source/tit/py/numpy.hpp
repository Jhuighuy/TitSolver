/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <array>
#include <concepts>
#include <memory>
#include <span>
#include <utility>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/containers/mdvector.hpp"

#include "tit/data/type.hpp"

#include "tit/py/capsule.hpp"
#include "tit/py/object.hpp"
#include "tit/py/sequence.hpp"

using PyArrayObject = struct tagPyArrayObject;

namespace tit::py {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// NumPy array reference.
class NDArray : public Object {
public:

  /// Get the type object of the `NDArray`.
  static auto type() -> Type;

  /// Check if the object is a subclass of `NDArray`.
  static auto isinstance(const Object& obj) -> bool;

  /// Create a new NumPy array from a multidimensional array.
  template<data::known_type_of Val, size_t Rank>
  explicit NDArray(Mdvector<Val, Rank> mdvec)
      : NDArray{data::kind_of<Val>,
                std::bit_cast<byte_t*>(mdvec.data()),
                mdvec.size() * sizeof(Val),
                mdvec.shape()} {
    set_base(Capsule{std::make_unique<Mdvector<Val, Rank>>(std::move(mdvec))});
  }

  /// Create a new NumPy array from a byte array.
  explicit NDArray(data::DataKind kind,
                   std::vector<byte_t> bytes,
                   std::span<const size_t> shape = {})
      : NDArray{kind, bytes.data(), bytes.size(), shape} {
    set_base(Capsule{std::make_unique<std::vector<byte_t>>(std::move(bytes))});
  }

  /// Get pointer to the object as `PyArrayObject*`.
  auto get_array() const -> PyArrayObject*;

  /// Get the array rank.
  auto rank() const -> size_t;

  /// Get the array shape.
  auto shape() const -> std::span<const size_t>;

  /// Get the array data kind.
  auto kind() const -> data::DataKind;

  /// Get the array element at the given index.
  /// @{
  auto elem(std::span<const ssize_t> mdindex) const -> std::span<byte_t>;
  template<data::known_kind_of Val, std::integral... Indices>
  auto elem(Indices... indices) const -> Val& {
    TIT_ASSERT(data::kind_of<Val> == kind(), "Element type mismatch!");
    const auto result = elem(std::to_array<ssize_t>({indices...}));
    TIT_ASSERT(result.size() == sizeof(Val), "Element size mismatch!");
    return *std::bit_cast<Val*>(result.data());
  }
  template<std::integral... Indices>
    requires (sizeof...(Indices) > 0)
  auto operator[](Indices... indices) const -> ItemAt<NDArray, Object> {
    if constexpr (sizeof...(Indices) == 1) return {*this, object(indices...)};
    else return {*this, make_tuple(indices...)};
  }
  /// @}

  /// Access the base object of the array.
  /// @{
  auto base() const -> Object;
  void set_base(Object base) const;
  /// @}

private:

  // Create a new NumPy array from a raw pointer.
  NDArray(data::DataKind kind,
          byte_t* data,
          size_t num_bytes,
          std::span<const size_t> shape);

}; // class NDArray

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::py
