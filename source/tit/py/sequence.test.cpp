/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "tit/core/str_utils.hpp"

#include "tit/py/error.hpp"
#include "tit/py/number.hpp"
#include "tit/py/object.hpp"
#include "tit/py/sequence.hpp"

#include "tit/testing/test.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("py::Sequence") {
  SUBCASE("typing") {
    CHECK(py::Sequence::type_name() == "Sequence");
    CHECK(py::Sequence::isinstance(py::Str{}));
    CHECK_FALSE(py::Sequence::isinstance(py::Int{}));
  }
  // Other methods are tested in "Str", "Tuple" and "List" tests below.
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("py::Str") {
  SUBCASE("typing") {
    CHECK(py::Str::type().fully_qualified_name() == "str");
    CHECK(py::Str::isinstance(py::Str{}));
    CHECK_FALSE(py::Str::isinstance(py::Int{}));
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
          py::ErrorException,
          "TypeError: 'str' object does not support item assignment");
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

TEST_CASE("py::Tuple") {
  SUBCASE("typing") {
    CHECK(py::Tuple::type().fully_qualified_name() == "tuple");
    CHECK(py::Tuple::isinstance(py::Tuple{}));
    CHECK_FALSE(py::Tuple::isinstance(py::List{}));
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
                       py::ErrorException,
                       "TypeError: 'NoneType' object is not iterable");
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
                     py::ErrorException,
                     "ValueError: sequence.index(x): x not in sequence");
  }
  SUBCASE("operators") {
    SUBCASE("operator[]") {
      const auto tuple = py::make_tuple(1, 2, 2, 3);
      CHECK_THROWS_MSG(
          tuple[0] = py::Int{4},
          py::ErrorException,
          "TypeError: 'tuple' object does not support item assignment");
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

TEST_CASE("py::List") {
  SUBCASE("typing") {
    CHECK(py::List::type().fully_qualified_name() == "list");
    CHECK(py::List::isinstance(py::List{}));
    CHECK_FALSE(py::List::isinstance(py::Tuple{}));
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
                       py::ErrorException,
                       "TypeError: 'NoneType' object is not iterable");
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
                       py::ErrorException,
                       "IndexError: list index out of range");

      list[0] = 4;
      CHECK(list[0] == py::Int{4});
      CHECK_THROWS_MSG(list[3] = 5,
                       py::ErrorException,
                       "IndexError: list assignment index out of range");

      list.del(1);
      CHECK(list == py::make_list(4, 3));
      CHECK_THROWS_MSG(list.del(3),
                       py::ErrorException,
                       "IndexError: list assignment index out of range");

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

} // namespace
} // namespace tit
