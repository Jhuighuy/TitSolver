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

TEST_CASE("prop::ValidationContext::declare_symbol/declare_ref") {
  prop::ValidationContext context;
  SUBCASE("success") {
    SUBCASE("empty") {
      context.resolve_references();
      CHECK_FALSE(context.has_issues());
    }
    SUBCASE("reference") {
      CHECK(context.declare_symbol("materials",
                                   "steel",
                                   prop::make_path("materials", 0UZ, "id")));
      context.declare_ref("materials",
                          "steel",
                          prop::make_path("shapes", 0UZ, "material"));
      context.resolve_references();
      CHECK_FALSE(context.has_issues());
    }
    SUBCASE("forward reference") {
      context.declare_ref("materials",
                          "steel",
                          prop::make_path("shapes", 0UZ, "material"));
      CHECK(context.declare_symbol("materials",
                                   "steel",
                                   prop::make_path("materials", 0UZ, "id")));
      context.resolve_references();
      CHECK_FALSE(context.has_issues());
    }
    SUBCASE("multiple symbols") {
      const auto steel_path = prop::make_path("materials", 0UZ, "id");
      const auto glass_path = prop::make_path("materials", 1UZ, "id");
      CHECK(context.declare_symbol("materials", "steel", steel_path));
      CHECK(context.declare_symbol("materials", "glass", glass_path));
      context.resolve_references();
      CHECK_FALSE(context.has_issues());

      const auto& namespaces = context.namespaces();
      REQUIRE(namespaces.contains("materials"));
      CHECK(namespaces.at("materials").at("steel") == steel_path);
      CHECK(namespaces.at("materials").at("glass") == glass_path);
    }
    SUBCASE("same symbol in different namespaces") {
      CHECK(context.declare_symbol("materials",
                                   "steel",
                                   prop::make_path("materials", 0UZ, "id")));
      CHECK(context.declare_symbol("shapes",
                                   "steel",
                                   prop::make_path("shapes", 0UZ, "id")));
      context.resolve_references();
      CHECK_FALSE(context.has_issues());
    }
    SUBCASE("same namespace in different locations") {
      CHECK(context.declare_symbol("materials",
                                   "steel",
                                   prop::make_path("materials", 0UZ, "id")));
      CHECK(context.declare_symbol(
          "materials",
          "glass",
          prop::make_path("other_materials", 0UZ, "id")));
      context.resolve_references();
      CHECK_FALSE(context.has_issues());
    }
  }
  SUBCASE("error") {
    SUBCASE("invalid symbol") {
      CHECK_THROWS_MSG(
          context.declare_symbol("bad namespace",
                                 "steel",
                                 prop::make_path("materials", 0UZ, "id")),
          tit::Exception,
          "valid identifier");
      CHECK_THROWS_MSG(
          context.declare_symbol("namespace",
                                 "bad symbol",
                                 prop::make_path("materials", 0UZ, "id")),
          tit::Exception,
          "valid identifier");
      CHECK_THROWS_MSG(
          context.declare_ref("bad namespace",
                              "steel",
                              prop::make_path("shapes", 0UZ, "material")),
          tit::Exception,
          "valid identifier");
      CHECK_THROWS_MSG(
          context.declare_ref("namespace",
                              "bad symbol",
                              prop::make_path("shapes", 0UZ, "material")),
          tit::Exception,
          "valid identifier");
    }
    SUBCASE("duplicate symbol") {
      CHECK(context.declare_symbol("materials",
                                   "steel",
                                   prop::make_path("materials", 0UZ, "id")));
      CHECK_FALSE(
          context.declare_symbol("materials",
                                 "steel",
                                 prop::make_path("materials", 1UZ, "id")));
      CHECK(context.has_issue("Duplicate symbol"));
    }
    SUBCASE("missing symbol") {
      CHECK(context.declare_symbol("materials",
                                   "glass",
                                   prop::make_path("materials", 0UZ, "id")));
      context.declare_ref("materials",
                          "steel",
                          prop::make_path("shapes", 0UZ, "material"));
      context.resolve_references();
      CHECK(context.has_issue("not found in namespace"));
    }
    SUBCASE("missing symbol in absent namespace") {
      context.declare_ref("materials",
                          "steel",
                          prop::make_path("shapes", 0UZ, "material"));
      context.resolve_references();
      CHECK(context.has_issue("not found in namespace"));
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
