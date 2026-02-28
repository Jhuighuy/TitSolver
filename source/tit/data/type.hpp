/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <format>
#include <string>
#include <type_traits>
#include <utility>

#include "tit/core/basic_types.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/mat.hpp"
#include "tit/core/math.hpp"
#include "tit/core/type.hpp"
#include "tit/core/vec.hpp"

namespace tit::data {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Data kind specification.
class Kind final {
public:

  /// Kind IDs.
  enum class ID : uint8_t {
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
    unknown_,
  };

  /// Construct a data kind.
  constexpr explicit Kind(ID id) : id_{id} {
    TIT_ENSURE(id_ < ID::unknown_,
               "Invalid data kind ID: {}.",
               std::to_underlying(id_));
  }

  /// Data kind ID.
  constexpr auto id() const -> ID {
    return id_;
  }

  /// Data kind width (in bytes).
  constexpr auto width() const -> size_t {
    using enum ID;
    switch (id_) {
      case int8:    [[fallthrough]];
      case uint8:   return 1;
      case int16:   [[fallthrough]];
      case uint16:  return 2;
      case int32:   [[fallthrough]];
      case uint32:  [[fallthrough]];
      case float32: return 4;
      case int64:   [[fallthrough]];
      case uint64:  [[fallthrough]];
      case float64: return 8;
      default:      std::unreachable();
    }
  }

  /// Data kind name.
  constexpr auto name() const -> const char* {
    using enum ID;
    switch (id_) {
      case int8:    return "int8_t";
      case uint8:   return "uint8_t";
      case int16:   return "int16_t";
      case uint16:  return "uint16_t";
      case int32:   return "int32_t";
      case uint32:  return "uint32_t";
      case float32: return "float32_t";
      case int64:   return "int64_t";
      case uint64:  return "uint64_t";
      case float64: return "float64_t";
      default:      std::unreachable();
    }
  }

  /// Compare data kinds.
  constexpr auto operator==(const Kind&) const -> bool = default;

private:

  ID id_;

}; // class Kind

namespace impl {

template<class Val>
inline constexpr auto kind_id_of = Kind::ID::unknown_;

template<>
inline constexpr auto kind_id_of<int8_t> = Kind::ID::int8;

template<>
inline constexpr auto kind_id_of<uint8_t> = Kind::ID::uint8;

template<>
inline constexpr auto kind_id_of<int16_t> = Kind::ID::int16;

template<>
inline constexpr auto kind_id_of<uint16_t> = Kind::ID::uint16;

template<>
inline constexpr auto kind_id_of<int32_t> = Kind::ID::int32;

template<>
inline constexpr auto kind_id_of<uint32_t> = Kind::ID::uint32;

template<>
inline constexpr auto kind_id_of<int64_t> = Kind::ID::int64;

template<>
inline constexpr auto kind_id_of<uint64_t> = Kind::ID::uint64;

template<>
inline constexpr auto kind_id_of<float32_t> = Kind::ID::float32;

template<>
inline constexpr auto kind_id_of<float64_t> = Kind::ID::float64;

} // namespace impl

/// Class that has a known data kind.
template<class Val>
concept known_kind_of =
    std::is_object_v<Val> &&
    (impl::kind_id_of<normalize_type_t<Val>> < Kind::ID::unknown_);

/// Data kind of a class.
template<known_kind_of Val>
inline constexpr Kind kind_of = Kind{impl::kind_id_of<Val>};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Data type rank.
enum class Rank : uint8_t {
  scalar,
  vector,
  matrix,
  count_,
};

/// Data type specification.
class Type final {
public:

  /// Construct a data type.
  /// @{
  constexpr explicit Type(Kind kind) : Type{kind, Rank::scalar, 1} {}
  constexpr explicit Type(Kind kind, Rank rank, uint8_t dim)
      : kind_{kind}, rank_{rank}, dim_{dim} {
    TIT_ENSURE(rank < Rank::count_,
               "Invalid data type rank: {}.",
               std::to_underlying(rank));
    TIT_ENSURE(dim > 0, "Dimensionality must be positive, but is {}.", dim);
    TIT_ENSURE(rank != Rank::scalar || dim == 1,
               "Dimensionality of a scalar must be 1, but is {}.",
               dim);
  }
  /// @}

  /// Construct a data type from integer identifier.
  constexpr explicit Type(uint32_t id)
      : Type{Kind{static_cast<Kind::ID>((id - 1) & 0xFF)},
             static_cast<Rank>((id >> 8) & 0xFF),
             static_cast<uint8_t>((id >> 16) & 0xFF)} {}

  /// Data type integer identifier.
  constexpr auto id() const -> uint32_t {
    return static_cast<uint32_t>(std::to_underlying(kind_.id()) + 1) |
           static_cast<uint32_t>(std::to_underlying(rank_)) << 8 |
           static_cast<uint32_t>(dim_) << 16;
  }

  /// Data type kind.
  constexpr auto kind() const noexcept -> Kind {
    return kind_;
  }

  /// Data type rank.
  constexpr auto rank() const noexcept -> Rank {
    return rank_;
  }

  /// Data type dimensionality. Always 1 for scalars.
  constexpr auto dim() const noexcept -> size_t {
    return dim_;
  }

  /// Data type width (in bytes).
  constexpr auto width() const -> size_t {
    return kind().width() * pow(dim(), std::to_underlying(rank()));
  }

  /// Data type string representation.
  constexpr auto name() const -> std::string {
    using enum Rank;
    switch (rank()) {
      case scalar: return std::string{kind().name()};
      case vector: return std::format("Vec<{}, {}>", kind().name(), dim());
      case matrix: return std::format("Mat<{}, {}>", kind().name(), dim());
      default:     std::unreachable();
    }
  }

  /// Compare data types.
  constexpr auto operator==(const Type&) const -> bool = default;

private:

  Kind kind_;
  Rank rank_;
  uint8_t dim_;

}; // class Type

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace impl {

template<class Val>
inline constexpr Type type_of{kind_of<Val>};

template<known_kind_of Num, size_t Dim>
inline constexpr Type type_of<Vec<Num, Dim>>{kind_of<Num>, Rank::vector, Dim};

template<known_kind_of Num, size_t Dim>
inline constexpr Type type_of<Mat<Num, Dim>>{kind_of<Num>, Rank::matrix, Dim};

} // namespace impl

/// Class that has a known data type.
template<class Val>
concept known_type_of = requires { impl::type_of<Val>.id(); };

/// Data kind of a class.
template<known_type_of Val>
  requires std::is_object_v<Val>
inline constexpr Type type_of = impl::type_of<Val>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::data
