/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "tit/core/exception.hpp"
#include "tit/prop/path.hpp"
#include "tit/prop/validation.hpp"
#include "tit/testing/test.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("prop::ValidationContext::add_issue") {
  prop::ValidationContext context{};

  context.add_issue(prop::Path{}.child("issueB"),
                    prop::IssueCode::invalid_type,
                    "First issue.");
  context.add_issue(prop::Path{}.child("issueA"),
                    prop::IssueCode::missing_required,
                    "Second issue.");

  CHECK(context.has_issues());
  CHECK(context.has_issue("First issue."));
  CHECK(context.has_issue("Second issue."));
  CHECK_THROWS_MSG(context.throw_issues(),
                   tit::Exception,
                   "2 issues detected during validation:\n"
                   "\n"
                   "'/issueA': Second issue.\n"
                   "'/issueB': First issue.");
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("prop::ValidationContext::suppress") {
  prop::ValidationContext context{};

  const auto previous_codes =
      context.suppress({prop::IssueCode::missing_required});
  CHECK(previous_codes.empty());
  CHECK(context.suppressions().contains(prop::IssueCode::missing_required));

  context.add_issue(prop::Path{},
                    prop::IssueCode::missing_required,
                    "Suppressed issue.");
  context.add_issue(prop::Path{},
                    prop::IssueCode::invalid_type,
                    "Reported issue.");

  CHECK_FALSE(context.has_issue("Suppressed issue."));
  CHECK(context.has_issue("Reported issue."));

  context.set_suppressions(previous_codes);
  CHECK_FALSE(
      context.suppressions().contains(prop::IssueCode::missing_required));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
