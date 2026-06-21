/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <format>
#include <memory>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "tit/core/assert.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/float.hpp"
#include "tit/core/str.hpp"
#include "tit/core/utils.hpp"
#include "tit/prop/path.hpp"
#include "tit/prop/spec.hpp"
#include "tit/prop/tree.hpp"
#include "tit/prop/validation.hpp"

namespace tit::prop {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Spec
//

auto validate(const Spec& spec, Tree& tree) -> ValidationContext {
  ValidationContext context{};
  spec.validate(tree, Path{}, context);
  return context;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// BoolSpec
//

auto BoolSpec::type() const noexcept -> SpecType {
  return SpecType::Bool;
}

auto BoolSpec::default_value(bool val) && -> BoolSpec&& {
  default_ = val;
  return std::move(*this);
}

auto BoolSpec::default_value() const noexcept -> bool {
  return default_;
}

void BoolSpec::validate(Tree& tree,
                        const Path& path,
                        ValidationContext& context) const {
  // Fill default value.
  if (tree.is_null()) {
    tree.assign(default_);
    return;
  }

  // Check type.
  if (!tree.is_bool()) {
    context.add_issue(path,
                      IssueCode::invalid_type,
                      "Expected boolean, got {}.",
                      tree.type_name());
    tree.assign(default_);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// IntSpec
//

auto IntSpec::type() const noexcept -> SpecType {
  return SpecType::Int;
}

auto IntSpec::min() const noexcept -> std::optional<std::int64_t> {
  return min_;
}

auto IntSpec::min(std::int64_t val) && -> IntSpec&& {
  if (max_.has_value()) {
    TIT_ENSURE(val <= max_.value(),
               "Min value '{}' must be <= max value '{}'.",
               val,
               max_.value());
  }
  if (default_.has_value()) {
    TIT_ENSURE(val <= default_.value(),
               "Min value '{}' must be <= default value '{}'.",
               val,
               default_.value());
  }

  min_ = val;
  return std::move(*this);
}

auto IntSpec::max() const noexcept -> std::optional<std::int64_t> {
  return max_;
}

auto IntSpec::max(std::int64_t val) && -> IntSpec&& {
  if (min_.has_value()) {
    TIT_ENSURE(val >= min_.value(),
               "Max value '{}' must be >= min value '{}'.",
               val,
               min_.value());
  }
  if (default_.has_value()) {
    TIT_ENSURE(val >= default_.value(),
               "Max value '{}' must be >= default value '{}'.",
               val,
               default_.value());
  }

  max_ = val;
  return std::move(*this);
}

auto IntSpec::range(std::int64_t min_val,
                    std::int64_t max_val) && -> IntSpec&& {
  return std::move(*this).min(min_val).max(max_val);
}

auto IntSpec::default_value() const noexcept -> std::optional<std::int64_t> {
  return default_;
}

auto IntSpec::default_value(std::int64_t val) && -> IntSpec&& {
  if (min_.has_value()) {
    TIT_ENSURE(val >= min_.value(),
               "Default value '{}' must be >= min value '{}'.",
               val,
               min_.value());
  }
  if (max_.has_value()) {
    TIT_ENSURE(val <= max_.value(),
               "Default value '{}' must be <= max value '{}'.",
               val,
               max_.value());
  }

  default_ = val;
  return std::move(*this);
}

void IntSpec::validate(Tree& tree,
                       const Path& path,
                       ValidationContext& context) const {
  // Fill default value.
  if (tree.is_null()) {
    tree = default_value_tree();
    if (tree.is_null()) {
      context.add_issue(path,
                        IssueCode::missing_required,
                        "Required integer property is missing.");
    }
    return;
  }

  // Check type.
  if (!tree.is_int()) {
    context.add_issue(path,
                      IssueCode::invalid_type,
                      "Expected integer, got {}.",
                      tree.type_name());
    tree = default_value_tree();
    return;
  }

  // Check value.
  const auto val = tree.as_int();
  if (min_.has_value() && val < min_.value()) {
    context.add_issue(path,
                      IssueCode::below_minimum,
                      "Value {} is below minimum {}.",
                      val,
                      min_.value());
    tree.assign(min_.value());
    return;
  }
  if (max_.has_value() && val > max_.value()) {
    context.add_issue(path,
                      IssueCode::above_maximum,
                      "Value {} is above maximum {}.",
                      val,
                      max_.value());
    tree.assign(max_.value());
    return;
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// RealSpec
//

auto RealSpec::type() const noexcept -> SpecType {
  return SpecType::Real;
}

auto RealSpec::min() const noexcept -> std::optional<float64_t> {
  return min_;
}

auto RealSpec::min(float64_t val) && -> RealSpec&& {
  if (max_.has_value()) {
    TIT_ENSURE(val <= max_.value(),
               "Min value '{}' must be <= max value '{}'.",
               val,
               max_.value());
  }
  if (default_.has_value()) {
    TIT_ENSURE(val <= default_.value(),
               "Min value '{}' must be <= default value '{}'.",
               val,
               default_.value());
  }

  min_ = val;
  return std::move(*this);
}

auto RealSpec::max() const noexcept -> std::optional<float64_t> {
  return max_;
}

auto RealSpec::max(float64_t val) && -> RealSpec&& {
  if (min_.has_value()) {
    TIT_ENSURE(val >= min_.value(),
               "Max value '{}' must be >= min value '{}'.",
               val,
               min_.value());
  }
  if (default_.has_value()) {
    TIT_ENSURE(val >= default_.value(),
               "Max value '{}' must be >= default value '{}'.",
               val,
               default_.value());
  }

  max_ = val;
  return std::move(*this);
}

auto RealSpec::range(float64_t min_val, float64_t max_val) && -> RealSpec&& {
  return std::move(*this).min(min_val).max(max_val);
}

auto RealSpec::default_value() const noexcept -> std::optional<float64_t> {
  return default_;
}

auto RealSpec::default_value(float64_t val) && -> RealSpec&& {
  if (min_.has_value()) {
    TIT_ENSURE(val >= min_.value(),
               "Default value '{}' must be >= min value '{}'.",
               val,
               min_.value());
  }
  if (max_.has_value()) {
    TIT_ENSURE(val <= max_.value(),
               "Default value '{}' must be <= max value '{}'.",
               val,
               max_.value());
  }

  default_ = val;
  return std::move(*this);
}

void RealSpec::validate(Tree& tree,
                        const Path& path,
                        ValidationContext& context) const {
  // Fill default value.
  if (tree.is_null()) {
    tree = default_value_tree();
    if (tree.is_null()) {
      context.add_issue(path,
                        IssueCode::missing_required,
                        "Required real property is missing.");
    }
    return;
  }

  // Check type.
  if (tree.is_int()) {
    tree.assign(static_cast<float64_t>(tree.as_int()));
  } else if (!tree.is_real()) {
    context.add_issue(path,
                      IssueCode::invalid_type,
                      "Expected real, got {}.",
                      tree.type_name());
    tree = default_value_tree();
    return;
  }

  // Check value.
  const auto val = tree.as_real();
  if (min_.has_value() && val < min_.value()) {
    context.add_issue(path,
                      IssueCode::below_minimum,
                      "Value {} is below minimum {}.",
                      val,
                      min_.value());
    tree.assign(min_.value());
    return;
  }
  if (max_.has_value() && val > max_.value()) {
    context.add_issue(path,
                      IssueCode::above_maximum,
                      "Value {} is above maximum {}.",
                      val,
                      max_.value());
    tree.assign(max_.value());
    return;
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// StringSpec
//

auto StringSpec::type() const noexcept -> SpecType {
  return SpecType::String;
}

auto StringSpec::default_value() const noexcept
    -> const std::optional<std::string>& {
  return default_;
}

auto StringSpec::default_value(std::string val) && -> StringSpec&& {
  default_ = std::move(val);
  return std::move(*this);
}

void StringSpec::validate(Tree& tree,
                          const Path& path,
                          ValidationContext& context) const {
  // Fill default value.
  if (tree.is_null()) {
    tree = default_value_tree();
    if (tree.is_null()) {
      context.add_issue(path,
                        IssueCode::missing_required,
                        "Required string property is missing.");
    }
    return;
  }

  // Check type.
  if (tree.is_int()) {
    tree.assign(std::format("{}", tree.as_int()));
    return;
  }
  if (tree.is_real()) {
    tree.assign(std::format("{}", tree.as_real()));
    return;
  }
  if (!tree.is_string()) {
    context.add_issue(path,
                      IssueCode::invalid_type,
                      "Expected string, got {}.",
                      tree.type_name());
    tree = default_value_tree();
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// EnumSpec
//

auto EnumSpec::type() const noexcept -> SpecType {
  return SpecType::Enum;
}

auto EnumSpec::options() const noexcept -> const std::vector<Option>& {
  return options_;
}

auto EnumSpec::option(std::string_view id) const -> const Option* {
  const auto iter = std::ranges::find(options_, id, &Option::id);
  return iter == options_.end() ? nullptr : std::to_address(iter);
}

auto EnumSpec::option(std::string_view id,
                      std::string_view name) && -> EnumSpec&& {
  TIT_ENSURE(str_is_identifier(id),
             "Enum option ID '{}' must be a valid identifier.",
             id);
  TIT_ENSURE(option(id) == nullptr, "Enum option ID '{}' is duplicate.", id);

  options_.emplace_back(std::string{id}, std::string{name});
  return std::move(*this);
}

auto EnumSpec::default_value() const noexcept
    -> const std::optional<std::string>& {
  return default_;
}

auto EnumSpec::default_value(std::string val) && -> EnumSpec&& {
  if (!options_.empty()) {
    TIT_ENSURE(option(val) != nullptr,
               "Enum default value '{}' is not a valid option ID.",
               val);
  }

  default_ = std::move(val);
  return std::move(*this);
}

void EnumSpec::validate(Tree& tree,
                        const Path& path,
                        ValidationContext& context) const {
  // Fill default value.
  if (tree.is_null()) {
    tree = default_value_tree();
    if (tree.is_null()) {
      context.add_issue(path,
                        IssueCode::missing_required,
                        "Required enum property is missing.");
    }
    return;
  }

  // Check type.
  if (!tree.is_string()) {
    context.add_issue(path,
                      IssueCode::invalid_type,
                      "Expected string, got {}.",
                      tree.type_name());
    tree = default_value_tree();
    return;
  }

  // Check value.
  if (const auto val = tree.as_string(); option(val) == nullptr) {
    context.add_issue(path,
                      IssueCode::invalid_value,
                      "Value '{}' is not a valid enum value. "
                      "Expected one of: {}.",
                      val,
                      options_ | std::views::transform(&Option::id));
    tree = default_value_tree();
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// ArraySpec
//

auto ArraySpec::type() const noexcept -> SpecType {
  return SpecType::Array;
}

auto ArraySpec::item() const -> const Spec& {
  TIT_ASSERT(item_spec_ != nullptr, "Item spec is not set!");
  return *item_spec_;
}

auto ArraySpec::item(SpecPtr spec) && -> ArraySpec&& {
  TIT_ASSERT(spec != nullptr, "Array item spec must not be null.");

  item_spec_ = std::move(spec);
  return std::move(*this);
}

void ArraySpec::validate(Tree& tree,
                         const Path& path,
                         ValidationContext& context) const {
  // Fill default value.
  if (tree.is_null()) {
    tree.assign(Tree::Array{});
    return;
  }

  // Check type.
  if (!tree.is_array()) {
    context.add_issue(path,
                      IssueCode::invalid_type,
                      "Expected array, got {}.",
                      tree.type_name());
    tree.assign(Tree::Array{});
    return;
  }

  // Validate each element.
  for (std::size_t i = 0; i < tree.size(); ++i) {
    item().validate(tree.get(i), path.child(i), context);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// RecordSpec
//

auto RecordSpec::type() const noexcept -> SpecType {
  return SpecType::Record;
}

auto RecordSpec::fields() const noexcept -> const std::vector<Field>& {
  return fields_;
}

auto RecordSpec::field(std::string_view id) const -> const Field* {
  const auto iter = std::ranges::find(fields_, id, &Field::id);
  return iter == fields_.end() ? nullptr : std::to_address(iter);
}

auto RecordSpec::field(std::string_view id,
                       std::string_view name,
                       SpecPtr spec) && -> RecordSpec&& {
  TIT_ASSERT(spec != nullptr, "Record field spec must not be null.");
  TIT_ENSURE(str_is_identifier(id),
             "Record field ID '{}' must be a valid identifier.",
             id);
  TIT_ENSURE(field(id) == nullptr, "Record field ID '{}' is duplicate.", id);

  fields_.emplace_back(std::string{id}, std::string{name}, std::move(spec));
  return std::move(*this);
}

void RecordSpec::validate(Tree& tree,
                          const Path& path,
                          ValidationContext& context) const {
  // Fill default value.
  if (tree.is_null()) tree.assign(Tree::Map{});

  // Check type.
  if (!tree.is_map()) {
    context.add_issue(path,
                      IssueCode::invalid_type,
                      "Expected object (record), got {}.",
                      tree.type_name());
    tree.assign(Tree::Map{});
  }

  // Check for unknown keys.
  for (const auto& key : tree.keys()) {
    if (field(key) == nullptr) {
      context.add_issue(path,
                        IssueCode::unknown_field,
                        "Unknown field '{}'.",
                        key);
      tree.erase(key);
    }
  }

  // Validate each field.
  for (const auto& field : fields_) {
    field.spec->validate(tree.get(field.id), path.child(field.id), context);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// VariantSpec
//

auto VariantSpec::type() const noexcept -> SpecType {
  return SpecType::Variant;
}

auto VariantSpec::options() const noexcept -> const std::vector<Option>& {
  return options_;
}

auto VariantSpec::option(std::string_view id) const -> const Option* {
  const auto iter = std::ranges::find(options_, id, &Option::id);
  return iter == options_.end() ? nullptr : std::to_address(iter);
}

auto VariantSpec::option(std::string_view id,
                         std::string_view name,
                         SpecPtr spec) && -> VariantSpec&& {
  TIT_ASSERT(spec != nullptr, "Variant option spec must not be null.");
  TIT_ENSURE(id != "_active", "Variant option ID '_active' is reserved.");
  TIT_ENSURE(str_is_identifier(id),
             "Variant option ID '{}' must be a valid identifier.",
             id);
  TIT_ENSURE(option(id) == nullptr, "Variant option ID '{}' is duplicate.", id);

  options_.emplace_back(std::string{id}, std::string{name}, std::move(spec));
  return std::move(*this);
}

auto VariantSpec::default_value() const noexcept
    -> const std::optional<std::string>& {
  return default_;
}

auto VariantSpec::default_value(std::string val) && -> VariantSpec&& {
  if (!options_.empty()) {
    TIT_ENSURE(option(val) != nullptr,
               "Variant default value '{}' is not a valid option ID.",
               val);
  }

  default_ = std::move(val);
  return std::move(*this);
}

void VariantSpec::validate(Tree& tree,
                           const Path& path,
                           ValidationContext& context) const {
  // Fill default value.
  if (tree.is_null()) tree.assign(Tree::Map{});

  // Check type.
  if (!tree.is_map()) {
    context.add_issue(path,
                      IssueCode::invalid_type,
                      "Expected object (variant), got {}.",
                      tree.type_name());
    tree.assign(Tree::Map{});
  }

  // Check for unknown keys.
  for (const auto& key : tree.keys()) {
    if (key != "_active" && option(key) == nullptr) {
      context.add_issue(path,
                        IssueCode::unknown_option,
                        "Unknown option '{}'. "
                        "Expected one of: {}.",
                        key,
                        options_ | std::views::transform(&Option::id));
      tree.erase(key);
    }
  }

  // Get the active option.
  auto& active_node = tree.get("_active");

  // Fill default active option.
  if (active_node.is_null()) {
    if (default_.has_value()) {
      active_node.assign(default_.value());
    } else {
      // Infer active option if possible.
      const auto present_keys =
          tree.keys() |
          std::views::filter([](const auto& key) { return key != "_active"; }) |
          std::ranges::to<std::vector>();
      if (present_keys.size() == 1) {
        active_node.assign(present_keys.front());
      } else if (present_keys.size() > 1) {
        context.add_issue(path,
                          IssueCode::missing_required,
                          "Variant has multiple options but no "
                          "'_active' key. Provide '_active' key with "
                          "one of: {}.",
                          options_ | std::views::transform(&Option::id));
      }
    }
    if (active_node.is_null()) {
      context.add_issue(path,
                        IssueCode::missing_required,
                        "Variant has no active option. "
                        "Provide '_active' key with one of: {}.",
                        options_ | std::views::transform(&Option::id));
    }
  }

  // Check active option type.
  if (!active_node.is_null() && !active_node.is_string()) {
    context.add_issue(path.child("_active"),
                      IssueCode::invalid_type,
                      "Value of '_active' must be a string, got {}.",
                      active_node.type_name());
    active_node = Tree{};
  }

  // Check active option value.
  if (!active_node.is_null()) {
    const auto active_id = active_node.as_string();
    const auto* const active_option = option(active_id);
    if (active_option == nullptr) {
      context.add_issue(path.child("_active"),
                        IssueCode::invalid_value,
                        "Variant active option '{}' is not valid. "
                        "Expected one of: {}.",
                        active_id,
                        options_ | std::views::transform(&Option::id));
      active_node = Tree{};
    }
  }

  // Validate inactive options in a relaxed mode.
  // In absence of active option, all options are considered inactive.
  for (const auto& option : options_) {
    if (!tree.has(option.id)) continue;
    if (!active_node.is_null() && option.id == active_node.as_string()) {
      continue;
    }
    const auto previous_suppressions = context.suppress({
        IssueCode::missing_required,
    });
    const Defer restore_suppressions{[&context, previous_suppressions] {
      context.set_suppressions(previous_suppressions);
    }};
    option.spec->validate(tree.get(option.id), path.child(option.id), context);
  }

  // Validate the active option.
  if (!active_node.is_null()) {
    const auto active_id = active_node.as_string();
    if (const auto* const active_option = option(active_id);
        active_option != nullptr) {
      active_option->spec->validate(tree.get(active_id),
                                    path.child(active_id),
                                    context);
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::prop
