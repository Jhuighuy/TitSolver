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

/// Parameter specification type.
enum class ParamSpecType : uint8_t {
  bool_,
  int_,
  float_,
  str,
  enum_,
  record,
};

/// Convert a specification type to a string.
auto param_spec_type_to_string(ParamSpecType type) -> std::string_view;

/// Construct a specification type from a string.
auto param_spec_type_from_string(std::string_view type) -> ParamSpecType;

/// Construct a specification type from JSON.
auto param_spec_type_from_json(const json::json& json) -> ParamSpecType;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Pointer to a parameter specification.
using ParamSpecPtr = std::unique_ptr<class ParamSpec>;

/// Abstract parameter specification.
class ParamSpec : public VirtualBase {
public:

  /// Construct a specification from JSON.
  static auto from_json(json::json json) -> ParamSpecPtr;

  /// Construct a specification from string.
  static auto from_string(std::string_view spec) -> ParamSpecPtr;

  /// Convert the specification to JSON.
  virtual auto to_json() const -> json::json = 0;

  /// Convert the specification to string.
  auto to_string() const -> std::string;

  /// Get the type of the specification.
  virtual auto type() const noexcept -> ParamSpecType = 0;

  /// Validate the value against the specification.
  virtual void validate(std::string_view value) const = 0;

private:

  std::string name_;

}; // class ParamSpec

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Pointer to a boolean parameter specification.
using BoolParamSpecPtr = std::unique_ptr<class BoolParamSpec>;

/// Boolean parameter specification.
class BoolParamSpec final : public ParamSpec {
public:

  static auto from_json(json::json& json) -> BoolParamSpecPtr;

  auto to_json() const -> json::json override;

  auto type() const noexcept -> ParamSpecType override;

  void validate(std::string_view value) const override;

private:

  std::optional<bool> default_;

}; // class BoolParamSpec

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Pointer to an integer parameter specification.
using IntParamSpecPtr = std::unique_ptr<class IntParamSpec>;

/// Integer parameter specification.
class IntParamSpec final : public ParamSpec {
public:

  static auto from_json(json::json& json) -> IntParamSpecPtr;

  auto to_json() const -> json::json override;

  auto type() const noexcept -> ParamSpecType override;

  void validate(std::string_view value) const override;

private:

  std::optional<int64_t> default_;
  std::optional<int64_t> min_;
  std::optional<int64_t> max_;

}; // class IntParamSpec

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Pointer to a floating-point parameter specification.
using FloatParamSpecPtr = std::unique_ptr<class FloatParamSpec>;

/// Floating-point parameter specification.
class FloatParamSpec final : public ParamSpec {
public:

  static auto from_json(json::json& json) -> FloatParamSpecPtr;

  auto to_json() const -> json::json override;

  auto type() const noexcept -> ParamSpecType override;

  void validate(std::string_view value) const override;

private:

  std::optional<float64_t> default_;
  std::optional<float64_t> min_;
  std::optional<float64_t> max_;
  std::optional<std::string> unit_;

}; // class FloatParamSpec

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Pointer to a string parameter specification.
using StrParamSpecPtr = std::unique_ptr<class StrParamSpec>;

/// String parameter specification.
class StrParamSpec final : public ParamSpec {
public:

  static auto from_json(json::json& json) -> StrParamSpecPtr;

  auto to_json() const -> json::json override;

  auto type() const noexcept -> ParamSpecType override;

  void validate(std::string_view value) const override;

private:

  std::optional<std::string> default_;

}; // class StrParamSpec

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Pointer to an enumeration parameter specification.
using EnumParamSpecPtr = std::unique_ptr<class EnumParamSpec>;

/// Enumeration parameter specification.
class EnumParamSpec final : public ParamSpec {
public:

  static auto from_json(json::json& json) -> EnumParamSpecPtr;

  auto to_json() const -> json::json override;

  auto type() const noexcept -> ParamSpecType override;

  void validate(std::string_view value) const override;

private:

  std::vector<std::string> options_;
  std::optional<std::string> default_;

}; // class EnumParamSpec

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Pointer to a record parameter specification.
using RecordParamSpecPtr = std::unique_ptr<class RecordParamSpec>;

/// Record parameter specification.
class RecordParamSpec final : public ParamSpec {
public:

  static auto from_json(json::json& json) -> RecordParamSpecPtr;

  auto to_json() const -> json::json override;

  auto type() const noexcept -> ParamSpecType override;

  void validate(std::string_view value) const override;

}; // class RecordParamSpec

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::data
