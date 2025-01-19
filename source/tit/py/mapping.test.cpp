/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "tit/core/str_utils.hpp"

#include "tit/py/error.hpp"
#include "tit/py/mapping.hpp"
#include "tit/py/number.hpp"
#include "tit/py/object.hpp"
#include "tit/py/sequence.hpp"

#include "tit/py/interpreter.testing.hpp"
#include "tit/testing/test.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("py::Mapping") {
  SUBCASE("typing") {
    CHECK(py::Mapping::isinstance(py::Dict{}));
    CHECK(py::Mapping::isinstance(py::List{})); // Yes, list is a mapping.
    CHECK_FALSE(py::Mapping::isinstance(py::Int{}));
  }
  // Other methods are tested in "Dict" tests below.
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("py::Dict") {
  SUBCASE("typing") {
    CHECK(py::Dict::type().fully_qualified_name() == "dict");
    CHECK(py::Dict::isinstance(py::Dict{}));
    CHECK_FALSE(py::Dict::isinstance(py::List{}));
  }
  SUBCASE("construction") {
    SUBCASE("empty") {
      CHECK(!py::Dict{});
      CHECK(py::len(py::Dict{}) == 0);
    }
    SUBCASE("from iterable") {
      const auto iterable = py::make_list(py::make_tuple("a", 1),
                                          py::make_tuple("b", 2),
                                          py::make_tuple("c", 3));
      const py::Dict dict{iterable};
      CHECK_FALSE(dict.is(iterable));
      CHECK(py::len(dict) == 3);
      CHECK(dict.has_key("a"));
      CHECK(dict.has_key("b"));
      CHECK(dict.has_key("c"));
      CHECK(dict["a"] == py::Int{1});
      CHECK(dict["b"] == py::Int{2});
      CHECK(dict["c"] == py::Int{3});
    }
    SUBCASE("from mapping") {
      REQUIRE(testing::interpreter().exec(R"PY(
        # Minimal mapping class.
        class MyMapping:
          def keys(self):
            return ["a", "b", "c"]

          def __getitem__(self, key):
            return self.keys().index(key) + 1
      )PY"));
      const py::Object MyMapping =
          testing::interpreter().globals()["MyMapping"];
      const auto mapping = MyMapping();
      const py::Dict dict(mapping);
      CHECK_FALSE(dict.is(mapping));
      CHECK(py::len(dict) == 3);
      CHECK(dict.has_key("a"));
      CHECK(dict.has_key("b"));
      CHECK(dict.has_key("c"));
      CHECK(dict["a"] == py::Int{1});
      CHECK(dict["b"] == py::Int{2});
      CHECK(dict["c"] == py::Int{3});
    }
    SUBCASE("invalid") {
      CHECK_THROWS_MSG(py::repr(py::Dict{py::None()}),
                       py::ErrorException,
                       "TypeError: 'NoneType' object is not iterable");
    }
  }
  SUBCASE("methods") {
    const auto items =
        py::make_list(py::make_tuple("a", 1), py::make_tuple("b", 2));
    const py::Dict dict{items};
    SUBCASE("access") {
      CHECK(dict.keys() == py::make_list("a", "b"));
      CHECK(dict.values() == py::make_list(1, 2));
      CHECK(dict.items() == items);
    }
    SUBCASE("update") {
      dict.update(
          py::make_list(py::make_tuple("b", 3), py::make_tuple("c", 4)));
      CHECK(dict.items() == py::make_list(py::make_tuple("a", 1),
                                          py::make_tuple("b", 3),
                                          py::make_tuple("c", 4)));
    }
    SUBCASE("clear") {
      dict.clear();
      CHECK(!dict);
    }
    SUBCASE("for_each") {
      dict.for_each([](const py::Object& key, const py::Object& value) {
        if (key == py::Str{"a"}) CHECK(value == py::Int{1});
        else if (key == py::Str{"b"}) CHECK(value == py::Int{2});
        else CHECK_MESSAGE(false, "Unexpected key");
      });
    }
  }
  SUBCASE("operators") {
    SUBCASE("operator[]") {
      const py::Dict dict{
          py::make_list(py::make_tuple("a", 1), py::make_tuple("b", 2))};
      const auto run_subcase = [&dict]<class Key> {
        SUBCASE("has_key") {
          CHECK(dict.has_key(Key{"a"}));
          CHECK(dict.has_key(Key{"b"}));
          CHECK_FALSE(dict.has_key(Key{"c"}));
        }
        SUBCASE("at") {
          CHECK(dict[Key{"a"}] == py::Int{1});
          CHECK(dict[Key{"b"}] == py::Int{2});
          REQUIRE_FALSE(dict.has_key(Key{"does_not_exist"}));
          CHECK_THROWS_MSG(py::repr(dict[Key{"does_not_exist"}]),
                           py::ErrorException,
                           "KeyError: 'does_not_exist'");
        }
        SUBCASE("set_at") {
          dict[Key{"c"}] = 3;
          CHECK(dict[Key{"c"}] == py::Int{3});
        }
        SUBCASE("del") {
          dict.del(Key{"a"});
          CHECK_FALSE(dict.has_key(Key{"a"}));
          REQUIRE_FALSE(dict.has_key(Key{"does_not_exist"}));
          CHECK_THROWS_MSG(dict.del(Key{"does_not_exist"}),
                           py::ErrorException,
                           "KeyError: 'does_not_exist'");
        }
      };
      SUBCASE("string keys") {
        run_subcase.template operator()<CStrView>();
      }
      SUBCASE("object keys") {
        run_subcase.template operator()<py::Str>();
      }
    }
    SUBCASE("operator|=") {
      SUBCASE("normal") {
        py::Dict dict{
            py::make_list(py::make_tuple("a", 1), py::make_tuple("b", 2))};
        dict |= py::make_list(py::make_tuple("b", 3), py::make_tuple("c", 4));
        CHECK(py::len(dict) == 3);
        CHECK(dict.has_key("a"));
        CHECK(dict.has_key("b"));
        CHECK(dict.has_key("c"));
        CHECK(dict["a"] == py::Int{1});
        CHECK(dict["b"] == py::Int{3});
        CHECK(dict["c"] == py::Int{4});
      }
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("py::Set") {
  SUBCASE("typing") {
    CHECK(py::Set::type().fully_qualified_name() == "set");
    CHECK(py::Set::isinstance(py::Set{}));
    CHECK_FALSE(py::Set::isinstance(py::Dict{}));
  }
  SUBCASE("construction") {
    SUBCASE("empty") {
      CHECK(!py::Set{});
      CHECK(py::len(py::Set{}) == 0);
    }
    SUBCASE("from items") {
      const auto set = py::make_set(1, 2, 3);
      CHECK(py::len(set) == 3);
      CHECK(set.has(1));
      CHECK(set.has(2));
      CHECK(set.has(3));
    }
    SUBCASE("from iterable") {
      const auto iterable = py::make_tuple(1, 2, 3);
      const py::Set set{iterable};
      CHECK_FALSE(set.is(iterable));
      CHECK(set == py::make_set(1, 2, 3));
    }
    SUBCASE("invalid") {
      CHECK_THROWS_MSG(py::repr(py::Set{py::None()}),
                       py::ErrorException,
                       "TypeError: 'NoneType' object is not iterable");
    }
  }
  SUBCASE("methods") {
    const auto set = py::make_set(1, 2, 3);
    SUBCASE("add") {
      set.add(3);
      set.add(4);
      CHECK(set == py::make_set(1, 2, 3, 4));
    }
    SUBCASE("discard") {
      set.discard(2);
      CHECK(set == py::make_set(1, 3));
    }
    SUBCASE("pop") {
      const auto item = set.pop();
      CHECK((py::Int{1} <= item && item <= py::Int{3}));
      set.clear();
      CHECK_THROWS_MSG(set.pop(),
                       py::ErrorException,
                       "KeyError: 'pop from an empty set'");
    }
    SUBCASE("clear") {
      set.clear();
      CHECK(!set);
    }
  }
  SUBCASE("operators") {
    SUBCASE("operator&") {
      SUBCASE("normal") {
        CHECK((py::make_set(1, 2, 3) & py::make_set(2, 3, 4)) ==
              py::make_set(2, 3));
      }
      SUBCASE("augmented") {
        auto set = py::make_set(1, 2, 3);
        set &= py::make_set(2, 3, 4);
        CHECK(set == py::make_set(2, 3));
      }
    }
    SUBCASE("operator|") {
      SUBCASE("normal") {
        CHECK((py::make_set(1, 2, 3) | py::make_set(2, 3, 4)) ==
              py::make_set(1, 2, 3, 4));
      }
      SUBCASE("augmented") {
        auto set = py::make_set(1, 2, 3);
        set |= py::make_set(2, 3, 4);
        CHECK(set == py::make_set(1, 2, 3, 4));
      }
    }
    SUBCASE("operator^") {
      SUBCASE("normal") {
        CHECK((py::make_set(1, 2, 3) ^ py::make_set(2, 3, 4)) ==
              py::make_set(1, 4));
      }
      SUBCASE("augmented") {
        auto set = py::make_set(1, 2, 3);
        set ^= py::make_set(2, 3, 4);
        CHECK(set == py::make_set(1, 4));
      }
    }
    SUBCASE("operator-") {
      SUBCASE("normal") {
        CHECK((py::make_set(1, 2, 3) - py::make_set(2, 3, 4)) ==
              py::make_set(1));
      }
      SUBCASE("augmented") {
        auto set = py::make_set(1, 2, 3);
        set -= py::make_set(2, 3, 4);
        CHECK(set == py::make_set(1));
      }
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
