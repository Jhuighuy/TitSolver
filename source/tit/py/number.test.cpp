/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "tit/py/error.hpp"
#include "tit/py/number.hpp"
#include "tit/py/object.hpp"
#include "tit/py/sequence.hpp"

#include "tit/testing/test.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("py::Bool") {
  SUBCASE("bool") {
    CHECK(py::Bool::type().fully_qualified_name() == "bool");
    CHECK(py::Bool::isinstance(py::Bool{}));
    CHECK_FALSE(py::Bool::isinstance(py::Int{}));
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

TEST_CASE("py::Int") {
  SUBCASE("typing") {
    CHECK(py::Int::type().fully_qualified_name() == "int");
    CHECK(py::Int::isinstance(py::Int{}));
    CHECK_FALSE(py::Int::isinstance(py::Float{}));
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
          py::ErrorException,
          "ValueError: invalid literal for int() with base 10: 'not-an-int'");
    }
    SUBCASE("failure") {
      CHECK_THROWS_MSG(py::repr(py::Int{py::None()}),
                       py::ErrorException,
                       "TypeError: int() argument must be a string, a "
                       "bytes-like object or a real number, not 'NoneType'");
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

TEST_CASE("py::Float") {
  SUBCASE("typing") {
    CHECK(py::Float::type().fully_qualified_name() == "float");
    CHECK(py::Float::isinstance(py::Float{}));
    CHECK_FALSE(py::Float::isinstance(py::Int{}));
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
          py::ErrorException,
          "ValueError: could not convert string to float: 'not-a-number'");
    }
    SUBCASE("failure") {
      CHECK_THROWS_MSG(py::repr(py::Float{py::None()}),
                       py::ErrorException,
                       "TypeError: float() argument must be a string or a "
                       "real number, not 'NoneType'");
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

} // namespace
} // namespace tit
