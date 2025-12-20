/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json_fwd.hpp>

#include "tit/core/basic_types.hpp"
#include "tit/core/type.hpp"

namespace tit::data {

namespace json = nlohmann;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Specification type.
enum class SpecType : uint8_t {
  bool_,
  int_,
  float_,
  string,
  enum_,
  record,
};

/// Convert a specification type to a string.
auto spec_type_to_string(SpecType type) -> std::string_view;

/// Construct a specification type from a string.
auto spec_type_from_string(std::string_view type) -> SpecType;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Pointer to a parameter specification.
using SpecPtr = std::unique_ptr<class Spec>;

/// Abstract parameter specification.
class Spec : public VirtualBase {
public:

  /// Construct a specification from JSON.
  static auto from_json(json::json json) -> SpecPtr;

  /// Construct a specification from string.
  static auto from_string(std::string_view spec) -> SpecPtr;

  /// Convert the specification to JSON.
  virtual auto to_json() const -> json::json = 0;

  /// Convert the specification to string.
  auto to_string() const -> std::string;

  /// Get the type of the specification.
  virtual auto type() const noexcept -> SpecType = 0;

  /// Validate the value against the specification.
  virtual void validate(std::string_view value) const = 0;

private:

  std::string name_;

}; // class Spec

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Pointer to a boolean parameter specification.
using BoolSpecPtr = std::unique_ptr<class BoolSpec>;

/// Boolean parameter specification.
class BoolSpec final : public Spec {
public:

  static auto from_json(json::json& json) -> BoolSpecPtr;

  auto to_json() const -> json::json override;

  auto type() const noexcept -> SpecType override;

  void validate(std::string_view value) const override;

private:

  std::optional<bool> default_;

}; // class BoolSpec

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Pointer to an integer parameter specification.
using IntSpecPtr = std::unique_ptr<class IntSpec>;

/// Integer parameter specification.
class IntSpec final : public Spec {
public:

  static auto from_json(json::json& json) -> IntSpecPtr;

  auto to_json() const -> json::json override;

  auto type() const noexcept -> SpecType override;

  void validate(std::string_view value) const override;

private:

  std::optional<int64_t> default_;
  std::optional<int64_t> min_;
  std::optional<int64_t> max_;

}; // class IntSpec

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Pointer to a floating-point parameter specification.
using FloatSpecPtr = std::unique_ptr<class FloatSpec>;

/// Floating-point parameter specification.
class FloatSpec final : public Spec {
public:

  static auto from_json(json::json& json) -> FloatSpecPtr;

  auto to_json() const -> json::json override;

  auto type() const noexcept -> SpecType override;

  void validate(std::string_view value) const override;

private:

  std::optional<float64_t> default_;
  std::optional<float64_t> min_;
  std::optional<float64_t> max_;
  std::optional<std::string> unit_;

}; // class FloatSpec

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Pointer to a string parameter specification.
using StringSpecPtr = std::unique_ptr<class StringSpec>;

/// String parameter specification.
class StringSpec final : public Spec {
public:

  static auto from_json(json::json& json) -> StringSpecPtr;

  auto to_json() const -> json::json override;

  auto type() const noexcept -> SpecType override;

  void validate(std::string_view value) const override;

private:

  std::optional<std::string> default_;

}; // class StringSpec

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Pointer to an enumeration parameter specification.
using EnumSpecPtr = std::unique_ptr<class EnumSpec>;

/// Enumeration parameter specification.
class EnumSpec final : public Spec {
public:

  static auto from_json(json::json& json) -> EnumSpecPtr;

  auto to_json() const -> json::json override;

  auto type() const noexcept -> SpecType override;

  void validate(std::string_view value) const override;

private:

  std::vector<std::string> options_;
  std::optional<std::string> default_;

}; // class EnumSpec

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Pointer to a record parameter specification.
using RecordSpecPtr = std::unique_ptr<class RecordSpec>;

/// Record parameter specification.
class RecordSpec final : public Spec {
public:

  static auto from_json(json::json& json) -> RecordSpecPtr;

  auto to_json() const -> json::json override;

  auto type() const noexcept -> SpecType override;

  void validate(std::string_view value) const override;

}; // class RecordSpec

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::data
