/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <algorithm>
#include <string>

#include "tit/prop/spec.hpp"
#include "tit/prop/tree.hpp"
#include "tit/prop/validation.hpp"
#include "tit/testing/test.hpp"
#include "titwcsph/case.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("wcsph::make_case_spec") {
  const auto spec = wcsph::make_case_spec();
  SUBCASE("empty tree materializes to defaults") {
    auto tree = prop::Tree{};
    CHECK_FALSE(prop::validate(*spec, tree).has_issues());
    CHECK(tree.get("schema").as_int() == wcsph::case_schema_version);
    const auto& simulation = tree.get("simulation");
    CHECK(simulation.get("title").as_string() == "Untitled Case");
    CHECK(simulation.get("end_time").as_real() == 10.0);
    CHECK(simulation.get("gravity").as_real() == 9.81);
    const auto& fluid = tree.get("fluid");
    CHECK(fluid.get("density").as_real() == 1000.0);
    CHECK(fluid.get("viscosity").as_real() == 0.001);
    const auto& geometry = tree.get("geometry");
    CHECK(geometry.get("_active").as_string() == "dam_break");
    CHECK(geometry.get("dam_break").get("water_height").as_real() == 0.6);
    CHECK(tree.get("output").get("interval").as_int() == 100);
  }
  SUBCASE("authored values are preserved") {
    auto tree = prop::tree_from_json(R"({"simulation": {"end_time": 2.5}})");
    CHECK_FALSE(prop::validate(*spec, tree).has_issues());
    CHECK(tree.get("simulation").get("end_time").as_real() == 2.5);
    CHECK(tree.get("fluid").get("density").as_real() == 1000.0);
  }
  SUBCASE("unsupported schema version is rejected") {
    auto tree = prop::tree_from_json(R"({"schema": 999})");
    const auto context = prop::validate(*spec, tree);
    REQUIRE(context.issues().size() == 1);
    CHECK(context.issues().front().code == prop::IssueCode::above_maximum);
  }
  SUBCASE("out-of-range values are diagnosed") {
    auto tree = prop::tree_from_json(R"({"fluid": {"density": -1.0}})");
    const auto context = prop::validate(*spec, tree);
    REQUIRE(context.issues().size() == 1);
    CHECK(context.issues().front().code == prop::IssueCode::below_minimum);
  }
  SUBCASE("unknown fields are diagnosed and dropped") {
    auto tree = prop::tree_from_json(R"({"bogus": 1})");
    const auto context = prop::validate(*spec, tree);
    REQUIRE(context.issues().size() == 1);
    CHECK(context.issues().front().code == prop::IssueCode::unknown_field);
    CHECK_FALSE(std::ranges::contains(tree.keys(), std::string{"bogus"}));
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("wcsph::make_case_spec::json") {
  const auto json = prop::spec_dump_json(*wcsph::make_case_spec());
  CHECK(json.contains(R"("dam_break")"));
  CHECK(json.contains(R"("record")"));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
