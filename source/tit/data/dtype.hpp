/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <type_traits>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/mat.hpp"
#include "tit/core/type_traits.hpp"
#include "tit/core/vec.hpp"

namespace tit::data {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Data type kind.
enum class Dkind : uint8_t {
  unknown,
  int8,
  uint8,
  int16,
  uint16,
  int32,
  uint32,
  int64,
  uint64,
  float32,
  float64,
  count,
};

/// Data type specification.
struct Dtype final {
public:

  /// Construct a data type.
  consteval Dtype(Dkind kind, uint16_t size, uint8_t dim, uint8_t rank)
      : kind_{kind}, size_{size}, dim_{dim}, rank_{rank} {
    TIT_ENSURE(kind_ < Dkind::count, "Data type kind is out of range!");
    TIT_ENSURE(rank_ <= 2,
               "Rank can be either 0 (scalar), 1 (vector), or 2 (matrix)!");
    TIT_ENSURE(rank_ > 0 || dim_ == 1,
               "Scalars must have dimensiality equal to 1!");
  }

  /// Data type kind.
  constexpr auto kind() const noexcept -> Dkind {
    return kind_;
  }

  /// Is a known data type?
  constexpr auto known() const noexcept -> bool {
    return kind_ != Dkind::unknown;
  }

  /// Size of the data type value (in bytes).
  constexpr auto size() const noexcept -> size_t {
    return size_;
  }

  /// Data type dimensiality. Always 1 for scalars.
  constexpr auto dim() const noexcept -> size_t {
    return dim_;
  }

  /// Data type rank. 0 for scalars, 1 for vectors, and 2 for matrices.
  constexpr auto rank() const noexcept -> size_t {
    return rank_;
  }

  /// Compare data types.
  constexpr auto operator==(const Dtype&) const -> bool = default;

  // We want to use `Dtype` as a non-type template parameter, therefore, it
  // must be a structural type, so all the members must be public.
  // NOLINTBEGIN(*-non-private-member-variables-in-classes)
  Dkind kind_;
  uint16_t size_;
  uint8_t dim_;
  uint8_t rank_;
  // NOLINTEND(*-non-private-member-variables-in-classes)

}; // class Dtype

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace impl {

template<class Val>
struct dkind_of : value_constant<Dkind::unknown> {};

template<>
struct dkind_of<int8_t> : value_constant<Dkind::int8> {};

template<>
struct dkind_of<uint8_t> : value_constant<Dkind::uint8> {};

template<>
struct dkind_of<int16_t> : value_constant<Dkind::int16> {};

template<>
struct dkind_of<uint16_t> : value_constant<Dkind::uint16> {};

template<>
struct dkind_of<int32_t> : value_constant<Dkind::int32> {};

template<>
struct dkind_of<uint32_t> : value_constant<Dkind::uint32> {};

template<>
struct dkind_of<int64_t> : value_constant<Dkind::int64> {};

template<>
struct dkind_of<uint64_t> : value_constant<Dkind::uint64> {};

template<>
struct dkind_of<float32_t> : value_constant<Dkind::float32> {};

template<>
struct dkind_of<float64_t> : value_constant<Dkind::float64> {};

template<class Val>
struct dtype_of :
    value_constant<Dtype{dkind_of<Val>::value,
                         sizeof(Val),
                         /*dim=*/1,
                         /*rank=*/0}> {};

template<class Num, size_t Dim>
struct dtype_of<Vec<Num, Dim>> :
    value_constant<Dtype{dkind_of<Num>::value,
                         sizeof(Vec<Num, Dim>),
                         Dim,
                         /*rank=*/1}> {};

template<class Num, size_t Dim>
struct dtype_of<Mat<Num, Dim>> :
    value_constant<Dtype{dkind_of<Num>::value,
                         sizeof(Mat<Num, Dim>),
                         Dim,
                         /*rank=*/2}> {};

} // namespace impl

/// Data type kind of a type.
template<class Val>
  requires std::is_object_v<Val>
inline constexpr Dkind dkind_of_v = impl::dkind_of<Val>::value;

/// Data type specification of a type.
template<class Val>
  requires std::is_object_v<Val>
inline constexpr Dtype dtype_of_v = impl::dtype_of<Val>::value;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::data
