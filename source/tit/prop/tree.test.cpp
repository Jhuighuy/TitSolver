/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <ranges>
#include <set>
#include <string_view>
#include <utility>

#include "tit/core/exception.hpp"
#include "tit/prop/tree.hpp"
#include "tit/testing/test.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("prop::Tree") {
  SUBCASE("success") {
    SUBCASE("null") {
      const prop::Tree tree{};
      CHECK(tree.is_null());
      CHECK_FALSE(tree.is_bool());
      CHECK_FALSE(tree.is_int());
      CHECK_FALSE(tree.is_real());
      CHECK_FALSE(tree.is_string());
      CHECK_FALSE(tree.is_array());
      CHECK_FALSE(tree.is_map());
      CHECK(tree.type_name() == "null");
    }
    SUBCASE("bool") {
      const prop::Tree tree{true};
      CHECK_FALSE(tree.is_null());
      CHECK(tree.is_bool());
      CHECK(tree.as_bool() == true);
      CHECK(tree.type_name() == "boolean");
    }
    SUBCASE("int") {
      const prop::Tree tree{42};
      CHECK_FALSE(tree.is_null());
      CHECK(tree.is_int());
      CHECK(tree.as_int() == 42);
      CHECK(tree.type_name() == "integer");
    }
    SUBCASE("real") {
      const prop::Tree tree{3.14};
      CHECK_FALSE(tree.is_null());
      CHECK(tree.is_real());
      CHECK(tree.as_real() == 3.14);
      CHECK(tree.type_name() == "number");
    }
    SUBCASE("string") {
      const prop::Tree tree{"hello"};
      CHECK_FALSE(tree.is_null());
      CHECK(tree.is_string());
      CHECK(tree.as_string() == "hello");
      CHECK(tree.type_name() == "string");
    }
    SUBCASE("array") {
      const prop::Tree tree{prop::Tree::Array{
          prop::Tree{1},
          prop::Tree{2},
      }};
      CHECK(tree.is_array());
      CHECK(tree.type_name() == "array");
      CHECK(tree.size() == 2);
    }
    SUBCASE("map") {
      const prop::Tree tree{prop::Tree::Map{
          {"a", prop::Tree{1}},
          {"b", prop::Tree{2}},
      }};
      CHECK(tree.is_map());
      CHECK(tree.type_name() == "map");
      CHECK(tree.size() == 2);
    }
  }
  SUBCASE("error") {
    SUBCASE("bool: wrong type accessors") {
      const prop::Tree tree{true};
      CHECK_THROWS_MSG(tree.as_int(), tit::Exception, "not an integer");
      CHECK_THROWS_MSG(tree.as_real(), tit::Exception, "not a number");
      CHECK_THROWS_MSG(tree.as_string(), tit::Exception, "not a string");
    }
    SUBCASE("int: wrong type accessors") {
      const prop::Tree tree{1};
      CHECK_THROWS_MSG(tree.as_bool(), tit::Exception, "not a boolean");
      CHECK_THROWS_MSG(tree.as_real(), tit::Exception, "not a number");
      CHECK_THROWS_MSG(tree.as_string(), tit::Exception, "not a string");
    }
    SUBCASE("real: wrong type accessors") {
      const prop::Tree tree{1.0};
      CHECK_THROWS_MSG(tree.as_bool(), tit::Exception, "not a boolean");
      CHECK_THROWS_MSG(tree.as_int(), tit::Exception, "not an integer");
      CHECK_THROWS_MSG(tree.as_string(), tit::Exception, "not a string");
    }
    SUBCASE("string: wrong type accessors") {
      const prop::Tree tree{"xxx"};
      CHECK_THROWS_MSG(tree.as_bool(), tit::Exception, "not a boolean");
      CHECK_THROWS_MSG(tree.as_int(), tit::Exception, "not an integer");
      CHECK_THROWS_MSG(tree.as_real(), tit::Exception, "not a number");
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("prop::Tree::as_array") {
  SUBCASE("success") {
    SUBCASE("const") {
      const prop::Tree tree{prop::Tree::Array{
          prop::Tree{1},
          prop::Tree{2},
      }};
      CHECK(tree.as_array().size() == 2);
    }
    SUBCASE("mutable") {
      prop::Tree tree{prop::Tree::Array{
          prop::Tree{1},
          prop::Tree{2},
      }};
      CHECK(tree.as_array().size() == 2);
    }
  }
  SUBCASE("error") {
    SUBCASE("const") {
      const prop::Tree tree{true};
      CHECK_THROWS_MSG(tree.as_array(), tit::Exception, "not an array");
    }
    SUBCASE("mutable") {
      prop::Tree tree{true};
      CHECK_THROWS_MSG(tree.as_array(), tit::Exception, "not an array");
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("prop::Tree::as_map") {
  SUBCASE("success") {
    SUBCASE("const") {
      const prop::Tree tree{prop::Tree::Map{
          {"a", prop::Tree{1}},
          {"b", prop::Tree{2}},
      }};
      CHECK(tree.as_map().size() == 2);
    }
    SUBCASE("mutable") {
      prop::Tree tree{prop::Tree::Map{
          {"a", prop::Tree{1}},
          {"b", prop::Tree{2}},
      }};
      CHECK(tree.as_map().size() == 2);
    }
  }
  SUBCASE("error") {
    SUBCASE("const") {
      const prop::Tree tree{true};
      CHECK_THROWS_MSG(tree.as_map(), tit::Exception, "not a map");
    }
    SUBCASE("mutable") {
      prop::Tree tree{true};
      CHECK_THROWS_MSG(tree.as_map(), tit::Exception, "not a map");
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("prop::Tree::size") {
  SUBCASE("success") {
    SUBCASE("array size") {
      const prop::Tree tree{prop::Tree::Array{
          prop::Tree{1},
          prop::Tree{2},
      }};
      CHECK(tree.size() == 2);
    }
    SUBCASE("map size") {
      const prop::Tree tree{prop::Tree::Map{
          {"a", prop::Tree{1}},
          {"b", prop::Tree{2}},
      }};
      CHECK(tree.size() == 2);
    }
  }
  SUBCASE("error") {
    const prop::Tree tree{true};
    CHECK_THROWS_MSG(tree.size(), tit::Exception, "not an array or map");
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("prop::Tree::get(index)") {
  SUBCASE("success") {
    SUBCASE("const") {
      const prop::Tree tree{prop::Tree::Array{
          prop::Tree{10},
          prop::Tree{20},
      }};
      CHECK(tree.get(0).as_int() == 10);
      CHECK(tree.get(1).as_int() == 20);
    }
    SUBCASE("mutable") {
      prop::Tree tree{prop::Tree::Array{
          prop::Tree{10},
          prop::Tree{20},
      }};
      CHECK(tree.get(0).as_int() == 10);
      CHECK(tree.get(1).as_int() == 20);
    }
  }
  SUBCASE("error") {
    SUBCASE("const") {
      const prop::Tree tree{prop::Tree::Array{
          prop::Tree{10},
          prop::Tree{20},
      }};
      CHECK_THROWS_MSG(tree.get(5), tit::Exception, "out of range");
    }
    SUBCASE("mutable") {
      prop::Tree tree{prop::Tree::Array{
          prop::Tree{10},
          prop::Tree{20},
      }};
      CHECK_THROWS_MSG(tree.get(5), tit::Exception, "out of range");
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("prop::Tree::get(key)") {
  SUBCASE("success") {
    SUBCASE("const") {
      const prop::Tree tree{prop::Tree::Map{
          {"a", prop::Tree{1}},
          {"b", prop::Tree{2}},
      }};
      CHECK(tree.has("b"));
      CHECK(tree.get("b").as_int() == 2);
      CHECK_FALSE(tree.has("c"));
    }
    SUBCASE("mutable") {
      prop::Tree tree{prop::Tree::Map{
          {"a", prop::Tree{1}},
          {"b", prop::Tree{2}},
      }};
      CHECK(tree.has("b"));
      CHECK(tree.get("b").as_int() == 2);
      auto& child = tree.get("new_key");
      CHECK(child.is_null());
      CHECK(tree.has("new_key"));
    }
  }
  SUBCASE("error") {
    SUBCASE("const") {
      const prop::Tree tree{prop::Tree::Map{}};
      CHECK_THROWS_MSG(tree.get("absent"), tit::Exception, "not found");
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("prop::Tree::keys") {
  SUBCASE("success") {
    const prop::Tree tree{prop::Tree::Map{
        {"a", prop::Tree{1}},
        {"b", prop::Tree{2}},
    }};
    CHECK_RANGE_EQ(tree.keys() | std::ranges::to<std::set>(), {"a", "b"});
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("prop::Tree::set") {
  SUBCASE("set bool") {
    prop::Tree tree{};
    tree.set(true);
    CHECK(tree.as_bool() == true);
  }
  SUBCASE("set int") {
    prop::Tree tree{};
    tree.set(99);
    CHECK(tree.as_int() == 99);
  }
  SUBCASE("set real") {
    prop::Tree tree{};
    tree.set(2.71);
    CHECK(tree.as_real() == 2.71);
  }
  SUBCASE("set string") {
    prop::Tree tree{};
    tree.set("hi");
    CHECK(tree.as_string() == "hi");
  }
  SUBCASE("set array") {
    prop::Tree tree{};
    tree.set(prop::Tree::Array{
        prop::Tree{1},
        prop::Tree{2},
    });
    CHECK(tree.is_array());
    CHECK(tree.size() == 2);
  }
  SUBCASE("set map") {
    prop::Tree tree{};
    tree.set(prop::Tree::Map{
        {"a", prop::Tree{1}},
        {"b", prop::Tree{2}},
    });
    CHECK(tree.is_map());
    CHECK(tree.size() == 2);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("prop::Tree::merge") {
  SUBCASE("merging null into value is a noop") {
    prop::Tree tree{1};
    tree.merge(prop::Tree{});
    CHECK(tree.as_int() == 1);
  }
  SUBCASE("merging into null replaces") {
    prop::Tree tree{};
    tree.merge(prop::Tree{42});
    CHECK(tree.as_int() == 42);
  }
  SUBCASE("two maps merged recursively") {
    prop::Tree tree_1{prop::Tree::Map{
        {"a", prop::Tree{1}},
        {"b", prop::Tree{2}},
    }};
    prop::Tree tree_2{prop::Tree::Map{
        {"b", prop::Tree{99}},
        {"c", prop::Tree{3}},
    }};
    tree_1.merge(std::move(tree_2));
    CHECK(tree_1.get("a").as_int() == 1);
    CHECK(tree_1.get("b").as_int() == 99);
    CHECK(tree_1.get("c").as_int() == 3);
  }
  SUBCASE("merging non-map into non-map replaces") {
    prop::Tree tree{"old"};
    tree.merge(prop::Tree{"new"});
    CHECK(tree.as_string() == "new");
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Note: tree JSON format should be very stable, so I'm hard-coding the expected
//       output here.
TEST_CASE("prop::tree_dump_json") {
  SUBCASE("null") {
    CHECK(prop::tree_dump_json(prop::Tree{}) == "null");
  }
  SUBCASE("bool") {
    CHECK(prop::tree_dump_json(prop::Tree{true}) == "true");
  }
  SUBCASE("int") {
    CHECK(prop::tree_dump_json(prop::Tree{7}) == "7");
  }
  SUBCASE("real") {
    CHECK(prop::tree_dump_json(prop::Tree{1.5}) == "1.5");
  }
  SUBCASE("string") {
    CHECK(prop::tree_dump_json(prop::Tree{"hello"}) == R"("hello")");
  }
  SUBCASE("array") {
    const prop::Tree tree{prop::Tree::Array{
        prop::Tree{1},
        prop::Tree{2},
    }};
    CHECK(prop::tree_dump_json(tree) == R"([
  1,
  2
])");
  }
  SUBCASE("map") {
    const prop::Tree tree{prop::Tree::Map{
        {"a", prop::Tree{1}},
        {"b", prop::Tree{2}},
    }};
    CHECK(prop::tree_dump_json(tree) == R"({
  "a": 1,
  "b": 2
})");
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("prop::tree_from_json") {
  SUBCASE("success") {
    const auto tree = prop::tree_from_json(R"({
  "alpha": true,
  "count": 5,
  "ratio": 1.5,
  "name": "test",
  "items": [1, 2, 3],
  "outer": { "inner": 42 }
}
)");
    CHECK(tree.is_map());
    CHECK(tree.get("alpha").as_bool() == true);
    CHECK(tree.get("count").as_int() == 5);
    CHECK(tree.get("ratio").as_real() == 1.5);
    CHECK(tree.get("name").as_string() == "test");
    CHECK(tree.get("items").is_array());
    CHECK(tree.get("items").size() == 3);
    CHECK(tree.get("outer").get("inner").as_int() == 42);
  }
  SUBCASE("error") {
    SUBCASE("invalid JSON") {
      CHECK_THROWS_MSG(prop::tree_from_json(R"({
  bad json
)"),
                       tit::Exception,
                       "Failed to parse JSON");
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
