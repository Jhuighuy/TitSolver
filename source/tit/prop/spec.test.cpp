/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "tit/core/exception.hpp"
#include "tit/prop/spec.hpp"
#include "tit/prop/tree.hpp"
#include "tit/prop/unit.hpp"
#include "tit/prop/validation.hpp"
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
      CHECK(prop::BoolSpec{}.default_value() == false);
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
      CHECK_FALSE(prop::validate(spec, tree).has_issues());
      CHECK(tree.as_bool() == true);
    }
    SUBCASE("null without explicit default fills in false") {
      const prop::BoolSpec spec{};
      auto tree = prop::Tree{};
      CHECK_FALSE(prop::validate(spec, tree).has_issues());
      CHECK(tree.as_bool() == false);
    }
    SUBCASE("bool accepted") {
      const prop::BoolSpec spec{};
      auto tree = prop::Tree{false};
      CHECK_FALSE(prop::validate(spec, tree).has_issues());
    }
  }
  SUBCASE("error") {
    SUBCASE("non-bool") {
      const prop::BoolSpec spec{};
      auto tree = prop::Tree{1};
      CHECK(prop::validate(spec, tree).has_issue("Expected boolean"));
      CHECK(tree.as_bool() == false);
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
      CHECK_FALSE(prop::validate(spec, tree).has_issues());
      CHECK(tree.as_int() == 42);
    }
    SUBCASE("int in range accepted") {
      const auto spec = prop::IntSpec{}.range(1, 10);
      auto tree = prop::Tree{5};
      CHECK_FALSE(prop::validate(spec, tree).has_issues());
    }
  }
  SUBCASE("error") {
    SUBCASE("null without default") {
      const prop::IntSpec spec{};
      auto tree = prop::Tree{};
      CHECK(prop::validate(spec, tree).has_issue("property is missing"));
      CHECK(tree.is_null());
    }
    SUBCASE("below min") {
      const auto spec = prop::IntSpec{}.min(5);
      auto tree = prop::Tree{1};
      CHECK(prop::validate(spec, tree).has_issue("below minimum"));
      CHECK(tree.as_int() == 5);
    }
    SUBCASE("above max") {
      const auto spec = prop::IntSpec{}.max(5);
      auto tree = prop::Tree{10};
      CHECK(prop::validate(spec, tree).has_issue("above maximum"));
      CHECK(tree.as_int() == 5);
    }
    SUBCASE("non-int") {
      const prop::IntSpec spec{};
      auto tree = prop::Tree{true};
      CHECK(prop::validate(spec, tree).has_issue("Expected integer"));
      CHECK(tree.is_null());
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
    SUBCASE("unit") {
      const auto spec = prop::RealSpec{}.unit(prop::Unit{"kg/m^3"});
      const auto& unit = spec.unit();
      REQUIRE(unit.has_value());
      CHECK(unit->symbol() == "kg/m^3"); // NOLINT(*-unchecked-optional-access)
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
      CHECK_FALSE(prop::validate(spec, tree).has_issues());
      CHECK(tree.as_real() == 1.5);
    }
    SUBCASE("real accepted") {
      const prop::RealSpec spec{};
      auto tree = prop::Tree{2.0};
      CHECK_FALSE(prop::validate(spec, tree).has_issues());
    }
    SUBCASE("int promoted to real") {
      const prop::RealSpec spec{};
      auto tree = prop::Tree{3};
      CHECK_FALSE(prop::validate(spec, tree).has_issues());
      CHECK(tree.is_real());
      CHECK(tree.as_real() == 3.0);
    }
    SUBCASE("string with implied unit accepted") {
      const auto spec = prop::RealSpec{}.unit(prop::Unit{"kg/m^3"});
      auto tree = prop::Tree{"7850.0"};
      CHECK_FALSE(prop::validate(spec, tree).has_issues());
      CHECK(tree.is_real());
      CHECK(tree.as_real() == 7850.0);
    }
    SUBCASE("string with matching explicit unit accepted") {
      const auto spec = prop::RealSpec{}.unit(prop::Unit{"kg/m^3"});
      auto tree = prop::Tree{"7850.0 kg/m^3"};
      CHECK_FALSE(prop::validate(spec, tree).has_issues());
      CHECK(tree.is_real());
      CHECK(tree.as_real() == 7850.0);
    }
  }
  SUBCASE("error") {
    SUBCASE("null without default") {
      const prop::RealSpec spec{};
      auto tree = prop::Tree{};
      CHECK(prop::validate(spec, tree).has_issue("property is missing"));
      CHECK(tree.is_null());
    }
    SUBCASE("below min") {
      const auto spec = prop::RealSpec{}.min(1.0);
      auto tree = prop::Tree{0.5};
      CHECK(prop::validate(spec, tree).has_issue("below minimum"));
      CHECK(tree.as_real() == 1.0);
    }
    SUBCASE("above max") {
      const auto spec = prop::RealSpec{}.max(1.0);
      auto tree = prop::Tree{2.0};
      CHECK(prop::validate(spec, tree).has_issue("above maximum"));
      CHECK(tree.as_real() == 1.0);
    }
    SUBCASE("non-number") {
      const prop::RealSpec spec{};
      auto tree = prop::Tree{"not a number"};
      CHECK(prop::validate(spec, tree).has_issue("Expected real"));
      CHECK(tree.is_null());
    }
    SUBCASE("wrong explicit unit") {
      const auto spec = prop::RealSpec{}.unit(prop::Unit{"kg/m^3"});
      auto tree = prop::Tree{"7.85 g/cm^3"};
      CHECK(prop::validate(spec, tree)
                .has_issue("unit conversion is not implemented yet"));
      CHECK(tree.is_null());
    }
    SUBCASE("explicit unit for unitless spec") {
      const prop::RealSpec spec{};
      auto tree = prop::Tree{"1.0 m"};
      CHECK(prop::validate(spec, tree).has_issue("Expected real"));
      CHECK(tree.is_null());
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
      CHECK_FALSE(prop::validate(spec, tree).has_issues());
      CHECK(tree.as_string() == "world");
    }
    SUBCASE("string accepted") {
      const prop::StringSpec spec{};
      auto tree = prop::Tree{"value"};
      CHECK_FALSE(prop::validate(spec, tree).has_issues());
    }
  }
  SUBCASE("error") {
    SUBCASE("null without default") {
      const prop::StringSpec spec{};
      auto tree = prop::Tree{};
      CHECK(prop::validate(spec, tree).has_issue("string property is missing"));
      CHECK(tree.is_null());
    }
    SUBCASE("non-scalar") {
      const prop::StringSpec spec{};
      auto tree = prop::Tree{prop::Tree::Array{}};
      CHECK(prop::validate(spec, tree).has_issue("Expected string"));
      CHECK(tree.is_null());
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
      CHECK_FALSE(prop::validate(spec, tree).has_issues());
      CHECK(tree.as_string() == "y");
    }
    SUBCASE("valid string accepted") {
      const auto spec = prop::EnumSpec{} //
                            .option("a", "A")
                            .option("b", "B");
      auto tree = prop::Tree{"a"};
      CHECK_FALSE(prop::validate(spec, tree).has_issues());
    }
  }
  SUBCASE("error") {
    SUBCASE("null without default") {
      const auto spec = prop::EnumSpec{}.option("a", "A");
      auto tree = prop::Tree{};
      CHECK(prop::validate(spec, tree).has_issue("enum property is missing"));
      CHECK(tree.is_null());
    }
    SUBCASE("unknown string") {
      const auto spec = prop::EnumSpec{}.option("a", "A");
      auto tree = prop::Tree{"z"};
      CHECK(prop::validate(spec, tree).has_issue("not a valid enum value"));
      CHECK(tree.is_null());
    }
    SUBCASE("non-scalar") {
      const auto spec = prop::EnumSpec{}.option("a", "A");
      auto tree = prop::Tree{prop::Tree::Map{}};
      CHECK(prop::validate(spec, tree).has_issue("Expected string"));
      CHECK(tree.is_null());
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("prop::ArraySpec") {
  SUBCASE("success") {
    SUBCASE("type") {
      CHECK(prop::ArraySpec{}.type() == prop::SpecType::Array);
    }
    SUBCASE("size: absent") {
      CHECK_FALSE(prop::ArraySpec{}.size().has_value());
    }
    SUBCASE("size: set") {
      const auto spec = prop::ArraySpec{}.size(3);
      CHECK(spec.size() == 3);
    }
    SUBCASE("item: set and retrieved") {
      const auto spec = prop::ArraySpec{}.item(prop::IntSpec{});
      CHECK(spec.item().type() == prop::SpecType::Int);
    }
    SUBCASE("item: entity record") {
      const auto spec = prop::ArraySpec{}.item(
          prop::RecordSpec{}.field("id", "ID", prop::SymbolSpec{"namespace"}));
      CHECK_RANGE_EQ(spec.item().namespaces(), {"namespace"});
    }
  }
  SUBCASE("error") {
    SUBCASE("only arrays of entity records are allowed") {
      CHECK_THROWS_MSG(
          prop::ArraySpec{}.item(prop::RecordSpec{}.field(
              "a",
              "A",
              prop::ArraySpec{}.item(
                  prop::RecordSpec{}.field("id",
                                           "ID",
                                           prop::SymbolSpec{"namespace"})))),
          Exception,
          "Array item that declares a namespace must be an entity record.");
      CHECK_THROWS_MSG(
          prop::ArraySpec{}.item(
              prop::RecordSpec{}
                  .field("a",
                         "A",
                         prop::ArraySpec{}.item(prop::RecordSpec{}.field(
                             "id",
                             "ID",
                             prop::SymbolSpec{"namespace_a"})))
                  .field("b",
                         "B",
                         prop::ArraySpec{}.item(prop::RecordSpec{}.field(
                             "id",
                             "ID",
                             prop::SymbolSpec{"namespace_b"})))),
          Exception,
          "Array item that declares a namespace must be an entity record.");
    }
  }
}

TEST_CASE("prop::ArraySpec::validate") {
  SUBCASE("success") {
    SUBCASE("null creates empty array") {
      const auto spec = prop::ArraySpec{}.item(prop::IntSpec{});
      auto tree = prop::Tree{};
      CHECK_FALSE(prop::validate(spec, tree).has_issues());
      CHECK(tree.is_array());
      CHECK(tree.size() == 0);
    }
    SUBCASE("elements validated") {
      const auto spec = prop::ArraySpec{}.item(prop::IntSpec{}.range(0, 9));
      auto tree = prop::Tree{prop::Tree::Array{
          prop::Tree{3},
          prop::Tree{7},
      }};
      CHECK_FALSE(prop::validate(spec, tree).has_issues());
    }
    SUBCASE("exact size accepted") {
      const auto spec = prop::ArraySpec{}.size(2).item(prop::IntSpec{});
      auto tree = prop::Tree{prop::Tree::Array{
          prop::Tree{3},
          prop::Tree{7},
      }};
      CHECK_FALSE(prop::validate(spec, tree).has_issues());
    }
    SUBCASE("null with exact size creates default-sized array") {
      const auto spec =
          prop::ArraySpec{}.size(2).item(prop::RealSpec{}.default_value(0.0));
      auto tree = prop::Tree{};
      CHECK_FALSE(prop::validate(spec, tree).has_issues());
      CHECK_RANGE_EQ(tree.as_array(), {prop::Tree{0.0}, prop::Tree{0.0}});
    }
  }
  SUBCASE("error") {
    SUBCASE("exact size mismatch") {
      const auto spec = prop::ArraySpec{}.size(2).item(prop::IntSpec{});
      auto tree = prop::Tree{prop::Tree::Array{
          prop::Tree{3},
      }};
      CHECK(prop::validate(spec, tree).has_issue("Expected array of size"));
      CHECK_RANGE_EQ(tree.as_array(), {prop::Tree{3}, prop::Tree{}});
    }
    SUBCASE("item out of range") {
      const auto spec = prop::ArraySpec{}.item(prop::IntSpec{}.range(0, 9));
      auto tree = prop::Tree{prop::Tree::Array{
          prop::Tree{99},
      }};
      CHECK(prop::validate(spec, tree).has_issue("above maximum"));
      CHECK_RANGE_EQ(tree.as_array(), {prop::Tree{9}});
    }
    SUBCASE("items of invalid type") {
      const auto spec = prop::ArraySpec{}.item(prop::IntSpec{}.range(0, 9));
      auto tree = prop::Tree{prop::Tree::Array{
          prop::Tree{7},
          prop::Tree{"not-an-int"},
      }};
      CHECK(prop::validate(spec, tree).has_issue("Expected integer"));
      CHECK_RANGE_EQ(tree.as_array(), {prop::Tree{7}, prop::Tree{}});
    }
    SUBCASE("non-array") {
      const auto spec = prop::ArraySpec{}.item(prop::IntSpec{});
      auto tree = prop::Tree{"not-an-array"};
      CHECK(prop::validate(spec, tree).has_issue("Expected array"));
      CHECK(tree.as_array().empty());
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
    SUBCASE("field: entity record arrays") {
      const auto spec =
          prop::RecordSpec{}
              .field("bucket1",
                     "Bucket1",
                     prop::ArraySpec{}.item(prop::RecordSpec{}.field(
                         "id1",
                         "ID1",
                         prop::SymbolSpec{"namespace1"})))
              .field("bucket2",
                     "Bucket2",
                     prop::ArraySpec{}.item(prop::RecordSpec{}.field(
                         "id2",
                         "ID2",
                         prop::SymbolSpec{"namespace2"})));
      CHECK_RANGE_EQ(spec.namespaces(), {"namespace1", "namespace2"});
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
    SUBCASE("field: entity record") {
      CHECK_THROWS_MSG(
          prop::RecordSpec{}.field(
              "field",
              "Field",
              prop::RecordSpec{}.field("id1",
                                       "ID1",
                                       prop::SymbolSpec{"namespace"})),
          tit::Exception,
          "Entity record cannot be nested in another record.");
    }
    SUBCASE("field: multiple symbol fields") {
      CHECK_THROWS_MSG(
          prop::RecordSpec{}
              .field("id1", "ID1", prop::SymbolSpec{"namespace1"})
              .field("id2", "ID2", prop::SymbolSpec{"namespace2"}),
          tit::Exception,
          "Symbol field cannot be placed in a record that declares namespaces");
      CHECK_THROWS_MSG(
          prop::RecordSpec{}
              .field("netsted",
                     "Nested",
                     prop::ArraySpec{}.item(prop::RecordSpec{}.field(
                         "id1",
                         "ID1",
                         prop::SymbolSpec{"namespace1"})))
              .field("id2", "ID2", prop::SymbolSpec{"namespace2"}),
          tit::Exception,
          "Symbol field cannot be placed in a record that declares namespaces");
      CHECK_THROWS_MSG(
          prop::RecordSpec{}
              .field("id1", "ID1", prop::SymbolSpec{"namespace1"})
              .field("netsted",
                     "Nested",
                     prop::ArraySpec{}.item(prop::RecordSpec{}.field(
                         "id2",
                         "ID2",
                         prop::SymbolSpec{"namespace2"}))),
          tit::Exception,
          "Field declaring namespace cannot be placed into entity record");
    }
    SUBCASE("field: duplicate namespace") {
      CHECK_THROWS_MSG(
          prop::RecordSpec{}
              .field("bucket1",
                     "Bucket1",
                     prop::ArraySpec{}.item(prop::RecordSpec{}.field(
                         "id1",
                         "ID1",
                         prop::SymbolSpec{"namespace"})))
              .field("bucket2",
                     "Bucket2",
                     prop::ArraySpec{}.item(prop::RecordSpec{}.field(
                         "id2",
                         "ID2",
                         prop::SymbolSpec{"namespace"}))),
          tit::Exception,
          "already declares namespace");
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
      CHECK_FALSE(prop::validate(spec, tree).has_issues());
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
      CHECK_FALSE(prop::validate(spec, tree).has_issues());
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
          {"bad-key", prop::Tree{2}},
          {"unknown", prop::Tree{1}},
      }};
      CHECK(prop::validate(spec, tree).has_issue("Unknown field"));
      REQUIRE(tree.is_map());
      CHECK(tree.get("known").as_bool() == false);
      CHECK_FALSE(tree.has("bad-key"));
      CHECK_FALSE(tree.has("unknown"));
    }
    SUBCASE("missing field") {
      const auto spec =
          prop::RecordSpec{}.field("field", "Field", prop::StringSpec{});
      auto tree = prop::Tree{prop::Tree::Map{}};
      CHECK(prop::validate(spec, tree).has_issue("is missing"));
      REQUIRE(tree.has("field"));
      CHECK(tree.get("field").is_null());
    }
    SUBCASE("non-map") {
      auto tree = prop::Tree{true};
      CHECK(prop::validate(prop::RecordSpec{}, tree)
                .has_issue("Expected object"));
      CHECK(tree.as_map().empty());
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
    SUBCASE("option: declares namespace") {
      CHECK_THROWS_MSG(
          prop::VariantSpec{}.option("a", "A", prop::SymbolSpec{"namespace"}),
          tit::Exception,
          "Variant option cannot declare a namespace");
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
      CHECK_FALSE(prop::validate(spec, tree).has_issues());
    }
    SUBCASE("null with default fills in") {
      const auto spec =
          prop::VariantSpec{}
              .option("on", "On", prop::BoolSpec{}.default_value(true))
              .option("off", "Off", prop::BoolSpec{}.default_value(false))
              .default_value("on");
      auto tree = prop::Tree{};
      CHECK_FALSE(prop::validate(spec, tree).has_issues());
      CHECK(tree.is_map());
      CHECK(tree.get("on").as_bool() == true);
      CHECK(tree.get("_active").as_string() == "on");
    }
    SUBCASE("single present option infers active option") {
      const auto spec = prop::VariantSpec{}
                            .option("fast", "Fast", prop::BoolSpec{})
                            .option("slow", "Slow", prop::IntSpec{});
      auto tree = prop::Tree{prop::Tree::Map{
          {"slow", prop::Tree{42}},
      }};
      CHECK_FALSE(prop::validate(spec, tree).has_issues());
      CHECK(tree.get("_active").as_string() == "slow");
      CHECK(tree.get("slow").as_int() == 42);
    }
    SUBCASE("inactive option materializes missing fields without issues") {
      const auto spec =
          prop::VariantSpec{}
              .option("plain", "Plain", prop::RecordSpec{})
              .option(
                  "advanced",
                  "Advanced",
                  prop::RecordSpec{}.field("level", "Level", prop::IntSpec{}));
      auto tree = prop::Tree{prop::Tree::Map{
          {"_active", prop::Tree{"plain"}},
          {"plain", prop::Tree{prop::Tree::Map{}}},
          {"advanced", prop::Tree{prop::Tree::Map{}}},
      }};
      CHECK_FALSE(prop::validate(spec, tree).has_issues());
      CHECK(tree.get("advanced").get("level").is_null());
    }
  }
  SUBCASE("error") {
    SUBCASE("unknown key") {
      const auto spec = prop::VariantSpec{}
                            .option("a", "A", prop::BoolSpec{})
                            .option("b", "B", prop::IntSpec{});
      auto tree = prop::Tree{prop::Tree::Map{
          {"_active", prop::Tree{"c"}},
          {"bad-key", prop::Tree{false}},
          {"c", prop::Tree{true}},
      }};
      CHECK(prop::validate(spec, tree).has_issue("Unknown option 'c'"));
      REQUIRE(tree.is_map());
      CHECK(tree.get("_active").is_null());
      CHECK_FALSE(tree.has("bad-key"));
      CHECK_FALSE(tree.has("c"));
    }
    SUBCASE("null without default") {
      const auto spec = prop::VariantSpec{}
                            .option("a", "A", prop::BoolSpec{})
                            .option("b", "B", prop::IntSpec{});
      auto tree = prop::Tree{};
      CHECK(prop::validate(spec, tree).has_issue("has no active option"));
      REQUIRE(tree.is_map());
      CHECK(tree.get("_active").is_null());
    }
    SUBCASE("missing _active value") {
      const auto spec = prop::VariantSpec{}.option("a", "A", prop::BoolSpec{});
      auto tree = prop::Tree{prop::Tree::Map{}};
      CHECK(prop::validate(spec, tree).has_issue("has no active option"));
      REQUIRE(tree.is_map());
      CHECK(tree.get("_active").is_null());
    }
    SUBCASE("multiple options cannot infer active option") {
      const auto spec = prop::VariantSpec{}
                            .option("a", "A", prop::BoolSpec{})
                            .option("b", "B", prop::IntSpec{});
      auto tree = prop::Tree{prop::Tree::Map{
          {"a", prop::Tree{true}},
          {"b", prop::Tree{42}},
      }};
      CHECK(prop::validate(spec, tree).has_issue("multiple options"));
      REQUIRE(tree.is_map());
      CHECK(tree.get("_active").is_null());
      CHECK(tree.get("a").as_bool() == true);
      CHECK(tree.get("b").as_int() == 42);
    }
    SUBCASE("bad _active type") {
      const auto spec = prop::VariantSpec{}.option("a", "A", prop::BoolSpec{});
      auto tree = prop::Tree{prop::Tree::Map{
          {"_active", prop::Tree{42}},
      }};
      CHECK(prop::validate(spec, tree).has_issue("'_active' must be a string"));
      REQUIRE(tree.is_map());
      CHECK(tree.get("_active").is_null());
    }
    SUBCASE("invalid _active value") {
      const auto spec = prop::VariantSpec{}.option("a", "A", prop::BoolSpec{});
      auto tree = prop::Tree{prop::Tree::Map{
          {"_active", prop::Tree{"z"}},
      }};
      CHECK(prop::validate(spec, tree).has_issue("not valid"));
      REQUIRE(tree.is_map());
      CHECK(tree.get("_active").is_null());
    }
    SUBCASE("non-map") {
      const auto spec = prop::VariantSpec{}.option("a", "A", prop::BoolSpec{});
      auto tree = prop::Tree{true};
      CHECK(prop::validate(spec, tree).has_issue("Expected object"));
      REQUIRE(tree.is_map());
      CHECK(tree.get("_active").is_null());
    }
    SUBCASE("inactive option still reports type issues") {
      const auto spec =
          prop::VariantSpec{}
              .option("plain", "Plain", prop::RecordSpec{})
              .option(
                  "advanced",
                  "Advanced",
                  prop::RecordSpec{}.field("level", "Level", prop::IntSpec{}));
      auto tree = prop::Tree{prop::Tree::Map{
          {"_active", prop::Tree{"plain"}},
          {"plain", prop::Tree{prop::Tree::Map{}}},
          {"advanced",
           prop::Tree{prop::Tree::Map{{"level", prop::Tree{"bad"}}}}},
      }};
      CHECK(prop::validate(spec, tree).has_issue("Expected integer"));
      CHECK(tree.get("advanced").get("level").is_null());
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("prop::SymbolSpec") {
  SUBCASE("success") {
    const prop::SymbolSpec spec{"materials"};
    CHECK(spec.type() == prop::SpecType::Symbol);
    CHECK_RANGE_EQ(spec.namespaces(), {"materials"});
  }
  SUBCASE("error") {
    CHECK_THROWS_MSG(prop::SymbolSpec{"bad namespace"},
                     tit::Exception,
                     "valid identifier");
  }
}

TEST_CASE("prop::SymbolSpec::validate") {
  const auto materials_spec = [] {
    return prop::ArraySpec{}.item(
        prop::RecordSpec{}.field("id", "ID", prop::SymbolSpec{"materials"}));
  };
  SUBCASE("success") {
    auto tree = prop::Tree{prop::Tree::Array{
        prop::Tree{prop::Tree::Map{{"id", prop::Tree{"steel"}}}},
    }};
    CHECK_FALSE(prop::validate(materials_spec(), tree).has_issues());
  }
  SUBCASE("error") {
    SUBCASE("invalid symbol") {
      const prop::SymbolSpec spec{"materials"};
      auto tree = prop::Tree{"bad symbol"};
      CHECK(prop::validate(spec, tree).has_issue("valid identifier"));
      CHECK(tree.is_null());
    }
    SUBCASE("duplicate symbol") {
      auto tree = prop::Tree{prop::Tree::Array{
          prop::Tree{prop::Tree::Map{{"id", prop::Tree{"steel"}}}},
          prop::Tree{prop::Tree::Map{{"id", prop::Tree{"steel"}}}},
      }};
      CHECK(
          prop::validate(materials_spec(), tree).has_issue("Duplicate symbol"));
      REQUIRE(tree.is_array());
      CHECK(tree.get(0).get("id").as_string() == "steel");
      CHECK(tree.get(1).get("id").is_null());
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("prop::RefSpec") {
  SUBCASE("success") {
    const prop::RefSpec spec{"materials"};
    CHECK(spec.type() == prop::SpecType::Ref);
    CHECK(spec.target_namespace() == "materials");
  }
  SUBCASE("error") {
    CHECK_THROWS_MSG(prop::RefSpec{"bad namespace"},
                     tit::Exception,
                     "valid identifier");
  }
}

TEST_CASE("prop::RefSpec::validate") {
  const auto materials_spec = [] {
    return prop::ArraySpec{}.item(
        prop::RecordSpec{}.field("id", "ID", prop::SymbolSpec{"materials"}));
  };
  const auto shape_spec = [] {
    return prop::RecordSpec{}.field("material",
                                    "Material",
                                    prop::RefSpec{"materials"});
  };
  SUBCASE("success") {
    SUBCASE("reference") {
      const auto spec = prop::RecordSpec{}
                            .field("materials", "Materials", materials_spec())
                            .field("shape", "Shape", shape_spec());
      auto tree = prop::Tree{prop::Tree::Map{
          {"materials",
           prop::Tree{prop::Tree::Array{
               prop::Tree{prop::Tree::Map{{"id", prop::Tree{"steel"}}}},
           }}},
          {"shape",
           prop::Tree{prop::Tree::Map{{"material", prop::Tree{"steel"}}}}},
      }};
      CHECK_FALSE(prop::validate(spec, tree).has_issues());
    }
    SUBCASE("forward reference") {
      const auto spec = prop::RecordSpec{}
                            .field("shape", "Shape", shape_spec())
                            .field("materials", "Materials", materials_spec());
      auto tree = prop::Tree{prop::Tree::Map{
          {"shape",
           prop::Tree{prop::Tree::Map{{"material", prop::Tree{"steel"}}}}},
          {"materials",
           prop::Tree{prop::Tree::Array{
               prop::Tree{prop::Tree::Map{{"id", prop::Tree{"steel"}}}},
           }}},
      }};
      CHECK_FALSE(prop::validate(spec, tree).has_issues());
    }
    SUBCASE("inactive variant reference") {
      const auto spec =
          prop::RecordSpec{}
              .field("materials", "Materials", materials_spec())
              .field("choice",
                     "Choice",
                     prop::VariantSpec{}
                         .option("plain", "Plain", prop::RecordSpec{})
                         .option("material",
                                 "Material",
                                 prop::RefSpec{"materials"}));
      auto tree = prop::Tree{prop::Tree::Map{
          {"choice",
           prop::Tree{prop::Tree::Map{
               {"_active", prop::Tree{"plain"}},
               {"plain", prop::Tree{prop::Tree::Map{}}},
               {"material", prop::Tree{"missing"}},
           }}},
          {"materials", prop::Tree{prop::Tree::Array{}}},
      }};
      CHECK_FALSE(prop::validate(spec, tree).has_issues());
    }
  }
  SUBCASE("error") {
    SUBCASE("invalid symbol") {
      const prop::RefSpec spec{"materials"};
      auto tree = prop::Tree{"bad ref"};
      CHECK(prop::validate(spec, tree).has_issue("valid identifier"));
      CHECK(tree.is_null());
    }
    SUBCASE("missing symbol") {
      const auto spec = prop::RecordSpec{}
                            .field("shape", "Shape", shape_spec())
                            .field("materials", "Materials", materials_spec());
      auto tree = prop::Tree{prop::Tree::Map{
          {"shape",
           prop::Tree{prop::Tree::Map{{"material", prop::Tree{"wood"}}}}},
          {"materials",
           prop::Tree{prop::Tree::Array{
               prop::Tree{prop::Tree::Map{{"id", prop::Tree{"steel"}}}},
           }}},
      }};
      CHECK(prop::validate(spec, tree).has_issue("not found in namespace"));
      CHECK(tree.get("shape").get("material").as_string() == "wood");
    }
    SUBCASE("missing symbol in empty namespace") {
      const auto spec = prop::RecordSpec{}
                            .field("shape", "Shape", shape_spec())
                            .field("materials", "Materials", materials_spec());
      auto tree = prop::Tree{prop::Tree::Map{
          {"shape",
           prop::Tree{prop::Tree::Map{{"material", prop::Tree{"steel"}}}}},
          {"materials", prop::Tree{prop::Tree::Array{}}},
      }};
      CHECK(prop::validate(spec, tree).has_issue("not found in namespace"));
      CHECK(tree.get("shape").get("material").as_string() == "steel");
    }
    SUBCASE("missing symbol in absent namespace") {
      const auto spec = prop::RecordSpec{}.field(
          "shape",
          "Shape",
          prop::RecordSpec{}.field("material",
                                   "Material",
                                   prop::RefSpec{"materials"}));
      auto tree = prop::Tree{prop::Tree::Map{
          {"shape",
           prop::Tree{prop::Tree::Map{{"material", prop::Tree{"steel"}}}}},
      }};
      CHECK(prop::validate(spec, tree).has_issue("not found in namespace"));
      CHECK(tree.get("shape").get("material").as_string() == "steel");
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
    const auto spec = prop::RealSpec{}.range(1.0, 10.0).default_value(5.0).unit(
        prop::Unit{"m/s"});
    CHECK(prop::spec_dump_json(spec) == R"({
  "type": "real",
  "min": 1.0,
  "max": 10.0,
  "default": 5.0,
  "unit": "m/s"
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
    const auto spec = prop::ArraySpec{}.size(3).item(prop::IntSpec{});
    CHECK(prop::spec_dump_json(spec) == R"({
  "type": "array",
  "size": 3,
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
        "type": "bool",
        "default": false
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
        "type": "bool",
        "default": false
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
  SUBCASE("symbol") {
    const auto spec = prop::SymbolSpec{"materials"};
    CHECK(prop::spec_dump_json(spec) == R"({
  "type": "symbol",
  "namespace": "materials"
})");
  }
  SUBCASE("ref") {
    const auto spec = prop::RefSpec{"materials"};
    CHECK(prop::spec_dump_json(spec) == R"({
  "type": "ref",
  "namespace": "materials"
})");
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
