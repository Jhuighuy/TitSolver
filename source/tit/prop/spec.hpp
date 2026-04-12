/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"

namespace tit::prop {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Specification type.
enum class SpecType : uint8_t {
  Bool,
  Int,
  Real,
  String,
  Enum,
  Array,
  Record,
  Variant,
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Pointer to an abstract specification.
using SpecPtr = std::unique_ptr<class Spec>;

/// Abstract base for all property specifications.
class Spec {
public:

  /// Construct a specification.
  Spec() = default;

  /// Move-construct a specification.
  Spec(Spec&&) noexcept = default;

  /// Specification is not copy-constructible.
  Spec(const Spec&) = delete;

  /// Move-assign a specification.
  auto operator=(Spec&&) noexcept -> Spec& = default;

  /// Specification is not copy-assignable.
  auto operator=(const Spec&) -> Spec& = delete;

  /// Destruct the specification.
  virtual ~Spec() = default;

  /// Return the concrete type of this specification.
  virtual auto type() const noexcept -> SpecType = 0;

  /// Validate @p data, filling in any missing defaults.
  /// @p path is the dotted field path used in error messages (empty at root).
  virtual void validate(class Tree& tree, std::string_view path) const = 0;

  /// 'Box' the specification into a heap-allocated pointer.
  template<class Self>
  auto box(this Self&& self) -> SpecPtr {
    return std::make_unique<std::decay_t<Self>>(std::forward<Self>(self));
  }

}; // class Spec

/// Cast a specification to a concrete type.
template<std::derived_from<Spec> T>
auto spec_cast(const Spec& spec) -> const T& {
  const auto* const result = dynamic_cast<const T*>(&spec);
  TIT_ALWAYS_ASSERT(result != nullptr, "Invalid specification type!");
  return *result;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Boolean property specification.
class BoolSpec final : public Spec {
public:

  /// Construct a boolean specification.
  BoolSpec() = default;

  /// Get the default value.
  auto default_value() const noexcept -> std::optional<bool>;

  /// Set the default value.
  auto default_value(bool val) && -> BoolSpec&&;

  auto type() const noexcept -> SpecType override;
  void validate(Tree& tree, std::string_view path) const override;

private:

  std::optional<bool> default_;

}; // class BoolSpec

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Integer property specification.
class IntSpec final : public Spec {
public:

  /// Construct an integer specification.
  IntSpec() = default;

  /// Get the minimum value.
  auto min() const noexcept -> std::optional<int64_t>;

  /// Set the minimum value.
  auto min(int64_t val) && -> IntSpec&&;

  /// Set the maximum value.
  auto max(int64_t val) && -> IntSpec&&;

  /// Get the maximum value.
  auto max() const noexcept -> std::optional<int64_t>;

  /// Set the range of allowed values.
  auto range(int64_t min_val, int64_t max_val) && -> IntSpec&&;

  /// Set the default value.
  auto default_value(int64_t val) && -> IntSpec&&;

  /// Get the default value.
  auto default_value() const noexcept -> std::optional<int64_t>;

  auto type() const noexcept -> SpecType override;
  void validate(Tree& tree, std::string_view path) const override;

private:

  std::optional<int64_t> default_;
  std::optional<int64_t> min_;
  std::optional<int64_t> max_;

}; // class IntSpec

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Real property specification.
class RealSpec final : public Spec {
public:

  /// Construct a real specification.
  RealSpec() = default;

  /// Get the minimum value.
  auto min() const noexcept -> std::optional<float64_t>;

  /// Set the minimum value.
  auto min(float64_t val) && -> RealSpec&&;

  /// Get the maximum value.
  auto max() const noexcept -> std::optional<float64_t>;

  /// Set the maximum value.
  auto max(float64_t val) && -> RealSpec&&;

  /// Set the range of allowed values.
  auto range(float64_t min_val, float64_t max_val) && -> RealSpec&&;

  /// Get the default value.
  auto default_value() const noexcept -> std::optional<float64_t>;

  /// Set the default value.
  auto default_value(float64_t val) && -> RealSpec&&;

  auto type() const noexcept -> SpecType override;
  void validate(Tree& tree, std::string_view path) const override;

private:

  std::optional<float64_t> default_;
  std::optional<float64_t> min_;
  std::optional<float64_t> max_;

}; // class RealSpec

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// String property specification.
class StringSpec final : public Spec {
public:

  /// Construct a string specification.
  StringSpec() = default;

  /// Get the default value.
  auto default_value() const noexcept -> const std::optional<std::string>&;

  /// Set the default value.
  auto default_value(std::string val) && -> StringSpec&&;

  auto type() const noexcept -> SpecType override;
  void validate(Tree& tree, std::string_view path) const override;

private:

  std::optional<std::string> default_;

}; // class StringSpec

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Enum property specification (string restricted to a declared set of IDs).
class EnumSpec final : public Spec {
public:

  /// Option entry.
  struct Option final {
    std::string id;   ///< Identifier.
    std::string name; ///< Display name and description.
  };

  /// Construct an enum specification.
  EnumSpec() = default;

  /// Get the options.
  auto options() const noexcept -> const std::vector<Option>&;

  /// Get an option by ID.
  auto option(std::string_view id) const -> const Option*;

  /// Add an option to the enum.
  auto option(std::string_view id, std::string_view name) && -> EnumSpec&&;

  /// Get the default value.
  auto default_value() const noexcept -> const std::optional<std::string>&;

  /// Set the default value.
  auto default_value(std::string val) && -> EnumSpec&&;

  auto type() const noexcept -> SpecType override;
  void validate(Tree& tree, std::string_view path) const override;

private:

  std::vector<Option> options_;
  std::optional<std::string> default_;

}; // class EnumSpec

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Array property specification.
class ArraySpec final : public Spec {
public:

  /// Construct an array specification.
  ArraySpec() = default;

  /// Set the item specification.
  /// @{
  template<std::derived_from<Spec> ItemSpec>
  auto item(ItemSpec spec) && -> ArraySpec&& {
    return std::move(*this).item(std::move(spec).box());
  }
  auto item(SpecPtr spec) && -> ArraySpec&&;
  /// @}

  /// Get the element specification.
  auto item() const -> const Spec&;

  auto type() const noexcept -> SpecType override;
  void validate(Tree& tree, std::string_view path) const override;

private:

  SpecPtr item_spec_;

}; // class ArraySpec

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Record property specification.
class RecordSpec : public Spec {
public:

  /// Named field.
  struct Field final {
    std::string id;   ///< Identifier.
    std::string name; ///< Display name and description.
    SpecPtr spec;     ///< Specification for this field's value.
  };

  /// Construct a record specification.
  RecordSpec() = default;

  /// Get the fields.
  auto fields() const noexcept -> const std::vector<Field>&;

  /// Get a field by ID.
  auto field(std::string_view id) const -> const Field*;

  /// Add a field to the record.
  /// @{
  template<std::derived_from<Spec> FieldSpec>
  auto field(std::string_view id,
             std::string_view name,
             FieldSpec spec) && -> RecordSpec&& {
    return std::move(*this).field(id, name, std::move(spec).box());
  }
  auto field(std::string_view id,
             std::string_view name,
             SpecPtr spec) && -> RecordSpec&&;
  /// @}

  auto type() const noexcept -> SpecType override;
  void validate(Tree& tree, std::string_view path) const override;

private:

  std::vector<Field> fields_;

}; // class RecordSpec

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Variant property specification (one of several named options).
///
/// The active option is identified by the "_active" key in the data.
/// Data for non-active options is silently ignored.
class VariantSpec final : public Spec {
public:

  /// Named option.
  struct Option final {
    std::string id;   ///< Identifier.
    std::string name; ///< Display name and description.
    SpecPtr spec;     ///< Specification for this option's value.
  };

  /// Construct a variant specification.
  VariantSpec() = default;

  /// Get the options.
  auto options() const noexcept -> const std::vector<Option>&;

  /// Get an option by ID.
  auto option(std::string_view id) const -> const Option*;

  /// Add an option to the variant.
  /// @{
  template<std::derived_from<Spec> OptionSpec>
  auto option(std::string_view id,
              std::string_view name,
              OptionSpec spec) && -> VariantSpec&& {
    return std::move(*this).option(id, name, std::move(spec).box());
  }
  auto option(std::string_view id,
              std::string_view name,
              SpecPtr spec) && -> VariantSpec&&;
  /// @}

  /// Get the default value.
  auto default_value() const noexcept -> const std::optional<std::string>&;

  /// Set the default value.
  auto default_value(std::string val) && -> VariantSpec&&;

  auto type() const noexcept -> SpecType override;
  void validate(Tree& tree, std::string_view path) const override;

private:

  std::vector<Option> options_;
  std::optional<std::string> default_;

}; // class VariantSpec

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Serialize a specification to a JSON string.
auto spec_dump_json(const Spec& spec) -> std::string;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::prop
