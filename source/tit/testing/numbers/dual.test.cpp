/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <numbers>

#include "tit/testing/numbers/dual.hpp"
#include "tit/testing/test.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("Dual") {
  const Dual d{1.0, 2.0};
  CHECK(d.val() == 1.0);
  CHECK(d.deriv() == 2.0);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("Dual::operator+") {
  SUBCASE("normal") {
    const auto d = Dual{1.0, 2.0} + Dual{3.0, 4.0};
    CHECK(d.val() == 4.0);
    CHECK(d.deriv() == 6.0);
  }
  SUBCASE("with assignment") {
    Dual d{1.0, 2.0};
    d += Dual{3.0, 4.0};
    CHECK(d.val() == 4.0);
    CHECK(d.deriv() == 6.0);
  }
}

TEST_CASE("Dual::operator-") {
  SUBCASE("negation") {
    const auto d = -Dual{1.0, 2.0};
    CHECK(d.val() == -1.0);
    CHECK(d.deriv() == -2.0);
  }
  SUBCASE("subtraction") {
    SUBCASE("normal") {
      const Dual d = Dual{1.0, 2.0} - Dual{3.0, 4.0};
      CHECK(d.val() == -2.0);
      CHECK(d.deriv() == -2.0);
    }
    SUBCASE("with assignment") {
      Dual d{1.0, 2.0};
      d -= Dual{3.0, 4.0};
      CHECK(d.val() == -2.0);
      CHECK(d.deriv() == -2.0);
    }
  }
}

TEST_CASE("Dual::operator*") {
  SUBCASE("scaling") {
    SUBCASE("normal") {
      SUBCASE("real * dual") {
        const auto d = 2.0 * Dual{1.0, 2.0};
        CHECK(d.val() == 2.0);
        CHECK(d.deriv() == 4.0);
      }
      SUBCASE("dual * real") {
        const auto d = Dual{1.0, 2.0} * 2.0;
        CHECK(d.val() == 2.0);
        CHECK(d.deriv() == 4.0);
      }
    }
    SUBCASE("with assignment") {
      Dual d{1.0, 2.0};
      d *= 2.0;
      CHECK(d.val() == 2.0);
      CHECK(d.deriv() == 4.0);
    }
  }
  SUBCASE("multiplication") {
    SUBCASE("normal") {
      const Dual d = Dual{1.0, 2.0} * Dual{3.0, 4.0};
      CHECK(d.val() == 3.0);
      CHECK(d.deriv() == 10.0);
    }
    SUBCASE("with assignment") {
      Dual d{1.0, 2.0};
      d *= Dual{3.0, 4.0};
      CHECK(d.val() == 3.0);
      CHECK(d.deriv() == 10.0);
    }
  }
}

TEST_CASE("Dual::operator/") {
  SUBCASE("scaling") {
    SUBCASE("normal") {
      const auto d = Dual{1.0, 2.0} / 2.0;
      CHECK(d.val() == 0.5);
      CHECK(d.deriv() == 1.0);
    }
    SUBCASE("with assignment") {
      Dual d{1.0, 2.0};
      d /= 2.0;
      CHECK(d.val() == 0.5);
      CHECK(d.deriv() == 1.0);
    }
  }
  SUBCASE("inverse") {
    const auto d = 3.0 / Dual{1.0, 2.0};
    CHECK(d.val() == 3.0);
    CHECK(d.deriv() == -6.0);
  }
  SUBCASE("division") {
    SUBCASE("normal") {
      const auto d = Dual{3.0, 4.0} / Dual{1.0, 2.0};
      CHECK(d.val() == 3.0);
      CHECK(d.deriv() == -2.0);
    }
    SUBCASE("with assignment") {
      Dual d{3.0, 4.0};
      d /= Dual{1.0, 2.0};
      CHECK(d.val() == 3.0);
      CHECK(d.deriv() == -2.0);
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("Dual::operator<=>") {
  // Note: Dual numbers are compared by value only.
  SUBCASE("equality") {
    CHECK(Dual{3.0, 1.0} == Dual{3.0, 2.0});
    CHECK(Dual{3.0, 1.0} != Dual{4.0, 1.0});
  }
  SUBCASE("ordering") {
    CHECK(Dual{3.0, 1.0} < Dual{4.0, 1.0});
    CHECK(Dual{3.0, 1.0} > Dual{2.0, 1.0});
    CHECK(Dual{3.0, 2.0} <= Dual{3.0, 1.0});
    CHECK(Dual{3.0, 0.0} >= Dual{3.0, 1.0});
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("Dual::sqrt") {
  const auto d = sqrt(Dual{4.0, 1.0});
  CHECK(d.val() == 2.0);
  CHECK(d.deriv() == 0.25);
}

TEST_CASE("Dual::pow") {
  SUBCASE("dual ** real") {
    const auto d = pow(Dual{2.0, 1.0}, 3.0);
    CHECK(d.val() == 8.0);
    CHECK(d.deriv() == 12.0);
  }
  SUBCASE("dual ** dual") {
    using std::numbers::e;
    const auto d = pow(Dual{e, 1.0}, Dual{2.0, 1.0});
    CHECK(d.val() == e * e);
    CHECK_APPROX_EQ(d.deriv(), e * (e + 2.0));
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("Dual::exp") {
  using std::numbers::e;
  const auto d = exp(Dual{1.0, 2.0});
  CHECK(d.val() == e);
  CHECK(d.deriv() == 2.0 * e);
}

TEST_CASE("Dual::log") {
  using std::numbers::e;
  const auto d = log(Dual{e, 2.0});
  CHECK(d.val() == 1.0);
  CHECK_APPROX_EQ(d.deriv(), 2.0 / e);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
