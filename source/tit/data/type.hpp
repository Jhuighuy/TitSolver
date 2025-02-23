/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <format>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include "tit/core/basic_types.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/mat.hpp"
#include "tit/core/math.hpp"
#include "tit/core/utils.hpp"
#include "tit/core/vec.hpp"

namespace tit::data {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Data kind specification.
class DataKind final {
public:

  /// Kind IDs.
  enum class ID : uint8_t {
    unknown_,
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
    float128,
    count_,
  };

  /// Construct a data kind.
  constexpr explicit DataKind(ID id) : id_{id} {
    if (id_ <= ID::unknown_ || ID::count_ <= id_) {
      TIT_THROW("Invalid data kind ID: {}.", std::to_underlying(id_));
    }
  }

  /// Data kind ID.
  constexpr auto id() const -> ID {
    return id_;
  }

  /// Data kind width (in bytes).
  constexpr auto width() const -> size_t {
    using enum ID;
    return translate<size_t>(id_)
        .option(int8, 1)
        .option(uint8, 1)
        .option(int16, 2)
        .option(uint16, 2)
        .option(int32, 4)
        .option(uint32, 4)
        .option(int64, 8)
        .option(uint64, 8)
        .option(float32, 4)
        .option(float64, 8)
        .option(float128, 16);
  }

  /// Data kind name.
  constexpr auto name() const -> std::string_view {
    using enum ID;
    return translate<std::string_view>(id_)
        .option(int8, "int8_t")
        .option(uint8, "uint8_t")
        .option(int16, "int16_t")
        .option(uint16, "uint16_t")
        .option(int32, "int32_t")
        .option(uint32, "uint32_t")
        .option(int64, "int64_t")
        .option(uint64, "uint64_t")
        .option(float32, "float32_t")
        .option(float64, "float64_t")
        .option(float128, "float128_t");
  }

  /// Compare data kinds.
  constexpr auto operator==(const DataKind&) const -> bool = default;

private:

  ID id_;

}; // class DataKind

namespace impl {

template<class Val>
inline constexpr auto kind_id_of = DataKind::ID::unknown_;

template<std::signed_integral SInt>
  requires (sizeof(SInt) == 1)
inline constexpr auto kind_id_of<SInt> = DataKind::ID::int8;

template<std::unsigned_integral UInt>
  requires (sizeof(UInt) == 1)
inline constexpr auto kind_id_of<UInt> = DataKind::ID::uint8;

template<std::signed_integral SInt>
  requires (sizeof(SInt) == 2)
inline constexpr auto kind_id_of<SInt> = DataKind::ID::int16;

template<std::unsigned_integral UInt>
  requires (sizeof(UInt) == 2)
inline constexpr auto kind_id_of<UInt> = DataKind::ID::uint16;

template<std::signed_integral SInt>
  requires (sizeof(SInt) == 4)
inline constexpr auto kind_id_of<SInt> = DataKind::ID::int32;

template<std::unsigned_integral UInt>
  requires (sizeof(UInt) == 4)
inline constexpr auto kind_id_of<UInt> = DataKind::ID::uint32;

template<std::signed_integral SInt>
  requires (sizeof(SInt) == 8)
inline constexpr auto kind_id_of<SInt> = DataKind::ID::int64;

template<std::unsigned_integral UInt>
  requires (sizeof(UInt) == 8)
inline constexpr auto kind_id_of<UInt> = DataKind::ID::uint64;

template<std::floating_point Float>
  requires (sizeof(Float) == 4)
inline constexpr auto kind_id_of<Float> = DataKind::ID::float32;

template<std::floating_point Float>
  requires (sizeof(Float) == 8)
inline constexpr auto kind_id_of<Float> = DataKind::ID::float64;

template<std::floating_point Float>
  requires (sizeof(Float) == 16)
inline constexpr auto kind_id_of<Float> = DataKind::ID::float128;

} // namespace impl

/// Class that has a known data kind.
template<class Val>
concept known_kind_of =
    std::is_object_v<Val> && (DataKind::ID::unknown_ < impl::kind_id_of<Val> &&
                              impl::kind_id_of<Val> < DataKind::ID::count_);

/// Data kind of a class.
template<known_kind_of Val>
inline constexpr DataKind kind_of = DataKind{impl::kind_id_of<Val>};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Data type rank.
enum class DataRank : uint8_t {
  scalar,
  vector,
  matrix,
  count_,
};

/// Data type specification.
class DataType final {
public:

  /// Construct a data type.
  /// @{
  constexpr explicit DataType(DataKind kind)
      : DataType{kind, DataRank::scalar, 1} {}
  constexpr explicit DataType(DataKind kind, DataRank rank, uint8_t dim)
      : kind_{kind}, rank_{rank}, dim_{dim} {
    if (rank >= DataRank::count_) {
      TIT_THROW("Invalid data type rank: {}.", std::to_underlying(rank));
    }
    if (rank == DataRank::scalar && dim != 1) {
      TIT_THROW("Dimensionality of a scalar must be 1, but is {}.", dim);
    }
    if (dim <= 0) {
      TIT_THROW("Dimensionality must be positive, but is {}.", dim);
    }
  }
  /// @}

  /// Construct a data type from integer identifier.
  constexpr explicit DataType(uint32_t id)
      : DataType{DataKind{static_cast<DataKind::ID>(id & 0xFF)},
                 static_cast<DataRank>((id >> 8) & 0xFF),
                 static_cast<uint8_t>((id >> 16) & 0xFF)} {}

  /// Data type integer identifier.
  constexpr auto id() const -> uint32_t {
    return static_cast<uint32_t>(std::to_underlying(kind_.id())) |
           static_cast<uint32_t>(std::to_underlying(rank_)) << 8 |
           static_cast<uint32_t>(dim_) << 16;
  }

  /// Data type kind.
  constexpr auto kind() const noexcept -> DataKind {
    return kind_;
  }

  /// Data type rank.
  constexpr auto rank() const noexcept -> DataRank {
    return rank_;
  }

  /// Data type dimensionality. Always 1 for scalars.
  constexpr auto dim() const noexcept -> size_t {
    return dim_;
  }

  /// Data type width (in bytes).
  constexpr auto width() const -> size_t {
    return kind().width() * ipow(dim(), std::to_underlying(rank()));
  }

  /// Data type string representation.
  constexpr auto name() const -> std::string {
    using enum DataRank;
    switch (rank()) {
      case scalar: return std::string{kind().name()};
      case vector: return std::format("Vec<{}, {}>", kind().name(), dim());
      case matrix: return std::format("Mat<{}, {}>", kind().name(), dim());
      default:     std::unreachable();
    }
  }

  /// Compare data types.
  constexpr auto operator==(const DataType&) const -> bool = default;

private:

  DataKind kind_;
  DataRank rank_;
  uint8_t dim_;

}; // class DataType

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace impl {

template<class Val>
inline constexpr DataType type_of{kind_of<Val>};

template<known_kind_of Num, size_t Dim>
inline constexpr DataType type_of<Vec<Num, Dim>>{kind_of<Num>,
                                                 DataRank::vector,
                                                 Dim};

template<known_kind_of Num, size_t Dim>
inline constexpr DataType type_of<Mat<Num, Dim>>{kind_of<Num>,
                                                 DataRank::matrix,
                                                 Dim};

} // namespace impl

/// Class that has a known data type.
template<class Type>
concept known_type_of = requires { impl::type_of<Type>.id(); };

/// Data kind of a class.
template<known_type_of Type>
  requires std::is_object_v<Type>
inline constexpr DataType type_of = impl::type_of<Type>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::data
