/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "tit/core/str_utils.hpp"

#include "tit/py/core.hpp"

#include "tit/py/_embed/interpreter.testing.hpp"
#include "tit/testing/test.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("Object") {
  SUBCASE("attributes") {
    REQUIRE(testing::interpreter().exec(R"PY(
      class MyClass:
        def __init__(self):
          self.x = 1
          self.y = 2
    )PY"));
    const py::Object MyClass = testing::interpreter().globals()["MyClass"];
    const auto obj = MyClass();
    const auto run_subcase = [&obj]<class Key> {
      SUBCASE("has_attr") {
        CHECK(obj.has_attr(Key{"x"}));
        CHECK(obj.has_attr(Key{"y"}));
        CHECK_FALSE(obj.has_attr("does_not_exist"));
      }
      SUBCASE("attr") {
        CHECK(obj.attr(Key{"x"}) == py::Int{1});
        CHECK(obj.attr(Key{"y"}) == py::Int{2});
        REQUIRE_FALSE(obj.has_attr(Key{"does_not_exist"}));
        CHECK_THROWS_MSG(obj.attr("does_not_exist"),
                         py::Error,
                         "AttributeError: 'MyClass' object has no attribute "
                         "'does_not_exist'");
        py::clear_error();
      }
      SUBCASE("set_attr") {
        obj.set_attr(Key{"x"}, 3);
        CHECK(obj.attr(Key{"x"}) == py::Int{3});
        obj.set_attr(Key{"z"}, 4);
        CHECK(obj.attr(Key{"z"}) == py::Int{4});
      }
      SUBCASE("del_attr") {
        obj.del_attr(Key{"x"});
        CHECK_FALSE(obj.has_attr(Key{"x"}));
        REQUIRE_FALSE(obj.has_attr(Key{"does_not_exist"}));
        CHECK_THROWS_MSG(obj.del_attr("does_not_exist"),
                         py::Error,
                         "AttributeError: 'MyClass' object has no attribute "
                         "'does_not_exist'");
        py::clear_error();
      }
    };
    SUBCASE("string names") {
      run_subcase.template operator()<CStrView>();
    }
    SUBCASE("object names") {
      run_subcase.template operator()<py::Str>();
    }
  }
  SUBCASE("operators") {
    SUBCASE("operator()") {
      const auto func = testing::interpreter().eval(R"PY(
        lambda *args, **kwargs: f"{args} {tuple(sorted(kwargs.items()))}"
      )PY");
      CHECK(func() == py::Str{"() ()"});
      CHECK(func(1, 2.0, "abc") == py::Str{"(1, 2.0, 'abc') ()"});
      const auto x = func(1, py::kwarg("x", 2.0), py::kwarg("y", "abc"));
      CHECK(py::str(x) == "(1,) (('x', 2.0), ('y', 'abc'))");
      CHECK(x == py::Str{"(1,) (('x', 2.0), ('y', 'abc'))"});
    }
  }
  SUBCASE("functions") {
    SUBCASE("hash") {
      CHECK(py::hash(py::Int{1}) == py::hash(py::Int{1}));
      CHECK(py::hash(py::Int{1}) != py::hash(py::Int{2}));
    }
    SUBCASE("str") {
      CHECK(py::str(py::Int{1}) == "1");
      CHECK(py::str(py::Float{1.5}) == "1.5");
      CHECK(py::str(py::Str{"abc"}) == "abc");
    }
    SUBCASE("repr") {
      CHECK(py::repr(py::Int{1}) == "1");
      CHECK(py::repr(py::Float{1.5}) == "1.5");
      CHECK(py::repr(py::Str{"abc"}) == "'abc'");
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("Bool") {
  SUBCASE("isinstance") {
    CHECK(py::isinstance<py::Bool>(py::Bool{}));
    CHECK_FALSE(py::isinstance<py::Bool>(py::Int{}));
  }
  SUBCASE("construction") {
    SUBCASE("from bool") {
      CHECK(py::Bool{true}.val());
      CHECK_FALSE(py::Bool{false}.val());
    }
    SUBCASE("from object") {
      CHECK(py::Bool{py::Str("abc")}.val());
      CHECK_FALSE(py::Bool{py::Str(/*empty*/)}.val());
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("Int") {
  /// @todo Add conversion tests.
  SUBCASE("isinstance") {
    CHECK(py::isinstance<py::Int>(py::Int{}));
    CHECK_FALSE(py::isinstance<py::Int>(py::Float{}));
  }
  SUBCASE("construction") {
    SUBCASE("from number") {
      CHECK(py::Int{}.val() == 0);
      CHECK(py::Int{3}.val() == 3);
      CHECK(py::Int{py::Float{2.99}}.val() == 2);
    }
    SUBCASE("from string") {
      CHECK(py::Int{py::Str{"3"}}.val() == 3);
      CHECK_THROWS_MSG(
          py::repr(py::Int{py::Str{"not-an-int"}}),
          py::Error,
          "ValueError: invalid literal for int() with base 10: 'not-an-int'");
      py::clear_error();
    }
    SUBCASE("failure") {
      CHECK_THROWS_MSG(py::repr(py::Int{py::None()}),
                       py::Error,
                       "TypeError: int() argument must be a string, a "
                       "bytes-like object or a real number, not 'NoneType'");
      py::clear_error();
    }
  }
  SUBCASE("operators") {
    SUBCASE("comparison") {
      CHECK(py::Int{1} == py::Int{1});
      CHECK(py::Int{1} != py::Int{2});
      CHECK(py::Int{1} < py::Int{2});
      CHECK(py::Int{2} > py::Int{1});
      CHECK(py::Int{1} <= py::Int{1});
      CHECK(py::Int{2} >= py::Int{1});
    }
    SUBCASE("arithmetic") {
      SUBCASE("operator/") {
        SUBCASE("normal") {
          CHECK(py::Int{5} / py::Int{2} == py::Float{2.5});
        }
        SUBCASE("augmented") {
          py::Int a{5};
          a /= py::Int{2};
          CHECK(a == py::Float{2.5});
        }
      }
      SUBCASE("floordiv") {
        SUBCASE("normal") {
          CHECK(py::floordiv(py::Int{5}, py::Int{2}) == py::Int{2});
        }
        SUBCASE("augmented") {
          py::Int a{5};
          py::floordiv_inplace(a, py::Int{2});
          CHECK(a == py::Int{2});
        }
      }
      SUBCASE("operator%") {
        SUBCASE("normal") {
          CHECK(py::Int{5} % py::Int{2} == py::Int{1});
        }
        SUBCASE("augmented") {
          py::Int a{5};
          a %= py::Int{2};
          CHECK(a == py::Int{1});
        }
      }
      // Other operators are tested in "Float" tests below.
    }
    SUBCASE("bitwise") {
      SUBCASE("operator~") {
        CHECK(~py::Int{5} == py::Int{-6});
      }
      SUBCASE("operator&") {
        SUBCASE("normal") {
          CHECK((py::Int{5} & py::Int{3}) == py::Int{1});
        }
        SUBCASE("augmented") {
          py::Int a{5};
          a &= py::Int{3};
          CHECK(a == py::Int{1});
        }
      }
      SUBCASE("operator|") {
        SUBCASE("normal") {
          CHECK((py::Int{5} | py::Int{3}) == py::Int{7});
        }
        SUBCASE("augmented") {
          py::Int a{5};
          a |= py::Int{3};
          CHECK(a == py::Int{7});
        }
      }
      SUBCASE("operator^") {
        SUBCASE("normal") {
          CHECK((py::Int{5} ^ py::Int{3}) == py::Int{6});
        }
        SUBCASE("augmented") {
          py::Int a{5};
          a ^= py::Int{3};
          CHECK(a == py::Int{6});
        }
      }
      SUBCASE("operator<<") {
        SUBCASE("normal") {
          CHECK((py::Int{5} << py::Int{3}) == py::Int{40});
        }
        SUBCASE("augmented") {
          py::Int a{5};
          a <<= py::Int{3};
          CHECK(a == py::Int{40});
        }
      }
      SUBCASE("operator>>") {
        SUBCASE("normal") {
          CHECK((py::Int{40} >> py::Int{3}) == py::Int{5});
        }
        SUBCASE("augmented") {
          py::Int a{40};
          a >>= py::Int{3};
          CHECK(a == py::Int{5});
        }
      }
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("Float") {
  SUBCASE("isinstance") {
    CHECK(py::isinstance<py::Float>(py::Float{}));
    CHECK_FALSE(py::isinstance<py::Float>(py::Int{}));
  }
  SUBCASE("construction") {
    SUBCASE("from number") {
      CHECK(py::Float{}.val() == 0.0);
      CHECK(py::Float{2.5}.val() == 2.5);
      CHECK(py::Float{py::Int{2}}.val() == 2.0);
    }
    SUBCASE("from string") {
      CHECK(py::Float{py::Str{"2.5"}}.val() == 2.5);
      CHECK_THROWS_MSG(
          py::repr(py::Float{py::Str{"not-a-number"}}),
          py::Error,
          "ValueError: could not convert string to float: 'not-a-number'");
      py::clear_error();
    }
    SUBCASE("failure") {
      CHECK_THROWS_MSG(py::repr(py::Float{py::None()}),
                       py::Error,
                       "TypeError: float() argument must be a string or a "
                       "real number, not 'NoneType'");
      py::clear_error();
    }
  }
  SUBCASE("operators") {
    SUBCASE("arithmetic") {
      SUBCASE("operator+") {
        SUBCASE("unary") {
          CHECK(+py::Float{2.5} == py::Float{2.5});
        }
        SUBCASE("normal") {
          CHECK(py::Float{2.5} + py::Float{1.5} == py::Float{4.0});
        }
        SUBCASE("augmented") {
          py::Float a(2.5);
          a += py::Float{1.5};
          CHECK(a == py::Float{4.0});
        }
      }
      SUBCASE("operator-") {
        SUBCASE("unary") {
          CHECK(-py::Float{2.5} == py::Float{-2.5});
        }
        SUBCASE("normal") {
          CHECK(py::Float{2.5} - py::Float{1.5} == py::Float{1.0});
        }
        SUBCASE("augmented") {
          auto a = py::Float{2.5};
          a -= py::Float{1.5};
          CHECK(a == py::Float{1.0});
        }
      }
      SUBCASE("operator*") {
        SUBCASE("normal") {
          CHECK(py::Float{2.5} * py::Float{1.5} == py::Float{3.75});
        }
        SUBCASE("augmented") {
          auto a = py::Float{2.5};
          a *= py::Float{1.5};
          CHECK(a == py::Float{3.75});
        }
      }
      SUBCASE("operator/") {
        SUBCASE("normal") {
          CHECK(py::Float{2.5} / py::Float{0.5} == py::Float{5.0});
        }
        SUBCASE("augmented") {
          auto a = py::Float{2.5};
          a /= py::Float{0.5};
          CHECK(a == py::Float{5.0});
        }
      }
      SUBCASE("abs") {
        CHECK(py::abs(py::Float{2.5}) == py::Float{2.5});
        CHECK(py::abs(py::Float{-2.5}) == py::Float{2.5});
      }
      SUBCASE("pow") {
        SUBCASE("normal") {
          CHECK(py::pow(py::Float{2.5}, py::Float{2.0}) == py::Float{6.25});
        }
        SUBCASE("augmented") {
          auto a = py::Float{2.5};
          py::pow_inplace(a, py::Float{2.0});
          CHECK(a == py::Float{6.25});
        }
      }
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("Iterator") {
  SUBCASE("isinstance") {
    CHECK(py::isinstance<py::Iterator>(py::iter(py::List{})));
    CHECK_FALSE(py::isinstance<py::Iterator>(py::Int{}));
  }
  SUBCASE("construction") {
    SUBCASE("from iterable") {
      const auto iterable = py::make_list(1, 2, 3);
      const auto iter = py::iter(iterable);
      CHECK(iter.next() == py::Int{1});
      CHECK(iter.next() == py::Int{2});
      CHECK(iter.next() == py::Int{3});
      CHECK_FALSE(iter.next());
    }
    SUBCASE("failure") {
      CHECK_THROWS_MSG(py::iter(py::None()),
                       py::Error,
                       "TypeError: 'NoneType' object is not iterable");
      py::clear_error();
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("Sequence") {
  SUBCASE("isinstance") {
    CHECK(py::isinstance<py::Sequence>(py::Str{}));
    CHECK_FALSE(py::isinstance<py::Sequence>(py::Int{}));
  }
  // Other methods are tested in "str", "tuple" and "list" tests below.
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("Str") {
  SUBCASE("isinstance") {
    CHECK(py::isinstance<py::Str>(py::Str{}));
    CHECK_FALSE(py::isinstance<py::Str>(py::Int{}));
  }
  SUBCASE("construction") {
    SUBCASE("from string") {
      CHECK(!py::Str{});
      CHECK(py::Str{}.val().empty());
      CHECK(py::Str{"abc"}.val() == "abc");
    }
    SUBCASE("from non-string") {
      CHECK(py::Str{py::make_tuple(1, 2, 3)}.val() == "(1, 2, 3)");
    }
  }
  SUBCASE("operators") {
    SUBCASE("operator[]") {
      const auto str = py::Str{"αβγ"};
      CHECK(py::len(str) == 3);
      CHECK(str[1] == py::Str{"β"});
      CHECK(str[{1, 3}] == py::Str{"βγ"});
      CHECK_THROWS_MSG(
          str[0] = py::Str{"a"},
          py::Error,
          "TypeError: 'str' object does not support item assignment");
      py::clear_error();
    }
    SUBCASE("operator+") {
      SUBCASE("normal") {
        CHECK(py::Str{"abc"} + py::Str{"def"} == py::Str{"abcdef"});
      }
      SUBCASE("augmented") {
        auto str = py::Str{"abc"};
        str += py::Str{"def"};
        CHECK(str == py::Str{"abcdef"});
      }
    }
    SUBCASE("operator*") {
      SUBCASE("normal") {
        CHECK(py::Str{"ab"} * 3 == py::Str{"ababab"});
        CHECK(3 * py::Str{"ab"} == py::Str{"ababab"});
      }
      SUBCASE("augmented") {
        py::Str str{"ab"};
        str *= 3;
        CHECK(str == py::Str{"ababab"});
      }
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("Tuple") {
  SUBCASE("isinstance") {
    CHECK(py::isinstance<py::Tuple>(py::Tuple{}));
    CHECK_FALSE(py::isinstance<py::Tuple>(py::List{}));
  }
  SUBCASE("construction") {
    SUBCASE("empty") {
      CHECK(!py::Tuple{});
      CHECK(py::len(py::Tuple{}) == 0);
    }
    SUBCASE("from items") {
      const auto tuple = py::make_tuple(1, 2, 3);
      CHECK(py::len(tuple) == 3);
      CHECK(tuple[0] == py::Int{1});
      CHECK(tuple[1] == py::Int{2});
      CHECK(tuple[2] == py::Int{3});
    }
    SUBCASE("from iterable") {
      const auto iterable = py::make_list(1, 2, 3);
      const py::Tuple tuple{iterable};
      CHECK_FALSE(tuple.is(iterable));
      CHECK(tuple == py::make_tuple(1, 2, 3));
    }
    SUBCASE("failure") {
      CHECK_THROWS_MSG(py::repr(py::Tuple{py::None()}),
                       py::Error,
                       "TypeError: 'NoneType' object is not iterable");
      py::clear_error();
    }
  }
  SUBCASE("methods") {
    const auto tuple = py::make_tuple(1, 2, 2, 3);
    CHECK(tuple.count(2) == 2);
    CHECK(tuple.count(5) == 0);
    CHECK(tuple.contains(2));
    CHECK_FALSE(tuple.contains(5));
    CHECK(tuple.index(2) == 1);
    CHECK_THROWS_MSG(tuple.index(5),
                     py::Error,
                     "ValueError: sequence.index(x): x not in sequence");
    py::clear_error();
  }
  SUBCASE("operators") {
    SUBCASE("operator[]") {
      const auto tuple = py::make_tuple(1, 2, 2, 3);
      CHECK_THROWS_MSG(
          tuple[0] = py::Int{4},
          py::Error,
          "TypeError: 'tuple' object does not support item assignment");
      py::clear_error();
    }
    SUBCASE("operator+") {
      SUBCASE("normal") {
        CHECK(py::make_tuple(1, 2, 3) + py::make_tuple(4, 5, 6) ==
              py::make_tuple(1, 2, 3, 4, 5, 6));
      }
      SUBCASE("augmented") {
        auto tuple = py::make_tuple(1, 2, 3);
        tuple += py::make_tuple(4, 5, 6);
        CHECK(tuple == py::make_tuple(1, 2, 3, 4, 5, 6));
      }
    }
    SUBCASE("operator*") {
      SUBCASE("normal") {
        CHECK(py::make_tuple(1, 2) * 3 == py::make_tuple(1, 2, 1, 2, 1, 2));
        CHECK(3 * py::make_tuple(1, 2) == py::make_tuple(1, 2, 1, 2, 1, 2));
      }
      SUBCASE("augmented") {
        auto tuple = py::make_tuple(1, 2);
        tuple *= 3;
        CHECK(tuple == py::make_tuple(1, 2, 1, 2, 1, 2));
      }
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("List") {
  SUBCASE("isinstance") {
    CHECK(py::isinstance<py::List>(py::List{}));
    CHECK_FALSE(py::isinstance<py::List>(py::Tuple{}));
  }
  SUBCASE("construction") {
    SUBCASE("empty") {
      CHECK(!py::List{});
      CHECK(py::len(py::List{}) == 0);
    }
    SUBCASE("from items") {
      const auto list = py::make_list(1, 2, 3);
      CHECK(py::len(list) == 3);
      CHECK(list[0] == py::Int{1});
      CHECK(list[1] == py::Int{2});
      CHECK(list[2] == py::Int{3});
    }
    SUBCASE("from iterable") {
      const auto iterable = py::make_tuple(1, 2, 3);
      const py::List list{iterable};
      CHECK_FALSE(list.is(iterable));
      CHECK(list == py::make_list(1, 2, 3));
    }
    SUBCASE("failure") {
      CHECK_THROWS_MSG(py::repr(py::List{py::None()}),
                       py::Error,
                       "TypeError: 'NoneType' object is not iterable");
      py::clear_error();
    }
  }
  SUBCASE("methods") {
    SUBCASE("append and insert") {
      const py::List list{};
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
  SUBCASE("operators") {
    SUBCASE("operator[]") {
      auto list = py::make_list(1, 2, 3);
      CHECK(list[0] == py::Int{1});
      CHECK(list[1] == py::Int{2});
      CHECK(list[2] == py::Int{3});
      CHECK_THROWS_MSG(py::repr(list[3]),
                       py::Error,
                       "IndexError: list index out of range");
      py::clear_error();

      list[0] = 4;
      CHECK(list[0] == py::Int{4});
      CHECK_THROWS_MSG(list[3] = 5,
                       py::Error,
                       "IndexError: list assignment index out of range");
      py::clear_error();

      list.del(1);
      CHECK(list == py::make_list(4, 3));
      CHECK_THROWS_MSG(list.del(3),
                       py::Error,
                       "IndexError: list assignment index out of range");
      py::clear_error();

      list = py::make_list(1, 2, 3, 4, 5);
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
    SUBCASE("operator+") {
      SUBCASE("normal") {
        CHECK(py::make_list(1, 2, 3) + py::make_list(4, 5, 6) ==
              py::make_list(1, 2, 3, 4, 5, 6));
      }
      SUBCASE("augmented") {
        auto list = py::make_list(1, 2, 3);
        list += py::make_list(4, 5, 6);
        CHECK(list == py::make_list(1, 2, 3, 4, 5, 6));
      }
    }
    SUBCASE("operator*") {
      SUBCASE("normal") {
        CHECK(py::make_list(1, 2) * 3 == py::make_list(1, 2, 1, 2, 1, 2));
        CHECK(3 * py::make_list(1, 2) == py::make_list(1, 2, 1, 2, 1, 2));
      }
      SUBCASE("augmented") {
        auto list = py::make_list(1, 2);
        list *= 3;
        CHECK(list == py::make_list(1, 2, 1, 2, 1, 2));
      }
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("Mapping") {
  SUBCASE("isinstance") {
    CHECK(py::isinstance<py::Mapping>(py::Dict{}));
    CHECK(py::isinstance<py::Mapping>(py::List{})); // Yes, list is a mapping.
    CHECK_FALSE(py::isinstance<py::Mapping>(py::Int{}));
  }
  // Other methods are tested in "dict" tests below.
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("Dict") {
  SUBCASE("isinstance") {
    CHECK(py::isinstance<py::Dict>(py::Dict{}));
    CHECK_FALSE(py::isinstance<py::Dict>(py::List{}));
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
                       py::Error,
                       "TypeError: 'NoneType' object is not iterable");
      py::clear_error();
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
                           py::Error,
                           "KeyError: 'does_not_exist'");
          py::clear_error();
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
                           py::Error,
                           "KeyError: 'does_not_exist'");
          py::clear_error();
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

TEST_CASE("Set") {
  SUBCASE("isinstance") {
    CHECK(py::isinstance<py::Set>(py::Set{}));
    CHECK_FALSE(py::isinstance<py::Set>(py::Dict{}));
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
                       py::Error,
                       "TypeError: 'NoneType' object is not iterable");
      py::clear_error();
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
                       py::Error,
                       "KeyError: 'pop from an empty set'");
      py::clear_error();
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
