/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "tit/core/str_utils.hpp"

#include "tit/py/error.hpp"
#include "tit/py/number.hpp"
#include "tit/py/object.hpp"
#include "tit/py/sequence.hpp"

#include "tit/py/interpreter.testing.hpp"
#include "tit/testing/test.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// NOLINTBEGIN -- testing code distresses code analysis a lot.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wself-move"
#pragma GCC diagnostic ignored "-Wself-assign-overloaded"

TEST_CASE("py::BaseObject") {
  py::List obj1{};
  REQUIRE(obj1.get()->ob_refcnt == 1);
  SUBCASE("move construct") {
    const py::List obj2{std::move(obj1)};
    CHECK(obj2.get()->ob_refcnt == 1);
    CHECK_FALSE(obj1.valid());
    CHECK(obj2.valid());
  }
  SUBCASE("copy construct") {
    const py::List obj2{obj1};
    CHECK(obj2.get()->ob_refcnt == 2);
    CHECK(obj1.valid());
    CHECK(obj2.valid());
  }
  SUBCASE("move assign") {
    SUBCASE("self") {
      obj1 = std::move(obj1);
      CHECK(obj1.get()->ob_refcnt == 1);
      CHECK(obj1.valid());
    }
    SUBCASE("other") {
      py::List obj2{};
      obj2 = std::move(obj1);
      CHECK(obj2.get()->ob_refcnt == 1);
      CHECK_FALSE(obj1.valid());
      CHECK(obj2.valid());
    }
  }
  SUBCASE("copy assign") {
    SUBCASE("self") {
      obj1 = obj1;
      CHECK(obj1.get()->ob_refcnt == 1);
      CHECK(obj1.valid());
    }
    SUBCASE("other") {
      py::List obj2{};
      obj2 = obj1;
      CHECK(obj2.get()->ob_refcnt == 2);
      CHECK(obj1.valid());
      CHECK(obj2.valid());
    }
  }
  SUBCASE("reset") {
    SUBCASE("self") {
      obj1.reset(obj1.get());
      CHECK(obj1.get()->ob_refcnt == 1);
      CHECK(obj1.valid());
    }
    SUBCASE("other") {
      py::List obj2{};
      py::List obj3{obj2};
      obj3.reset(obj1.get());
      CHECK(obj1.get()->ob_refcnt == 1);
      CHECK(obj2.get()->ob_refcnt == 1);
      CHECK(obj3.get()->ob_refcnt == 1);
      CHECK(obj1.valid());
      CHECK(obj2.valid());
      CHECK(obj3.valid());
    }
  }
  SUBCASE("release") {
    auto* const ptr = obj1.release();
    CHECK_FALSE(obj1.valid());
    CHECK(ptr->ob_refcnt == 1);
    py::steal(ptr); // manual clean-up.
  }
  SUBCASE("incref/decref") {
    obj1.incref();
    CHECK(obj1.get()->ob_refcnt == 2);
    obj1.decref();
    CHECK(obj1.get()->ob_refcnt == 1);
  }
}

#pragma GCC diagnostic pop
// NOLINTEND

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("py::Object") {
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
                         py::ErrorException,
                         "AttributeError: 'MyClass' object has no attribute "
                         "'does_not_exist'");
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
                         py::ErrorException,
                         "AttributeError: 'MyClass' object has no attribute "
                         "'does_not_exist'");
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

} // namespace
} // namespace tit
