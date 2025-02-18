/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <string>
#include <string_view>

#include "tit/core/basic_types.hpp"
#include "tit/core/str_utils.hpp"

#include "tit/py/_python.hpp"
#include "tit/py/error.hpp"
#include "tit/py/module.hpp"
#include "tit/py/number.hpp"
#include "tit/py/object.hpp"
#include "tit/py/sequence.hpp"

#include "tit/py/interpreter.testing.hpp"
#include "tit/testing/test.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// NOLINTBEGIN -- testing code below distresses code analysis by a lot.
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
  SUBCASE("typing") {
    CHECK(py::Object::type_name() == "object");
    CHECK(py::Object::isinstance(py::Object{}));
    CHECK(py::Object::isinstance(py::Int{}));
  }
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

TEST_CASE("py::NoneType") {
  SUBCASE("typing") {
    CHECK(py::NoneType::type_name() == "NoneType");
    CHECK(py::NoneType::isinstance(py::None()));
    CHECK_FALSE(py::NoneType::isinstance(py::Int{}));
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("py::Type") {
  SUBCASE("typing") {
    CHECK(py::Type::type().fully_qualified_name() == "type");
    CHECK(py::Type::isinstance(py::type(py::Int{})));
    CHECK_FALSE(py::Type::isinstance(py::Int{}));
  }
  SUBCASE("properties") {
    SUBCASE("builtin types") {
      const auto int_type = py::type(py::Int{});
      CHECK(int_type.name() == "int");
      CHECK(int_type.qualified_name() == "int");
      CHECK(int_type.fully_qualified_name() == "int");
      CHECK(int_type.module_name() == "builtins");
    }
    SUBCASE("third-party types") {
      const auto numpy = py::import_("numpy");
      const auto ndarray = py::expect<py::Type>(numpy.attr("ndarray"));
      CHECK(ndarray.name() == "ndarray");
      CHECK(ndarray.qualified_name() == "ndarray");
      CHECK(ndarray.fully_qualified_name() == "numpy.ndarray");
      CHECK(ndarray.module_name() == "numpy");
    }
  }
  SUBCASE("methods") {
    SUBCASE("is_subtype_of") {
      CHECK(py::BaseException::type().is_subtype_of(py::BaseException::type()));
      const auto SystemError =
          py::expect<py::Type>(testing::interpreter().eval("SystemError"));
      CHECK(SystemError.is_subtype_of(py::BaseException::type()));
      CHECK_FALSE(SystemError.is_subtype_of(py::Int::type()));
    }
  }
}

TEST_CASE("py::type") {
  const py::Object obj = py::Int{1};
  CHECK(py::type(obj).is(py::Int::type()));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("py::expect") {
  SUBCASE("success") {
    const auto obj = py::Int{1};
    py::expect<py::Int>(obj);
  }
  SUBCASE("failure") {
    SUBCASE("concrete type") {
      CHECK_THROWS_MSG(py::expect<py::Float>(py::Dict{}),
                       py::ErrorException,
                       "TypeError: expected 'float', got 'dict'");
    }
    SUBCASE("abstract type") {
      CHECK_THROWS_MSG(py::expect<py::Mapping>(py::Int{}),
                       py::ErrorException,
                       "TypeError: expected 'Mapping', got 'int'");
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("py::object<Object>") {
  const py::Object obj = py::Int{1};
  SUBCASE("object") {
    CHECK(py::object(obj).is(obj));
  }
  SUBCASE("extract") {
    SUBCASE("success") {
      CHECK(py::extract<py::Int>(py::object(obj)).is(obj));
    }
    SUBCASE("failure") {
      CHECK_THROWS_MSG(py::extract<py::Bool>(obj),
                       py::ErrorException,
                       "TypeError: expected 'bool', got 'int'");
    }
  }
}

TEST_CASE("py::object<Bool>") {
  SUBCASE("object") {
    CHECK(py::object(true) == py::Bool{true});
    CHECK(py::object(false) == py::Bool{false});
  }
  SUBCASE("extract") {
    SUBCASE("success") {
      CHECK(py::extract<bool>(py::Bool{true}));
      CHECK_FALSE(py::extract<bool>(py::Bool{false}));
    }
    SUBCASE("failure") {
      CHECK_THROWS_MSG(py::extract<bool>(py::make_list(1, 2, 3)),
                       py::ErrorException,
                       "TypeError: expected 'bool', got 'list'");
    }
  }
}

TEST_CASE("py::object<Int>") {
  SUBCASE("object") {
    CHECK(py::object(int8_t{1}) == py::Int{1});
    CHECK(py::object(uint16_t{2}) == py::Int{2});
    CHECK(py::object(int64_t{3}) == py::Int{3});
  }
  SUBCASE("extract") {
    SUBCASE("success") {
      CHECK(py::extract<uint8_t>(py::Int{1}) == uint8_t{1});
      CHECK(py::extract<int16_t>(py::Int{2}) == int16_t{2});
      CHECK(py::extract<uint64_t>(py::Int{3}) == uint64_t{3});
    }
    SUBCASE("failure") {
      CHECK_THROWS_MSG(py::extract<int>(py::make_list(1, 2, 3)),
                       py::ErrorException,
                       "TypeError: expected 'int', got 'list'");
    }
  }
}

TEST_CASE("py::object<Float>") {
  using long_double_t = long double;
  SUBCASE("object") {
    CHECK(py::object(1.0F) == py::Float{1.0});
    CHECK(py::object(2.0) == py::Float{2.0});
    CHECK(py::object(long_double_t{3.0}) == py::Float{3.0});
  }
  SUBCASE("extract") {
    SUBCASE("success") {
      CHECK(py::extract<float>(py::Float{1.0}) == 1.0F);
      CHECK(py::extract<double>(py::Float{2.0}) == 2.0);
      CHECK(py::extract<long_double_t>(py::Float{3.0}) == long_double_t{3.0});
    }
    SUBCASE("failure") {
      CHECK_THROWS_MSG(py::extract<double>(py::make_list(1, 2, 3)),
                       py::ErrorException,
                       "TypeError: expected 'float', got 'list'");
    }
  }
}

TEST_CASE("py::object<Str>") {
  SUBCASE("object") {
    CHECK(py::object("abc") == py::Str{"abc"});
    CHECK(py::object(CStrView{"abc"}) == py::Str{"abc"});
    CHECK(py::object(std::string{"abc"}) == py::Str{"abc"});
    CHECK(py::object(std::string_view{"abc"}) == py::Str{"abc"});
  }
  SUBCASE("extract") {
    SUBCASE("success") {
      CHECK(py::extract<CStrView>(py::Str{"abc"}) == "abc");
      CHECK(py::extract<std::string>(py::Str{"abc"}) == "abc");
      CHECK(py::extract<std::string_view>(py::Str{"abc"}) == "abc");
    }
    SUBCASE("failure") {
      CHECK_THROWS_MSG(py::extract<std::string_view>(py::make_list(1, 2, 3)),
                       py::ErrorException,
                       "TypeError: expected 'str', got 'list'");
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
