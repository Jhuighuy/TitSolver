/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <filesystem>
#include <string>

#include "tit/py/error.hpp"
#include "tit/py/func.hpp"
#include "tit/py/number.hpp"
#include "tit/py/object.hpp"

#include "tit/testing/test.hpp"

namespace tit {
namespace {

// NOLINTBEGIN(*-const-correctness)

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("py::make_func") {
  SUBCASE("invoke") {
    SUBCASE("returns nothing") {
      const auto func = py::make_func<"func", [] {}>();
      CHECK(func().is(py::None()));
    }
    SUBCASE("no arguments") {
      const auto func = py::make_func<"func", [] { return py::Int{1}; }>();
      SUBCASE("success") {
        CHECK(func() == py::Int{1});
      }
      SUBCASE("failure") {
        CHECK_THROWS_MSG(func(1),
                         py::ErrorException,
                         "TypeError: function 'func': function takes no "
                         "arguments (1 given)");
        CHECK_THROWS_MSG(func(1, py::kwarg("b", 2)),
                         py::ErrorException,
                         "TypeError: function 'func': function takes no "
                         "arguments (2 given)");
      }
    }
    SUBCASE("with arguments") {
      const auto func = py::make_func<"func",
                                      [](int a, int b) { return a + b; },
                                      py::Param<int, "a">,
                                      py::Param<int, "b">>();
      SUBCASE("success") {
        SUBCASE("positional arguments") {
          CHECK(func(1, 2) == py::Int{3});
        }
        SUBCASE("keyword arguments") {
          CHECK(func(py::kwarg("b", 2), py::kwarg("a", 1)) == py::Int{3});
        }
        SUBCASE("mixed arguments") {
          CHECK(func(1, py::kwarg("b", 2)) == py::Int{3});
        }
      }
      SUBCASE("failure") {
        SUBCASE("too many arguments") {
          CHECK_THROWS_MSG(
              func(1, 2, 3),
              py::ErrorException,
              "TypeError: function 'func': function takes at most 2 "
              "arguments (3 given)");
        }
        SUBCASE("missing argument") {
          CHECK_THROWS_MSG(func(1),
                           py::ErrorException,
                           "TypeError: function 'func': missing argument 'b'");
        }
        SUBCASE("unexpected argument") {
          CHECK_THROWS_MSG(
              func(1, 2, py::kwarg("c", 3)),
              py::ErrorException,
              "TypeError: function 'func': unexpected argument 'c'");
        }
        SUBCASE("duplicate argument") {
          CHECK_THROWS_MSG(
              func(1, 2, py::kwarg("b", 3)),
              py::ErrorException,
              "TypeError: function 'func': duplicate argument 'b'");
        }
        SUBCASE("wrong argument type") {
          CHECK_THROWS_MSG(func(1, 2.0),
                           py::ErrorException,
                           "TypeError: function 'func': argument 'b': "
                           "expected 'int', got 'float'");
        }
      }
    }
    SUBCASE("default arguments") {
      const auto func =
          py::make_func<"func",
                        [](int a, int b, int c) { return a + b + c; },
                        py::Param<int, "a">,
                        py::Param<int, "b", 2>,
                        py::Param<int, "c", [] { return 3; }>>();
      SUBCASE("success") {
        CHECK(func(1) == py::Int{6});
        CHECK(func(1, 3) == py::Int{7});
        CHECK(func(1, 3, 4) == py::Int{8});
        CHECK(func(1, py::kwarg("c", 4)) == py::Int{7});
      }
      SUBCASE("failure") {
        SUBCASE("missing argument") {
          CHECK_THROWS_MSG(func(),
                           py::ErrorException,
                           "TypeError: function 'func': missing argument 'a'");
        }
      }
    }
  }
  SUBCASE("exceptions") {
    SUBCASE("assertion error") {
      const auto func = py::make_func<"func", [] { //
        return std::string{}.at(1);
      }>();
      CHECK_THROWS_MSG(func(), py::ErrorException, "AssertionError");
    }
    SUBCASE("system error") {
      const auto func = py::make_func<"func", [] {
        std::filesystem::create_directory("/this/call/should/throw");
      }>();
      CHECK_THROWS_MSG(func(), py::ErrorException, "SystemError");
    }
    SUBCASE("unknown C++ exception") {
      const auto func = py::make_func<"func", [] {
        throw "Who cares about throwing std::exception, right?!";
      }>();
      CHECK_THROWS_MSG(func(),
                       py::ErrorException,
                       "SystemError: unknown error.");
    }
    SUBCASE("exception in default argument") {
      const auto func = py::make_func<
          "func",
          [](int a) { return a; },
          py::Param<int, "a", [] -> int { return std::string{}.at(1); }>>();
      CHECK_THROWS_MSG(func(), py::ErrorException, "AssertionError");
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// NOLINTEND(*-const-correctness)

} // namespace
} // namespace tit
