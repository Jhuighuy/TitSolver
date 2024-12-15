/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <ranges>
#include <type_traits>
#include <utility>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/mat.hpp"
#include "tit/core/vec.hpp"

namespace tit::data {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Data type specification.
struct DataType final {
public:

  /// Data type kind.
  enum class Kind : uint8_t {
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
    count_,
  };

  /// Data type rank.
  enum class Rank : uint8_t {
    scalar,
    vector,
    matrix,
    count_,
  };

  /// Construct a data type.
  constexpr DataType(Kind kind, Rank rank, uint8_t dim)
      : kind_{kind}, rank_{rank}, dim_{dim} {
    if (kind >= Kind::count_) {
      TIT_THROW("Invalid data type kind: {}.", std::to_underlying(kind));
    }
    if (kind != Kind::unknown) {
      if (rank >= Rank::count_) {
        TIT_THROW("Invalid data type rank: {}.", std::to_underlying(rank));
      }
      if (dim <= 0) {
        TIT_THROW("Dimensionality must be positive, but is {}.", dim);
      }
      if (rank == Rank::scalar && dim != 1) {
        TIT_THROW("Dimensionality of a scalar must be 1, but is {}.", dim);
      }
    }
  }

  /// Construct a data type from integer identifier.
  constexpr explicit DataType(uint32_t id)
      : DataType{static_cast<Kind>(id & 0xFF),
                 static_cast<Rank>((id >> 8) & 0xFF),
                 static_cast<uint8_t>((id >> 16) & 0xFF)} {}

  /// Data type integer identifier.
  constexpr auto id() const -> uint32_t {
    return static_cast<uint32_t>(std::to_underlying(kind_)) |
           static_cast<uint32_t>(std::to_underlying(rank_)) << 8 |
           static_cast<uint32_t>(dim_) << 16;
  }

  /// Data type kind.
  constexpr auto kind() const noexcept -> Kind {
    return kind_;
  }

  /// Is a known data type?
  constexpr auto known() const noexcept -> bool {
    return kind_ != Kind::unknown;
  }

  /// Data type rank.
  constexpr auto rank() const noexcept -> Rank {
    TIT_ASSERT(known(), "Data type is not known!");
    return rank_;
  }

  /// Data type dimensionality. Always 1 for scalars.
  constexpr auto dim() const noexcept -> size_t {
    TIT_ASSERT(known(), "Data type is not known!");
    return dim_;
  }

  /// Compare data types.
  constexpr auto operator==(const DataType&) const -> bool = default;

private:

  Kind kind_;
  Rank rank_;
  uint8_t dim_;

}; // class Dtype

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace impl {

template<class Val>
inline constexpr auto data_kind_of = DataType::Kind::unknown;

template<>
inline constexpr auto data_kind_of<int8_t> = DataType::Kind::int8;

template<>
inline constexpr auto data_kind_of<uint8_t> = DataType::Kind::uint8;

template<>
inline constexpr auto data_kind_of<int16_t> = DataType::Kind::int16;

template<>
inline constexpr auto data_kind_of<uint16_t> = DataType::Kind::uint16;

template<>
inline constexpr auto data_kind_of<int32_t> = DataType::Kind::int32;

template<>
inline constexpr auto data_kind_of<uint32_t> = DataType::Kind::uint32;

template<>
inline constexpr auto data_kind_of<int64_t> = DataType::Kind::int64;

template<>
inline constexpr auto data_kind_of<uint64_t> = DataType::Kind::uint64;

template<>
inline constexpr auto data_kind_of<float32_t> = DataType::Kind::float32;

template<>
inline constexpr auto data_kind_of<float64_t> = DataType::Kind::float64;

template<class Val>
inline constexpr DataType data_type_of{data_kind_of<Val>,
                                       DataType::Rank::scalar,
                                       /*dim=*/1};

template<class Num, size_t Dim>
inline constexpr DataType data_type_of<Vec<Num, Dim>>{data_kind_of<Num>,
                                                      DataType::Rank::vector,
                                                      Dim};

template<class Num, size_t Dim>
inline constexpr DataType data_type_of<Mat<Num, Dim>>{data_kind_of<Num>,
                                                      DataType::Rank::matrix,
                                                      Dim};

} // namespace impl

/// Data type specification of a type.
template<class Val>
  requires std::is_object_v<Val>
inline constexpr DataType data_type_of = impl::data_type_of<Val>;

/// Data type.
template<class Val>
concept data_type = (data_type_of<std::remove_cv_t<Val>>.known());

/// Data range type.
template<class Val>
concept data_range =
    std::ranges::range<Val> && data_type<std::ranges::range_value_t<Val>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::data
