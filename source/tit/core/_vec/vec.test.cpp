/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <algorithm>
#include <array>
#include <format>
#include <random>
#include <utility>

#include "tit/core/basic_types.hpp"
#include "tit/core/math.hpp"
#include "tit/core/simd.hpp"
#include "tit/core/vec.hpp"

#include "tit/core/serialization.testing.hpp"
#include "tit/testing/numbers/tagged.hpp"
#include "tit/testing/test.hpp"

namespace tit {
namespace {

// To test implementations with and without SIMD.
#define NUM_TYPES float, double, Tagged<double>

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE_TEMPLATE("Vec", Num, NUM_TYPES) {
  SUBCASE("zero initialization") {
    Vec<Num, 2> v{};
    CHECK(v[0] == Num{0});
    CHECK(v[1] == Num{0});
    CHECK(v.elems() == std::array<Num, 2>{});
  }
  SUBCASE("zero assignment") {
    Vec v{Num{1}, Num{2}};
    v = {};
    CHECK(v[0] == Num{0});
    CHECK(v[1] == Num{0});
    CHECK(v.elems() == std::array<Num, 2>{});
  }
  SUBCASE("value initialization") {
    const Vec<Num, 2> v(Num{3});
    CHECK(v[0] == Num{3});
    CHECK(v[1] == Num{3});
    CHECK(v.elems() == std::array{Num{3}, Num{3}});
  }
  SUBCASE("aggregate initialization") {
    const Vec v{Num{1}, Num{2}};
    CHECK(v[0] == Num{1});
    CHECK(v[1] == Num{2});
    CHECK(v.elems() == std::array{Num{1}, Num{2}});
  }
  SUBCASE("aggregate assignment") {
    Vec<Num, 2> v;
    v = {Num{3}, Num{4}};
    CHECK(v[0] == Num{3});
    CHECK(v[1] == Num{4});
    CHECK(v.elems() == std::array{Num{3}, Num{4}});
  }
  SUBCASE("subscript") {
    Vec<Num, 2> v;
    v[0] = Num{3}, v[1] = Num{4};
    CHECK(v[0] == Num{3});
    CHECK(v[1] == Num{4});
    v.elems() = {Num{5}, Num{6}};
    CHECK(v.elems() == std::array{Num{5}, Num{6}});
  }
  SUBCASE("elements") {
    Vec<Num, 2> v;
    v.elems() = {Num{3}, Num{4}};
    CHECK(v.elems() == std::array{Num{3}, Num{4}});
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE_TEMPLATE("Vec::operator+", Num, NUM_TYPES) {
  SUBCASE("normal") {
    CHECK(Vec{Num{1}, Num{2}} + Vec{Num{3}, Num{4}} == Vec{Num{4}, Num{6}});
  }
  SUBCASE("with assignment") {
    Vec v{Num{1}, Num{2}};
    v += Vec{Num{3}, Num{4}};
    CHECK(v == Vec{Num{4}, Num{6}});
  }
}

TEST_CASE_TEMPLATE("Vec::operator-", Num, NUM_TYPES) {
  SUBCASE("negation") {
    CHECK(-Vec{Num{1}, Num{2}} == Vec{-Num{1}, -Num{2}});
  }
  SUBCASE("subtraction") {
    SUBCASE("normal") {
      CHECK(Vec{Num{3}, Num{4}} - Vec{Num{1}, Num{2}} == Vec{Num{2}, Num{2}});
    }
    SUBCASE("with assignment") {
      Vec v{Num{3}, Num{4}};
      v -= Vec{Num{1}, Num{2}};
      CHECK(v == Vec{Num{2}, Num{2}});
    }
  }
}

TEST_CASE_TEMPLATE("Vec::operator*", Num, NUM_TYPES) {
  SUBCASE("scaling") {
    SUBCASE("normal") {
      CHECK(Num{4} * Vec{Num{2}, Num{3}} == Vec{Num{8}, Num{12}});
      CHECK(Vec{Num{2}, Num{3}} * Num{4} == Vec{Num{8}, Num{12}});
    }
    SUBCASE("with assignment") {
      Vec v{Num{2}, Num{3}};
      v *= Num{4};
      CHECK(v == Vec{Num{8}, Num{12}});
    }
  }
  SUBCASE("multiplication") {
    SUBCASE("normal") {
      CHECK(Vec{Num{2}, Num{3}} * Vec{Num{4}, Num{5}} == Vec{Num{8}, Num{15}});
    }
    SUBCASE("with assignment") {
      Vec v{Num{2}, Num{3}};
      v *= Vec{Num{4}, Num{5}};
      CHECK(v == Vec{Num{8}, Num{15}});
    }
  }
}

TEST_CASE_TEMPLATE("Vec::operator/", Num, NUM_TYPES) {
  SUBCASE("scaling") {
    SUBCASE("normal") {
      CHECK(Vec{Num{8}, Num{12}} / Num{4} == Vec{Num{2}, Num{3}});
    }
    SUBCASE("with assignment") {
      Vec v{Num{8}, Num{12}};
      v /= Num{4};
      CHECK(v == Vec{Num{2}, Num{3}});
    }
  }
  SUBCASE("division") {
    SUBCASE("normal") {
      CHECK(Vec{Num{8}, Num{15}} / Vec{Num{2}, Num{3}} == Vec{Num{4}, Num{5}});
    }
    SUBCASE("with assignment") {
      Vec v{Num{8}, Num{15}};
      v /= Vec{Num{2}, Num{3}};
#if defined(__clang__) && defined(NDEBUG) && defined(__SSE__)
      CHECK(approx_equal_to(v, Vec{Num{4}, Num{5}}));
#else
      CHECK(v == Vec{Num{4}, Num{5}});
#endif
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE_TEMPLATE("Vec::operator==", Num, NUM_TYPES) {
  CHECK((Vec{Num{1}, Num{2}} == Vec{Num{1}, Num{3}}) ==
        VecMask<Num, 2>{true, false});
}

TEST_CASE_TEMPLATE("Vec::operator!=", Num, NUM_TYPES) {
  CHECK((Vec{Num{1}, Num{2}} != Vec{Num{1}, Num{3}}) ==
        VecMask<Num, 2>{false, true});
}

TEST_CASE_TEMPLATE("Vec::operator<", Num, NUM_TYPES) {
  CHECK((Vec{Num{1}, Num{2}, Num{3}} < Vec{Num{1}, Num{2}, Num{4}}) ==
        VecMask<Num, 3>{false, false, true});
}

TEST_CASE_TEMPLATE("Vec::operator<=", Num, NUM_TYPES) {
  CHECK((Vec{Num{1}, Num{2}, Num{4}} <= Vec{Num{1}, Num{2}, Num{3}}) ==
        VecMask<Num, 3>{true, true, false});
}

TEST_CASE_TEMPLATE("Vec::operator>", Num, NUM_TYPES) {
  CHECK((Vec{Num{1}, Num{2}, Num{4}} > Vec{Num{1}, Num{2}, Num{3}}) ==
        VecMask<Num, 3>{false, false, true});
}

TEST_CASE_TEMPLATE("Vec::operator>=", Num, NUM_TYPES) {
  CHECK((Vec{Num{1}, Num{2}, Num{3}} >= Vec{Num{1}, Num{2}, Num{4}}) ==
        VecMask<Num, 3>{true, true, false});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE_TEMPLATE("Vec::unit", Num, NUM_TYPES) {
  CHECK(unit(Vec<Num, 2>{}) == Vec{Num{1}, Num{0}});
  CHECK(unit<1>(Vec<Num, 2>{}) == Vec{Num{0}, Num{1}});
}

TEST_CASE_TEMPLATE("Vec::vec_cat", Num, NUM_TYPES) {
  CHECK(vec_cat(Vec{Num{1}, Num{2}}, Vec{Num{3}, Num{4}}) ==
        Vec{Num{1}, Num{2}, Num{3}, Num{4}});
}

TEST_CASE_TEMPLATE("Vec::vec_head", Num, NUM_TYPES) {
  CHECK(vec_head(Vec{Num{1}, Num{2}, Num{3}}) == Vec{Num{1}});
  CHECK(vec_head<2>(Vec{Num{1}, Num{2}, Num{3}}) == Vec{Num{1}, Num{2}});
}

TEST_CASE_TEMPLATE("Vec::vec_tail", Num, NUM_TYPES) {
  CHECK(vec_tail(Vec{Num{1}, Num{2}, Num{3}}) == Vec{Num{2}, Num{3}});
  CHECK(vec_tail<2>(Vec{Num{1}, Num{2}, Num{3}}) == Vec{Num{3}});
}

TEST_CASE_TEMPLATE("Vec::vec_cast<class To>", Num, NUM_TYPES) {
  SUBCASE("<class To>") {
    CHECK(vec_cast<int>(Vec{Num{1}, Num{2}}) == Vec{1, 2});
  }
  SUBCASE("<template<class...> To>") {
    using Decuded = decltype(Tagged{Num{}});
    const Vec<Decuded, 2> r = vec_cast<Tagged>(Vec{Num{1}, Num{2}});
    CHECK(r == Vec{Decuded{1}, Decuded{2}});
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE_TEMPLATE("Vec::minimum", Num, NUM_TYPES) {
  CHECK(minimum(Vec{-Num{3}, +Num{4}}, Vec{+Num{3}, +Num{2}}) ==
        Vec{-Num{3}, +Num{2}});
}

TEST_CASE_TEMPLATE("Vec::maximum", Num, NUM_TYPES) {
  CHECK(maximum(Vec{-Num{3}, +Num{4}}, Vec{+Num{3}, +Num{2}}) ==
        Vec{+Num{3}, +Num{4}});
}

TEST_CASE_TEMPLATE("VecMask::filter", Num, NUM_TYPES) {
  const auto m = Vec{Num{1}, Num{2}} == Vec{Num{3}, Num{2}};
  CHECK(filter(m, Vec{Num{1}, Num{2}}) == Vec{Num{0}, Num{2}});
}

TEST_CASE_TEMPLATE("VecMask::select", Num, NUM_TYPES) {
  const auto m = Vec{Num{1}, Num{2}} == Vec{Num{3}, Num{2}};
  CHECK(select(m, Vec{Num{1}, Num{2}}, Vec{Num{3}, Num{4}}) ==
        Vec{Num{3}, Num{2}});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE_TEMPLATE("Vec::floor", Num, NUM_TYPES) {
  CHECK(floor(Vec{Num{1.5}, Num{2.7}}) == Vec{Num{1}, Num{2}});
}

TEST_CASE_TEMPLATE("Vec::round", Num, NUM_TYPES) {
  CHECK(round(Vec{Num{1.5}, Num{2.7}}) == Vec{Num{2}, Num{3}});
}

TEST_CASE_TEMPLATE("Vec::ceil", Num, NUM_TYPES) {
  CHECK(ceil(Vec{Num{1.5}, Num{2.7}}) == Vec{Num{2}, Num{3}});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE_TEMPLATE("Vec::sum", Num, NUM_TYPES) {
  const auto run = []<size_t... Axes>(std::index_sequence<Axes...> /*axes*/) {
    FSUBCASE("Dim = {}", sizeof...(Axes)) {
      const Vec v{Num{Axes + 1}...};
      CHECK(sum(v) == Num{((Axes + 1) + ...)});
    }
  };
  [&run]<size_t... Dims>(std::index_sequence<Dims...> /*dims*/) {
    (run(std::make_index_sequence<Dims + 1>{}), ...);
  }(std::make_index_sequence<2 * simd::max_reg_size_v<double>>{});
}

// Note: this function does not use SIMD, so we can test it lightly.
TEST_CASE_TEMPLATE("Vec::prod", Num, NUM_TYPES) {
  CHECK(prod(Vec{Num{1}, Num{2}}) == Num{2});
  CHECK(prod(Vec{Num{1}, Num{2}, Num{3}}) == Num{6});
  CHECK(prod(Vec{Num{1}, Num{2}, Num{3}, Num{4}}) == Num{24});
}

TEST_CASE_TEMPLATE("Vec::min_value", Num, NUM_TYPES) {
  const auto run = []<size_t... Axes>(std::index_sequence<Axes...> /*axes*/) {
    FSUBCASE("Dim = {}", sizeof...(Axes)) {
      SUBCASE("all positive") {
        Vec v{Num{Axes + 1}...};
        std::ranges::shuffle(v.elems(), std::mt19937_64{});
        CHECK(min_value(v) == Num{1});
      }
      SUBCASE("all negative") {
        Vec v{Num{-ssize_t{Axes} - 1}...};
        std::ranges::shuffle(v.elems(), std::mt19937_64{});
        CHECK(min_value(v) == Num{-ssize_t{sizeof...(Axes)}});
      }
      SUBCASE("even positive") {
        const Vec v{Num{(Axes % 2 == 0 ? 1 : -1) * ssize_t{Axes}}...};
        CHECK(min_value(v) == Num{*std::ranges::min_element(v.elems())});
      }
      SUBCASE("even negative") {
        const Vec v{Num{(Axes % 2 == 0 ? -1 : 1) * ssize_t{Axes}}...};
        CHECK(min_value(v) == Num{*std::ranges::min_element(v.elems())});
      }
    }
  };
  [&run]<size_t... Dims>(std::index_sequence<Dims...> /*dims*/) {
    (run(std::make_index_sequence<Dims + 1>{}), ...);
  }(std::make_index_sequence<2 * simd::max_reg_size_v<double>>{});
}

TEST_CASE_TEMPLATE("Vec::max_value", Num, NUM_TYPES) {
  const auto run = []<size_t... Axes>(std::index_sequence<Axes...> /*axes*/) {
    FSUBCASE("Dim = {}", sizeof...(Axes)) {
      SUBCASE("all positive") {
        Vec v{Num{Axes + 1}...};
        std::ranges::shuffle(v.elems(), std::mt19937_64{});
        CHECK(max_value(v) == Num{sizeof...(Axes)});
      }
      SUBCASE("all negative") {
        Vec v{Num{-ssize_t{Axes} - 1}...};
        std::ranges::shuffle(v.elems(), std::mt19937_64{});
        CHECK(max_value(v) == Num{-1});
      }
      SUBCASE("even positive") {
        const Vec v{Num{(Axes % 2 == 0 ? 1 : -1) * ssize_t{Axes}}...};
        CHECK(max_value(v) == Num{*std::ranges::max_element(v.elems())});
      }
      SUBCASE("even negative") {
        const Vec v{Num{(Axes % 2 == 0 ? -1 : 1) * ssize_t{Axes}}...};
        CHECK(max_value(v) == Num{*std::ranges::max_element(v.elems())});
      }
    }
  };
  [&run]<size_t... Dims>(std::index_sequence<Dims...> /*dims*/) {
    (run(std::make_index_sequence<Dims + 1>{}), ...);
  }(std::make_index_sequence<2 * simd::max_reg_size_v<double>>{});
}

// Note: these functions do not use SIMD, so we can test them lightly.
TEST_CASE_TEMPLATE("Vec::min_value_index", Num, NUM_TYPES) {
  CHECK(min_value_index(Vec{Num{2}, Num{3}}) == 0);
  CHECK(min_value_index(Vec{Num{3}, Num{2}, Num{4}}) == 1);
  CHECK(min_value_index(Vec{Num{5}, Num{4}, Num{6}, Num{3}}) == 3);
}

TEST_CASE_TEMPLATE("Vec::max_value_index", Num, NUM_TYPES) {
  CHECK(min_value_index(Vec{Num{3}, Num{2}}) == 1);
  CHECK(max_value_index(Vec{Num{3}, Num{2}, Num{4}}) == 2);
  CHECK(max_value_index(Vec{Num{5}, Num{4}, Num{6}, Num{3}}) == 2);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE_TEMPLATE("Vec::dot", Num, NUM_TYPES) {
  const auto run = []<size_t... Axes>(std::index_sequence<Axes...> /*axes*/) {
    FSUBCASE("Dim = {}", sizeof...(Axes)) {
      const Vec v{Num{Axes + 1}...};
      const Vec w{Num{Axes + 2}...};
      CHECK(dot(v, w) == Num{(((Axes + 1) * (Axes + 2)) + ...)});
    }
  };
  [&run]<size_t... Dims>(std::index_sequence<Dims...> /*dims*/) {
    (run(std::make_index_sequence<Dims + 1>{}), ...);
  }(std::make_index_sequence<2 * simd::max_reg_size_v<double>>{});
}

TEST_CASE_TEMPLATE("Vec::norm2", Num, NUM_TYPES) {
  CHECK(norm2(Vec{Num{3}, Num{4}}) == Num{25});
  CHECK(norm2(Vec{Num{2}, Num{10}, Num{11}}) == Num{225});
}

TEST_CASE_TEMPLATE("Vec::norm", Num, NUM_TYPES) {
  CHECK(norm(Vec{-Num{3}}) == Num{3});
  CHECK_APPROX_EQ(norm(Vec{Num{3}, Num{4}}), Num{5});
  CHECK_APPROX_EQ(norm(Vec{Num{2}, Num{10}, Num{11}}), Num{15});
}

TEST_CASE_TEMPLATE("Vec::normalize", Num, NUM_TYPES) {
  CHECK(normalize(Vec{Num{0}}) == Vec{Num{0}});
  CHECK(normalize(Vec{-Num{3}}) == Vec{-Num{1}});
  CHECK(normalize(Vec{Num{0}, Num{0}}) == Vec{Num{0}, Num{0}});
  CHECK_APPROX_EQ(normalize(Vec{Num{3}, Num{4}}), Vec{Num{0.6}, Num{0.8}});
}

TEST_CASE_TEMPLATE("Vec::approx_equal_to", Num, NUM_TYPES) {
  CHECK_APPROX_EQ(Vec{Num{1}, Num{2}}, Vec{Num{1}, Num{2}});
  CHECK_APPROX_NE(Vec{Num{1}, Num{2}}, Vec{Num{1}, Num{3}});
}

TEST_CASE_TEMPLATE("Vec::cross", Num, NUM_TYPES) {
  CHECK(cross(Vec{Num{1}, Num{0}, Num{0}}, Vec{Num{0}, Num{1}, Num{0}}) ==
        Vec{Num{0}, Num{0}, Num{1}});
  CHECK(cross(Vec{Num{1}, Num{2}, Num{3}}, Vec{Num{4}, Num{5}, Num{6}}) ==
        Vec{-Num{3}, Num{6}, -Num{3}});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("Vec::serialize") {
  const Vec<float32_t, 3> vec{1.0, 2.0, 3.0};
  testing::test_serialization(vec, 3 * sizeof(float32_t));
}

TEST_CASE("Vec::format") {
  CHECK(std::format("{}", Vec{1, 2, 3}) == "1 2 3");
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
