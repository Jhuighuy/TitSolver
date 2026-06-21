/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <cstdint>
#include <format>
#include <span>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

#include "tit/prop/path.hpp"

namespace tit::prop {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Validation issue code.
enum class IssueCode : std::uint8_t {
  missing_required,
  invalid_type,
  invalid_value,
  below_minimum,
  above_maximum,
  unknown_field,
  unknown_option,
};

/// Convert issue code to a stable machine-readable string.
auto issue_code_to_string(IssueCode code) noexcept -> std::string_view;

/// Set of property validation issue codes.
using IssueCodes = std::unordered_set<IssueCode>;

/// Property validation issue.
struct Issue final {
  IssueCode code;      ///< Stable machine-readable issue code.
  Path path;           ///< Path to the affected tree node.
  std::string message; ///< User-facing issue message.
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Property validation context.
class ValidationContext final {
public:

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Get suppressed issue codes.
  auto suppressions() const noexcept -> const IssueCodes&;

  /// Set suppressed issue codes.
  void set_suppressions(IssueCodes codes);

  /// Suppress @p codes, returning previously suppressed issue codes.
  auto suppress(const IssueCodes& codes) -> IssueCodes;

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Add an issue.
  /// @{
  void add_issue(Path path, IssueCode code, std::string message);
  template<class... Args>
  void add_issue(Path path,
                 IssueCode code,
                 std::format_string<Args...> message,
                 Args&&... args) {
    add_issue(std::move(path),
              code,
              std::format(message, std::forward<Args>(args)...));
  }
  /// @}

  /// Get collected issues.
  auto issues() const noexcept -> std::span<const Issue>;

  /// Check if any issues were collected.
  auto has_issues() const noexcept -> bool;

  /// Check if any issue contains give text in its message.
  auto has_issue(std::string_view text) const -> bool;

  /// Throw all collected issues, if any were collected.
  void throw_issues() const;

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  IssueCodes suppressions_;
  std::vector<Issue> issues_;

}; // class ValidationContext

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::prop
