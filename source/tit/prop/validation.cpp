/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <algorithm>
#include <format>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "tit/core/exception.hpp"
#include "tit/core/str.hpp"
#include "tit/prop/path.hpp"
#include "tit/prop/validation.hpp"

namespace tit::prop {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// IssueCode
//

auto issue_code_to_string(IssueCode code) noexcept -> std::string_view {
  using enum IssueCode;
  switch (code) {
    case missing_required: return "missing_required";
    case invalid_type:     return "invalid_type";
    case invalid_value:    return "invalid_value";
    case below_minimum:    return "below_minimum";
    case above_maximum:    return "above_maximum";
    case unknown_field:    return "unknown_field";
    case unknown_option:   return "unknown_option";
  }
  std::unreachable();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// ValidationContext
//

auto ValidationContext::suppressions() const noexcept -> const IssueCodes& {
  return suppressions_;
}

void ValidationContext::set_suppressions(IssueCodes codes) {
  suppressions_ = std::move(codes);
}

auto ValidationContext::suppress(const IssueCodes& codes) -> IssueCodes {
  auto previous_codes{suppressions_};
  suppressions_.insert_range(codes);
  return previous_codes;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void ValidationContext::add_issue(Path path,
                                  IssueCode code,
                                  std::string message) {
  if (suppressions_.contains(code)) return;
  issues_.emplace_back(code, std::move(path), std::move(message));
}

auto ValidationContext::issues() const noexcept -> std::span<const Issue> {
  return issues_;
}

auto ValidationContext::has_issues() const noexcept -> bool {
  return !issues_.empty();
}

auto ValidationContext::has_issue(std::string_view text) const -> bool {
  return std::ranges::any_of(issues_, [text](const Issue& issue) {
    return issue.message.contains(text);
  });
}

void ValidationContext::throw_issues() const {
  auto messages = issues_ | std::views::transform([](const Issue& issue) {
                    return std::format("'{}': {}", issue.path, issue.message);
                  }) |
                  std::ranges::to<std::vector>();
  std::ranges::sort(messages);

  TIT_ENSURE(messages.empty(),
             "{} issue{} detected during validation:\n"
             "\n"
             "{}",
             messages.size(),
             messages.size() == 1 ? "" : "s",
             str_join(messages, "\n"));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::prop
