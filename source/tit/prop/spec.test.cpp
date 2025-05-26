/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "tit/core/exception.hpp"
#include "tit/prop/spec.hpp"
#include "tit/prop/tree.hpp"
#include "tit/testing/test.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("prop::BoolSpec") {
  SUBCASE("success") {
    SUBCASE("type") {
      CHECK(prop::BoolSpec{}.type() == prop::SpecType::Bool);
    }
    SUBCASE("default_value") {
      CHECK_FALSE(prop::BoolSpec{}.default_value().has_value());
      CHECK(prop::BoolSpec{}.default_value(true).default_value() == true);
      CHECK(prop::BoolSpec{}.default_value(false).default_value() == false);
    }
  }
}

TEST_CASE("prop::BoolSpec::validate") {
  SUBCASE("success") {
    SUBCASE("null with default fills in") {
      const auto spec = prop::BoolSpec{}.default_value(true);
      auto tree = prop::Tree{};
      spec.validate(tree, "flag");
      CHECK(tree.as_bool() == true);
    }
    SUBCASE("bool accepted") {
      const prop::BoolSpec spec{};
      auto tree = prop::Tree{false};
      spec.validate(tree, "flag");
    }
  }
  SUBCASE("error") {
    SUBCASE("null without default") {
      const prop::BoolSpec spec{};
      auto tree = prop::Tree{};
      CHECK_THROWS_MSG(spec.validate(tree, "flag"),
                       tit::Exception,
                       "required boolean property is missing");
    }
    SUBCASE("non-bool") {
      const prop::BoolSpec spec{};
      auto tree = prop::Tree{1};
      CHECK_THROWS_MSG(spec.validate(tree, "flag"),
                       tit::Exception,
                       "expected boolean");
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("prop::IntSpec") {
  SUBCASE("success") {
    SUBCASE("type") {
      CHECK(prop::IntSpec{}.type() == prop::SpecType::Int);
    }
    SUBCASE("nothing") {
      const prop::IntSpec spec{};
      CHECK_FALSE(spec.min().has_value());
      CHECK_FALSE(spec.max().has_value());
      CHECK_FALSE(spec.default_value().has_value());
    }
    SUBCASE("min") {
      const auto spec = prop::IntSpec{}.min(1);
      CHECK(spec.min() == 1);
      CHECK_FALSE(spec.max().has_value());
    }
    SUBCASE("max") {
      const auto spec = prop::IntSpec{}.max(10);
      CHECK(spec.max() == 10);
      CHECK_FALSE(spec.min().has_value());
    }
    SUBCASE("range") {
      const auto spec = prop::IntSpec{}.range(1, 10);
      CHECK(spec.min() == 1);
      CHECK(spec.max() == 10);
    }
    SUBCASE("default_value") {
      const auto spec = prop::IntSpec{}.default_value(5);
      CHECK(spec.default_value() == 5);
    }
  }
  SUBCASE("error") {
    SUBCASE("min > max") {
      CHECK_THROWS_MSG(prop::IntSpec{}.max(5).min(10),
                       tit::Exception,
                       "must be <=");
    }
    SUBCASE("max < min") {
      CHECK_THROWS_MSG(prop::IntSpec{}.min(5).max(1),
                       tit::Exception,
                       "must be >=");
    }
    SUBCASE("default below min") {
      CHECK_THROWS_MSG(prop::IntSpec{}.min(1).default_value(0),
                       tit::Exception,
                       ">= min value");
    }
    SUBCASE("default above max") {
      CHECK_THROWS_MSG(prop::IntSpec{}.max(10).default_value(20),
                       tit::Exception,
                       "<= max value");
    }
  }
}

TEST_CASE("prop::IntSpec::validate") {
  SUBCASE("success") {
    SUBCASE("null with default fills in") {
      const auto spec = prop::IntSpec{}.default_value(42);
      auto tree = prop::Tree{};
      spec.validate(tree, "count");
      CHECK(tree.as_int() == 42);
    }
    SUBCASE("int in range accepted") {
      const auto spec = prop::IntSpec{}.range(1, 10);
      auto tree = prop::Tree{5};
      spec.validate(tree, "count");
    }
  }
  SUBCASE("error") {
    SUBCASE("null without default") {
      const prop::IntSpec spec{};
      auto tree = prop::Tree{};
      CHECK_THROWS_MSG(spec.validate(tree, "count"),
                       tit::Exception,
                       "required integer property is missing");
    }
    SUBCASE("below min") {
      const auto spec = prop::IntSpec{}.min(5);
      auto tree = prop::Tree{1};
      CHECK_THROWS_MSG(spec.validate(tree, "count"),
                       tit::Exception,
                       "below minimum");
    }
    SUBCASE("above max") {
      const auto spec = prop::IntSpec{}.max(5);
      auto tree = prop::Tree{10};
      CHECK_THROWS_MSG(spec.validate(tree, "count"),
                       tit::Exception,
                       "above maximum");
    }
    SUBCASE("non-int") {
      const prop::IntSpec spec{};
      auto tree = prop::Tree{true};
      CHECK_THROWS_MSG(spec.validate(tree, "count"),
                       tit::Exception,
                       "expected integer");
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("prop::RealSpec") {
  SUBCASE("success") {
    SUBCASE("type") {
      CHECK(prop::RealSpec{}.type() == prop::SpecType::Real);
    }
    SUBCASE("nothing") {
      const prop::RealSpec spec{};
      CHECK_FALSE(spec.min().has_value());
      CHECK_FALSE(spec.max().has_value());
      CHECK_FALSE(spec.default_value().has_value());
    }
    SUBCASE("min") {
      const auto spec = prop::RealSpec{}.min(0.5);
      CHECK(spec.min() == 0.5);
    }
    SUBCASE("max") {
      const auto spec = prop::RealSpec{}.max(2.5);
      CHECK(spec.max() == 2.5);
    }
    SUBCASE("range") {
      const auto spec = prop::RealSpec{}.range(0.0, 1.0);
      CHECK(spec.min() == 0.0);
      CHECK(spec.max() == 1.0);
    }
    SUBCASE("default_value") {
      const auto spec = prop::RealSpec{}.default_value(3.14);
      CHECK(spec.default_value() == 3.14);
    }
  }
  SUBCASE("error") {
    SUBCASE("min > max") {
      CHECK_THROWS_MSG(prop::RealSpec{}.max(1.0).min(2.0),
                       tit::Exception,
                       "must be <=");
    }
    SUBCASE("max < min") {
      CHECK_THROWS_MSG(prop::RealSpec{}.min(2.0).max(1.0),
                       tit::Exception,
                       "must be >=");
    }
    SUBCASE("default below min") {
      CHECK_THROWS_MSG(prop::RealSpec{}.min(1.0).default_value(0.0),
                       tit::Exception,
                       ">= min value");
    }
    SUBCASE("default above max") {
      CHECK_THROWS_MSG(prop::RealSpec{}.max(1.0).default_value(2.0),
                       tit::Exception,
                       "<= max value");
    }
  }
}

TEST_CASE("prop::RealSpec::validate") {
  SUBCASE("success") {
    SUBCASE("null with default fills in") {
      const auto spec = prop::RealSpec{}.default_value(1.5);
      auto tree = prop::Tree{};
      spec.validate(tree, "x");
      CHECK(tree.as_real() == 1.5);
    }
    SUBCASE("real accepted") {
      const prop::RealSpec spec{};
      auto tree = prop::Tree{2.0};
      spec.validate(tree, "x");
    }
    SUBCASE("int promoted to real") {
      const prop::RealSpec spec{};
      auto tree = prop::Tree{3};
      spec.validate(tree, "x");
      CHECK(tree.is_real());
      CHECK(tree.as_real() == 3.0);
    }
  }
  SUBCASE("error") {
    SUBCASE("null without default") {
      const prop::RealSpec spec{};
      auto tree = prop::Tree{};
      CHECK_THROWS_MSG(spec.validate(tree, "x"),
                       tit::Exception,
                       "required real property is missing");
    }
    SUBCASE("below min") {
      const auto spec = prop::RealSpec{}.min(1.0);
      auto tree = prop::Tree{0.5};
      CHECK_THROWS_MSG(spec.validate(tree, "x"),
                       tit::Exception,
                       "below minimum");
    }
    SUBCASE("above max") {
      const auto spec = prop::RealSpec{}.max(1.0);
      auto tree = prop::Tree{2.0};
      CHECK_THROWS_MSG(spec.validate(tree, "x"),
                       tit::Exception,
                       "above maximum");
    }
    SUBCASE("non-number") {
      const prop::RealSpec spec{};
      auto tree = prop::Tree{"not a number"};
      CHECK_THROWS_MSG(spec.validate(tree, "x"),
                       tit::Exception,
                       "expected number");
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("prop::StringSpec") {
  SUBCASE("success") {
    SUBCASE("type") {
      CHECK(prop::StringSpec{}.type() == prop::SpecType::String);
    }
    SUBCASE("default_value: absent") {
      CHECK_FALSE(prop::StringSpec{}.default_value().has_value());
    }
    SUBCASE("default_value: set") {
      const auto spec = prop::StringSpec{}.default_value("hello");
      CHECK(spec.default_value() == "hello");
    }
  }
}

TEST_CASE("prop::StringSpec::validate") {
  SUBCASE("success") {
    SUBCASE("null with default fills in") {
      const auto spec = prop::StringSpec{}.default_value("world");
      auto tree = prop::Tree{};
      spec.validate(tree, "s");
      CHECK(tree.as_string() == "world");
    }
    SUBCASE("string accepted") {
      const prop::StringSpec spec{};
      auto tree = prop::Tree{"value"};
      spec.validate(tree, "s");
    }
  }
  SUBCASE("error") {
    SUBCASE("null without default") {
      const prop::StringSpec spec{};
      auto tree = prop::Tree{};
      CHECK_THROWS_MSG(spec.validate(tree, "s"),
                       tit::Exception,
                       "required string property is missing");
    }
    SUBCASE("non-scalar") {
      const prop::StringSpec spec{};
      auto tree = prop::Tree{prop::Tree::Array{}};
      CHECK_THROWS_MSG(spec.validate(tree, "s"),
                       tit::Exception,
                       "expected string scalar");
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("prop::EnumSpec") {
  SUBCASE("success") {
    SUBCASE("type") {
      CHECK(prop::EnumSpec{}.type() == prop::SpecType::Enum);
    }
    SUBCASE("options: initially empty") {
      CHECK(prop::EnumSpec{}.options().empty());
    }
    SUBCASE("option: absent") {
      CHECK(prop::EnumSpec{}.option("missing") == nullptr);
    }
    SUBCASE("option: add and lookup") {
      const auto spec = prop::EnumSpec{} //
                            .option("a", "Option A")
                            .option("b", "Option B");
      REQUIRE(spec.options().size() == 2);
      CHECK(spec.option("a")->name == "Option A");
      CHECK(spec.option("b")->name == "Option B");
      CHECK(spec.option("c") == nullptr);
    }
    SUBCASE("default_value: absent") {
      CHECK_FALSE(prop::EnumSpec{}.default_value().has_value());
    }
    SUBCASE("default_value: valid") {
      const auto spec = prop::EnumSpec{}.option("a", "A").default_value("a");
      CHECK(spec.default_value() == "a");
    }
  }
  SUBCASE("error") {
    SUBCASE("option: invalid id") {
      CHECK_THROWS_MSG(prop::EnumSpec{}.option("bad id!", "Bad"),
                       tit::Exception,
                       "valid identifier");
    }
    SUBCASE("option: duplicate id") {
      CHECK_THROWS_MSG(
          prop::EnumSpec{}.option("a", "First").option("a", "Second"),
          tit::Exception,
          "duplicate");
    }
    SUBCASE("default_value: unknown id") {
      CHECK_THROWS_MSG(prop::EnumSpec{}.option("a", "A").default_value("z"),
                       tit::Exception,
                       "not a valid option");
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("prop::EnumSpec::validate") {
  SUBCASE("success") {
    SUBCASE("null with default fills in") {
      const auto spec = prop::EnumSpec{} //
                            .option("x", "X")
                            .option("y", "Y")
                            .default_value("y");
      auto tree = prop::Tree{};
      spec.validate(tree, "mode");
      CHECK(tree.as_string() == "y");
    }
    SUBCASE("valid string accepted") {
      const auto spec = prop::EnumSpec{} //
                            .option("a", "A")
                            .option("b", "B");
      auto tree = prop::Tree{"a"};
      spec.validate(tree, "mode");
    }
  }
  SUBCASE("error") {
    SUBCASE("null without default") {
      const auto spec = prop::EnumSpec{}.option("a", "A");
      auto tree = prop::Tree{};
      CHECK_THROWS_MSG(spec.validate(tree, "mode"), tit::Exception, "required");
    }
    SUBCASE("unknown string") {
      const auto spec = prop::EnumSpec{}.option("a", "A");
      auto tree = prop::Tree{"z"};
      CHECK_THROWS_MSG(spec.validate(tree, "mode"),
                       tit::Exception,
                       "not a valid enum value");
    }
    SUBCASE("non-scalar") {
      const auto spec = prop::EnumSpec{}.option("a", "A");
      auto tree = prop::Tree{prop::Tree::Map{}};
      CHECK_THROWS_MSG(spec.validate(tree, "mode"),
                       tit::Exception,
                       "expected string scalar");
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("prop::ArraySpec") {
  SUBCASE("success") {
    SUBCASE("type") {
      CHECK(prop::ArraySpec{}.type() == prop::SpecType::Array);
    }
    SUBCASE("item: set and retrieved") {
      const auto spec = prop::ArraySpec{}.item(prop::IntSpec{});
      CHECK(spec.item().type() == prop::SpecType::Int);
    }
  }
}

TEST_CASE("prop::ArraySpec::validate") {
  SUBCASE("success") {
    SUBCASE("null creates empty array") {
      const auto spec = prop::ArraySpec{}.item(prop::IntSpec{});
      auto tree = prop::Tree{};
      spec.validate(tree, "arr");
      CHECK(tree.is_array());
      CHECK(tree.size() == 0);
    }
    SUBCASE("elements validated") {
      const auto spec = prop::ArraySpec{}.item(prop::IntSpec{}.range(0, 9));
      auto tree = prop::Tree{prop::Tree::Array{
          prop::Tree{3},
          prop::Tree{7},
      }};
      spec.validate(tree, "arr");
    }
  }
  SUBCASE("error") {
    SUBCASE("item out of range") {
      const auto spec = prop::ArraySpec{}.item(prop::IntSpec{}.range(0, 9));
      auto tree = prop::Tree{prop::Tree::Array{
          prop::Tree{99},
      }};
      CHECK_THROWS_MSG(spec.validate(tree, "arr"),
                       tit::Exception,
                       "above maximum");
    }
    SUBCASE("items of invalid type") {
      const auto spec = prop::ArraySpec{}.item(prop::IntSpec{}.range(0, 9));
      auto tree = prop::Tree{prop::Tree::Array{
          prop::Tree{7},
          prop::Tree{"not-an-int"},
      }};
      CHECK_THROWS_MSG(spec.validate(tree, "arr"),
                       tit::Exception,
                       "expected integer");
    }
    SUBCASE("non-array") {
      const auto spec = prop::ArraySpec{}.item(prop::IntSpec{});
      auto tree = prop::Tree{"not-an-array"};
      CHECK_THROWS_MSG(spec.validate(tree, "arr"),
                       tit::Exception,
                       "expected array");
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("prop::RecordSpec") {
  SUBCASE("success") {
    SUBCASE("type") {
      CHECK(prop::RecordSpec{}.type() == prop::SpecType::Record);
    }
    SUBCASE("fields: initially empty") {
      CHECK(prop::RecordSpec{}.fields().empty());
    }
    SUBCASE("field: absent") {
      CHECK(prop::RecordSpec{}.field("missing") == nullptr);
    }
    SUBCASE("field: add and lookup") {
      const auto spec = prop::RecordSpec{}
                            .field("alpha", "Alpha", prop::BoolSpec{})
                            .field("beta", "Beta", prop::IntSpec{});
      REQUIRE(spec.fields().size() == 2);
      CHECK(spec.field("alpha")->name == "Alpha");
      CHECK(spec.field("beta")->name == "Beta");
      CHECK(spec.field("gamma") == nullptr);
    }
  }
  SUBCASE("error") {
    SUBCASE("field: invalid id") {
      CHECK_THROWS_MSG(
          prop::RecordSpec{}.field("bad field!", "Bad", prop::BoolSpec{}),
          tit::Exception,
          "valid identifier");
    }
    SUBCASE("field: duplicate id") {
      CHECK_THROWS_MSG(prop::RecordSpec{}
                           .field("dup", "First", prop::BoolSpec{})
                           .field("dup", "Second", prop::IntSpec{}),
                       tit::Exception,
                       "duplicate");
    }
  }
}

TEST_CASE("prop::RecordSpec::validate") {
  SUBCASE("success") {
    SUBCASE("null becomes map") {
      const auto spec =
          prop::RecordSpec{}.field("flag",
                                   "Flag",
                                   prop::BoolSpec{}.default_value(false));
      auto tree = prop::Tree{};
      spec.validate(tree, "rec");
      CHECK(tree.is_map());
      CHECK(tree.get("flag").as_bool() == false);
    }
    SUBCASE("known field accepted") {
      const auto spec =
          prop::RecordSpec{}.field("num",
                                   "Num",
                                   prop::IntSpec{}.default_value(0));
      auto tree = prop::Tree{prop::Tree::Map{
          {"num", prop::Tree{7}},
      }};
      CHECK_NOTHROW(spec.validate(tree, "rec"));
    }
  }
  SUBCASE("error") {
    SUBCASE("unknown field") {
      const auto spec =
          prop::RecordSpec{}.field("known",
                                   "Known",
                                   prop::BoolSpec{}.default_value(false));
      auto tree = prop::Tree{prop::Tree::Map{
          {"known", prop::Tree{false}},
          {"unknown", prop::Tree{1}},
      }};
      CHECK_THROWS_MSG(spec.validate(tree, "rec"),
                       tit::Exception,
                       "unknown field");
    }
    SUBCASE("missing field") {
      const auto spec =
          prop::RecordSpec{}.field("field", "Field", prop::BoolSpec{});
      auto tree = prop::Tree{prop::Tree::Map{}};
      CHECK_THROWS_MSG(spec.validate(tree, "rec"),
                       tit::Exception,
                       "is missing");
    }
    SUBCASE("non-map") {
      auto tree = prop::Tree{true};
      CHECK_THROWS_MSG(prop::RecordSpec{}.validate(tree, "rec"),
                       tit::Exception,
                       "expected object");
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("prop::VariantSpec") {
  SUBCASE("success") {
    SUBCASE("type") {
      CHECK(prop::VariantSpec{}.type() == prop::SpecType::Variant);
    }
    SUBCASE("options: initially empty") {
      CHECK(prop::VariantSpec{}.options().empty());
    }
    SUBCASE("option: absent") {
      CHECK(prop::VariantSpec{}.option("missing") == nullptr);
    }
    SUBCASE("option: add and lookup") {
      const auto spec = prop::VariantSpec{}
                            .option("left", "Left", prop::BoolSpec{})
                            .option("right", "Right", prop::IntSpec{});
      REQUIRE(spec.options().size() == 2);
      CHECK(spec.option("left")->name == "Left");
      CHECK(spec.option("right")->name == "Right");
      CHECK(spec.option("other") == nullptr);
    }
    SUBCASE("default_value: absent") {
      CHECK_FALSE(prop::VariantSpec{}.default_value().has_value());
    }
    SUBCASE("default_value: valid") {
      const auto spec = prop::VariantSpec{}
                            .option("a", "A", prop::BoolSpec{})
                            .default_value("a");
      CHECK(spec.default_value() == "a");
    }
  }
  SUBCASE("error") {
    SUBCASE("option: invalid id") {
      CHECK_THROWS_MSG(
          prop::VariantSpec{}.option("bad id!", "Bad", prop::BoolSpec{}),
          tit::Exception,
          "valid identifier");
    }
    SUBCASE("option: reserved id") {
      CHECK_THROWS_MSG(
          prop::VariantSpec{}.option("_active", "Bad", prop::BoolSpec{}),
          tit::Exception,
          "is reserved");
    }
    SUBCASE("option: duplicate id") {
      CHECK_THROWS_MSG(prop::VariantSpec{}
                           .option("dup", "First", prop::BoolSpec{})
                           .option("dup", "Second", prop::IntSpec{}),
                       tit::Exception,
                       "duplicate");
    }
    SUBCASE("default_value: unknown") {
      CHECK_THROWS_MSG(prop::VariantSpec{}
                           .option("a", "A", prop::BoolSpec{})
                           .default_value("z"),
                       tit::Exception,
                       "not a valid option");
    }
  }
}

TEST_CASE("prop::VariantSpec::validate") {
  SUBCASE("success") {
    SUBCASE("_active key present validates option") {
      const auto spec = prop::VariantSpec{}
                            .option("fast", "Fast", prop::BoolSpec{})
                            .option("slow", "Slow", prop::IntSpec{});
      auto tree = prop::Tree{prop::Tree::Map{
          {"_active", prop::Tree{"fast"}},
          {"fast", prop::Tree{true}},
      }};
      spec.validate(tree, "mode");
    }
    SUBCASE("null with default fills in") {
      const auto spec =
          prop::VariantSpec{}
              .option("on", "On", prop::BoolSpec{}.default_value(true))
              .option("off", "Off", prop::BoolSpec{}.default_value(false))
              .default_value("on");
      auto tree = prop::Tree{};
      spec.validate(tree, "switch");
      CHECK(tree.is_map());
      CHECK(tree.get("on").as_bool() == true);
      CHECK(tree.get("_active").as_string() == "on");
    }
  }
  SUBCASE("error") {
    SUBCASE("unknown key") {
      const auto spec = prop::VariantSpec{}
                            .option("a", "A", prop::BoolSpec{})
                            .option("b", "B", prop::IntSpec{});
      auto tree = prop::Tree{prop::Tree::Map{
          {"_active", prop::Tree{"c"}},
          {"c", prop::Tree{true}},
      }};
      CHECK_THROWS_MSG(spec.validate(tree, "mode"),
                       tit::Exception,
                       "unknown option 'c'");
    }
    SUBCASE("null without default") {
      const auto spec = prop::VariantSpec{}
                            .option("a", "A", prop::BoolSpec{})
                            .option("b", "B", prop::IntSpec{});
      auto tree = prop::Tree{};
      CHECK_THROWS_MSG(spec.validate(tree, "mode"),
                       tit::Exception,
                       "_active' must be a string");
    }
    SUBCASE("missing _active value") {
      const auto spec = prop::VariantSpec{}.option("a", "A", prop::BoolSpec{});
      auto tree = prop::Tree{prop::Tree::Map{}};
      CHECK_THROWS_MSG(spec.validate(tree, "mode"),
                       tit::Exception,
                       "_active' must be a string");
    }
    SUBCASE("invalid _active value") {
      const auto spec = prop::VariantSpec{}.option("a", "A", prop::BoolSpec{});
      auto tree = prop::Tree{prop::Tree::Map{
          {"_active", prop::Tree{"z"}},
      }};
      CHECK_THROWS_MSG(spec.validate(tree, "mode"),
                       tit::Exception,
                       "not valid");
    }
    SUBCASE("non-map") {
      const auto spec = prop::VariantSpec{}.option("a", "A", prop::BoolSpec{});
      auto tree = prop::Tree{true};
      CHECK_THROWS_MSG(spec.validate(tree, "mode"),
                       tit::Exception,
                       "expected object");
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Note: specification JSON format should be very stable, so I'm hard-coding the
//       expected output here.
TEST_CASE("prop::spec_dump_json") {
  SUBCASE("bool") {
    const auto spec = prop::BoolSpec{}.default_value(true);
    CHECK(prop::spec_dump_json(spec) == R"({
  "type": "bool",
  "default": true
})");
  }
  SUBCASE("int") {
    const auto spec = prop::IntSpec{}.range(1, 10).default_value(5);
    CHECK(prop::spec_dump_json(spec) == R"({
  "type": "int",
  "min": 1,
  "max": 10,
  "default": 5
})");
  }
  SUBCASE("real") {
    const auto spec = prop::RealSpec{}.range(1.0, 10.0).default_value(5.0);
    CHECK(prop::spec_dump_json(spec) == R"({
  "type": "real",
  "min": 1.0,
  "max": 10.0,
  "default": 5.0
})");
  }
  SUBCASE("string") {
    const auto spec = prop::StringSpec{}.default_value("hello");
    CHECK(prop::spec_dump_json(spec) == R"({
  "type": "string",
  "default": "hello"
})");
  }
  SUBCASE("enum") {
    const auto spec = prop::EnumSpec{} //
                          .option("a", "A")
                          .option("b", "B");
    CHECK(prop::spec_dump_json(spec) == R"({
  "type": "enum",
  "options": [
    {
      "id": "a",
      "name": "A"
    },
    {
      "id": "b",
      "name": "B"
    }
  ]
})");
  }
  SUBCASE("array") {
    const auto spec = prop::ArraySpec{}.item(prop::IntSpec{});
    CHECK(prop::spec_dump_json(spec) == R"({
  "type": "array",
  "item": {
    "type": "int"
  }
})");
  }
  SUBCASE("record") {
    const auto spec = prop::RecordSpec{}
                          .field("flag", "Flag", prop::BoolSpec{})
                          .field("num", "Num", prop::IntSpec{});
    CHECK(prop::spec_dump_json(spec) == R"({
  "type": "record",
  "fields": [
    {
      "id": "flag",
      "name": "Flag",
      "spec": {
        "type": "bool"
      }
    },
    {
      "id": "num",
      "name": "Num",
      "spec": {
        "type": "int"
      }
    }
  ]
})");
  }
  SUBCASE("variant") {
    const auto spec = prop::VariantSpec{}
                          .option("fast", "Fast", prop::BoolSpec{})
                          .option("slow", "Slow", prop::IntSpec{})
                          .default_value("fast");
    CHECK(prop::spec_dump_json(spec) == R"({
  "type": "variant",
  "options": [
    {
      "id": "fast",
      "name": "Fast",
      "spec": {
        "type": "bool"
      }
    },
    {
      "id": "slow",
      "name": "Slow",
      "spec": {
        "type": "int"
      }
    }
  ],
  "default": "fast"
})");
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
