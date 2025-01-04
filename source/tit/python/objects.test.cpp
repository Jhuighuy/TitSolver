/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <string>
#include <utility>

#include "tit/core/sys/utils.hpp"

#include "tit/python/casts.hpp"
#include "tit/python/interpreter.hpp"
#include "tit/python/objects.hpp"

#include "tit/testing/test.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto interpreter = [] { // NOLINT(cert-err58-cpp)
  py::embed::Config config;
  config.set_home(std::string{get_env("INSTALL_DIR").value()} + "/python");
  return py::embed::Interpreter{std::move(config)};
}();

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("Iterator") {
  const auto iterable = py::make_list(1, 2, 3);
  const auto iter = py::iter(iterable);
  CHECK(iter.next() == py::int_(1));
  CHECK(iter.next() == py::int_(2));
  CHECK(iter.next() == py::int_(3));
  CHECK_FALSE(iter.next());
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("Sequence") {
  SUBCASE("construction") {
    SUBCASE("from non-sequence") {
      // CHECK_THROWS_MSG(py::Sequence{py::None()},
      //                  Exception,
      //                  "TypeError: 'NoneType' object is not a sequence");
    }
  }
  // Other methods are tested in "str", "tuple" and "list" test below.
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("str") {
  SUBCASE("construction") {
    SUBCASE("empty") {
      const auto str = py::str();
      CHECK(len(str) == 0);
      CHECK(str.val().empty());
    }
    SUBCASE("from string") {
      const auto str_1 = py::str("abc");
      const auto str_2 = py::str(str_1);
      const auto str_3 = py::str(str_1, py::keep);
      CHECK(str_2.is(str_1)); // Since strings are immutable.
      CHECK(str_3.is(str_1));
    }
    SUBCASE("from non-string") {
      const auto str = py::str(py::make_tuple(1, 2, 3));
      CHECK(str.val() == "(1, 2, 3)");
    }
  }
  SUBCASE("characters access") {
    const auto str = py::str("αβγ");
    SUBCASE("unicode") {
      CHECK(len(str) == 3);
      CHECK(str[1] == py::str("β"));
      CHECK(str[{1, 3}] == py::str("βγ"));
    }
    SUBCASE("immutability") {
      // CHECK_THROWS_MSG(
      //     str[0] = py::str("a"),
      //     Exception,
      //     "TypeError: 'str' object does not support item assignment");
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("tuple") {
  SUBCASE("construction") {
    SUBCASE("empty") {
      const auto tuple = py::tuple();
      CHECK(len(tuple) == 0);
    }
    SUBCASE("from iterable") {
      const auto iterable = py::make_list(1, 2, 3);
      const auto tuple = py::tuple(iterable);
      CHECK(tuple.is_not(iterable));
      CHECK(len(tuple) == 3);
      CHECK(tuple[0] == py::int_(1));
      CHECK(tuple[1] == py::int_(2));
      CHECK(tuple[2] == py::int_(3));
    }
    SUBCASE("from tuple") {
      const auto tuple_1 = py::tuple(py::make_tuple(1, 2, 3));
      const auto tuple_2 = py::tuple(tuple_1);
      const auto tuple_3 = py::tuple(tuple_1, py::keep);
      CHECK(tuple_2.is(tuple_1)); // Since tuples are immutable.
      CHECK(tuple_3.is(tuple_1));
    }
    SUBCASE("from non-iterable") {
      // CHECK_THROWS_MSG(py::tuple(py::None()),
      //                  Exception,
      //                  "TypeError: 'NoneType' object is not iterable");
    }
  }
  SUBCASE("methods") {
    const auto tuple = py::make_tuple(1, 2, 2, 3);
    SUBCASE("immutability") {
      // CHECK_THROWS_MSG(
      //     tuple[0] = py::int_(4),
      //     Exception,
      //     "TypeError: 'tuple' object does not support item assignment");
    }
    SUBCASE("search") {
      CHECK(tuple.count(2) == 2);
      CHECK(tuple.count(5) == 0);
      CHECK(tuple.count(2));
      CHECK_FALSE(tuple.count(5));
      CHECK(tuple.index(2) == 1);
      // CHECK_THROWS_MSG(tuple.index(5),
      //                  Exception,
      //                  "ValueError: sequence.index(x): x not in sequence");
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("list") {
  SUBCASE("construction") {
    SUBCASE("empty") {
      const auto list = py::list();
      CHECK(len(list) == 0);
    }
    SUBCASE("from iterable") {
      const auto iterable = py::make_tuple(1, 2, 3);
      const auto list = py::list(iterable);
      CHECK(list.is_not(iterable));
      CHECK(len(list) == 3);
      CHECK(list[0] == py::int_(1));
      CHECK(list[1] == py::int_(2));
      CHECK(list[2] == py::int_(3));
    }
    SUBCASE("from list") {
      const auto list_1 = py::list(py::make_tuple(1, 2, 3));
      const auto list_2 = py::list(list_1);
      const auto list_3 = py::list(list_1, py::keep);
      CHECK(list_2.is_not(list_1));
      CHECK(len(list_2) == 3);
      CHECK(list_2[0] == py::int_(1));
      CHECK(list_2[1] == py::int_(2));
      CHECK(list_2[2] == py::int_(3));
      CHECK(list_3.is(list_1));
    }
    SUBCASE("from non-iterable") {
      // CHECK_THROWS_MSG(py::list(py::None()),
      //                  Exception,
      //                  "TypeError: 'NoneType' object is not iterable");
    }
  }
  SUBCASE("methods") {
    SUBCASE("item access") {
      const auto list = py::make_list(1, 2, 3);
      CHECK(list[0] == py::int_(1));
      CHECK(list[1] == py::int_(2));
      CHECK(list[2] == py::int_(3));
      // CHECK_THROWS_MSG(py::repr(list[3]),
      //                  Exception,
      //                  "IndexError: list index out of range");

      list[0] = 4;
      CHECK(list[0] == py::int_(4));
      // CHECK_THROWS_MSG(list[3] = 5,
      //                  Exception,
      //                  "IndexError: list assignment index out of range");

      list.del(1);
      CHECK(list == py::make_list(4, 3));
      // CHECK_THROWS_MSG(list.del(3),
      //                  Exception,
      //                  "IndexError: list assignment index out of range");
    }
    SUBCASE("slice access") {
      const auto list = py::make_list(1, 2, 3, 4, 5);
      CHECK(list[{0, 3}] == py::make_list(1, 2, 3));
      CHECK(list[{1, 4}] == py::make_list(2, 3, 4));
      CHECK(list[{2, 5}] == py::make_list(3, 4, 5));
      CHECK_FALSE(list[{100, 101}]); // Out of range access is not an error.

      list[{0, 3}] = py::make_list(4, 5, 6);
      CHECK(list == py::make_list(4, 5, 6, 4, 5));
      list[{100, 101}] = py::make_list(7, 8); // Appends when out of range.
      CHECK(list == py::make_list(4, 5, 6, 4, 5, 7, 8));

      list.del({1, 3});
      list.del({100, 101}); // Does nothing when out of range.
      CHECK(list == py::make_list(4, 4, 5, 7, 8));
    }
    SUBCASE("append and insert") {
      const auto list = py::list();
      list.append(1);
      list.append(2);
      list.append(3);
      CHECK(list == py::make_list(1, 2, 3));
      list.insert(1, 4);
      CHECK(list == py::make_list(1, 4, 2, 3));
    }
    SUBCASE("sort") {
      const auto list = py::make_list(3, 1, 2);
      list.sort();
      CHECK(list == py::make_list(1, 2, 3));
    }
    SUBCASE("reverse") {
      const auto list = py::make_list(1, 2, 3);
      list.reverse();
      CHECK(list == py::make_list(3, 2, 1));
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("Mapping") {
  SUBCASE("construction") {
    SUBCASE("from non-mapping") {
      // CHECK_THROWS_MSG(py::Mapping{py::None()},
      //                  Exception,
      //                  "TypeError: 'NoneType' object is not a mapping");
    }
  }
  // Other methods are tested in "dict" test below.
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("dict") {
  SUBCASE("construction") {
    SUBCASE("empty") {
      const auto dict = py::dict();
      CHECK(len(dict) == 0);
    }
    SUBCASE("from iterable") {
      const auto iterable = py::make_list(py::make_tuple("a", 1),
                                          py::make_tuple("b", 2),
                                          py::make_tuple("c", 3));
      const auto dict = py::dict(iterable);
      CHECK(dict.is_not(iterable));
      CHECK(len(dict) == 3);
      CHECK(dict.has_key("a"));
      CHECK(dict.has_key("b"));
      CHECK(dict.has_key("c"));
      CHECK(dict["a"] == py::int_(1));
      CHECK(dict["b"] == py::int_(2));
      CHECK(dict["c"] == py::int_(3));
    }
    SUBCASE("from mapping") {
      /// @todo We should unindent the code on execution.
      interpreter.exec(R"PY(if True:
        # Minimal mapping class.
        class MyMapping:
          def keys(self):
            return ["a", "b", "c"]

          def __getitem__(self, key):
            return self.keys().index(key) + 1
      )PY");
      const py::Object MyMapping = interpreter.globals()["MyMapping"];
      const auto mapping = MyMapping();
      const auto dict = py::dict(mapping);
      CHECK(dict.is_not(mapping));
      CHECK(len(dict) == 3);
      CHECK(dict.has_key("a"));
      CHECK(dict.has_key("b"));
      CHECK(dict.has_key("c"));
      CHECK(dict["a"] == py::int_(1));
      CHECK(dict["b"] == py::int_(2));
      CHECK(dict["c"] == py::int_(3));
    }
    SUBCASE("from dict") {
      const auto dict_1 = py::dict(py::make_list(py::make_tuple("a", 1),
                                                 py::make_tuple("b", 2),
                                                 py::make_tuple("c", 3)));
      const auto dict_2 = py::dict(dict_1);
      const auto dict_3 = py::dict(dict_1, py::keep);
      CHECK(dict_2.is_not(dict_1));
      CHECK(dict_2["a"] == py::int_(1));
      CHECK(dict_2["b"] == py::int_(2));
      CHECK(dict_2["c"] == py::int_(3));
      CHECK(dict_3.is(dict_1));
    }
    SUBCASE("from non-mapping or non-iterable") {
      // CHECK_THROWS_MSG(py::dict(py::None()),
      //                  Exception,
      //                  "TypeError: 'NoneType' object is not iterable");
    }
  }
  SUBCASE("methods") {
    const auto dict =
        py::dict(py::make_list(py::make_tuple("a", 1), py::make_tuple("b", 2)));

    dict["c"] = 3;
    CHECK(len(dict) == 3);
    CHECK(dict.has_key("a"));
    CHECK(dict.has_key("b"));
    CHECK(dict.has_key("c"));
    CHECK(dict["a"] == py::int_(1));
    CHECK(dict["b"] == py::int_(2));
    CHECK(dict["c"] == py::int_(3));

    dict.del("c");
    CHECK(len(dict) == 2);
    CHECK_FALSE(dict.has_key("c"));

    dict["b"] = 4;
    CHECK(len(dict) == 2);
    CHECK(dict.has_key("b"));
    CHECK(dict["b"] == py::int_(4));

    CHECK(dict.keys() == py::make_list("a", "b"));
    CHECK(dict.values() == py::make_list(py::int_(1), py::int_(4)));
    CHECK(dict.items() == py::make_list(py::make_tuple("a", py::int_(1)),
                                        py::make_tuple("b", py::int_(4))));

    // CHECK_THROWS_MSG(py::repr(dict["d"]), Exception, "KeyError: 'd'");

    dict.clear();
    CHECK(len(dict) == 0);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("set") {
  SUBCASE("construction") {
    SUBCASE("empty") {
      const auto set = py::set();
      CHECK(len(set) == 0);
    }
    SUBCASE("from iterable") {
      SUBCASE("different type") {
        const auto iterable = py::make_tuple(1, 2, 3);
        const auto set = py::set(iterable);
        CHECK(set.is_not(iterable));
        CHECK(len(set) == 3);
        CHECK(set.has(1));
        CHECK(set.has(2));
        CHECK(set.has(3));
      }
      SUBCASE("from set") {
        const auto set_1 = py::make_set(1, 2, 3);
        const auto set_2 = py::set(set_1);
        const auto set_3 = py::set(set_1, py::keep);
        CHECK(set_2.is_not(set_1));
        CHECK(set_2.has(1));
        CHECK(set_2.has(2));
        CHECK(set_2.has(3));
        CHECK(set_3.is(set_1));
      }
    }
    SUBCASE("from non-iterable") {
      // CHECK_THROWS_MSG(py::set(py::None()),
      //                  Exception,
      //                  "TypeError: 'NoneType' object is not iterable");
    }
  }
  SUBCASE("methods") {
    const auto set = py::make_set(1, 2);

    set.add(3);
    CHECK(len(set) == 3);
    CHECK(set.has(1));
    CHECK(set.has(2));
    CHECK(set.has(3));

    set.discard(2);
    CHECK(len(set) == 2);
    CHECK(set.has(1));
    CHECK(set.has(3));

    const auto item = py::cast<int>(set.pop());
    CHECK(((item == 1) || (item == 3)));
    CHECK(len(set) == 1);

    set.clear();
    CHECK(len(set) == 0);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("Module") {
  SUBCASE("import") {
    SUBCASE("existing module") {
      const auto module = py::import_("numpy");
      CHECK(module.has_attr("ndarray"));
      CHECK(module.dict().has_key("ndarray"));
    }
  }
  SUBCASE("non-existing module") {
    // CHECK_THROWS_MSG(py::import_("does_not_exist"),
    //                  Exception,
    //                 "ModuleNotFoundError: No module named
    //                 'does_not_exist'");
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
