/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <cmath>
#include <concepts> // IWYU pragma: keep
#include <functional>
#include <limits>
#include <numbers>
#include <tuple>
#include <utility>

#include <doctest/doctest.h>

#include "tit/core/math_utils.hpp"
#include "tit/core/types.hpp"

namespace tit {
namespace {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** A wrapper for a function with call counter. */
template<class Func>
class CountedFunc {
public:

  /* Initialize a wrapper with a specified function. */
  constexpr explicit CountedFunc(Func func) noexcept : func_(std::move(func)) {}

  /** Call the function and increase the call counter. */
  template<class... Args>
    requires std::invocable<Func, Args&&...>
  constexpr auto operator()(Args&&... args) {
    count_ += 1;
    return std::invoke(func_, std::forward<Args>(args)...);
  }

  /** Call count. */
  constexpr auto count() const noexcept {
    return count_;
  }

private:

  Func func_;
  size_t count_ = 0;

}; // class CountedFunc

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

// Numerical (signed) types to test against.
#define NUM_TYPES int, float, double

// Floating-point types to test against.
#define REAL_TYPES float, double

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

TEST_CASE_TEMPLATE("tit::abs", Num, NUM_TYPES) {
  CHECK(abs(Num{0}) == Num{0});
  CHECK(abs(+Num{2}) == Num{2});
  CHECK(abs(-Num{2}) == Num{2});
}

TEST_CASE_TEMPLATE("tit::plus", Num, NUM_TYPES) {
  CHECK(plus(Num{0}) == Num{0});
  CHECK(plus(+Num{2}) == Num{2});
  CHECK(plus(-Num{2}) == Num{0});
}

TEST_CASE_TEMPLATE("tit::minus", Num, NUM_TYPES) {
  CHECK(minus(Num{0}) == Num{0});
  CHECK(minus(+Num{2}) == Num{0});
  CHECK(minus(-Num{2}) == -Num{2});
}

TEST_CASE_TEMPLATE("tit::sign", Num, NUM_TYPES) {
  CHECK(sign(Num{0}) == Num{0});
  CHECK(sign(+Num{2}) == +Num{1});
  CHECK(sign(-Num{2}) == -Num{1});
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

TEST_CASE_TEMPLATE("tit::small_number_v", Real, REAL_TYPES) {
  // Small number must be positive.
  CHECK(small_number_v<Real> > 0.0);
  // Small number should be larger than machine epsilon.
  CHECK(small_number_v<Real> >= std::numeric_limits<Real>::epsilon());
}

TEST_CASE_TEMPLATE("tit::is_zero", Real, REAL_TYPES) {
  // Check ordinary numbers.
  CHECK(is_zero(+Real{0.0}));
  CHECK(is_zero(-Real{0.0}));
  CHECK(!is_zero(+Real{1.0}));
  CHECK(!is_zero(-Real{1.0}));
  // Check if comparisons with `small_number_v` work as expected.
  CHECK(is_zero(+small_number_v<Real>));
  CHECK(is_zero(-small_number_v<Real>));
  CHECK(is_zero(+Real{0.1} * small_number_v<Real>));
  CHECK(is_zero(-Real{0.1} * small_number_v<Real>));
  CHECK(!is_zero(+Real{2.0} * small_number_v<Real>));
  CHECK(!is_zero(-Real{2.0} * small_number_v<Real>));
}

TEST_CASE_TEMPLATE("tit::approx_eq", Real, REAL_TYPES) {
  // Check ordinary numbers.
  CHECK(approx_eq(Real{1.23}, Real{1.23}));
  CHECK(!approx_eq(Real{1.23}, Real{5.67}));
  // Check if comparisons with `small_number_v` work as expected.
  CHECK(approx_eq(Real{1.23}, Real{1.23} + small_number_v<Real>));
  CHECK(approx_eq(Real{1.23} - small_number_v<Real>, Real{1.23}));
  CHECK(approx_eq(Real{1.23}, Real{1.23} + Real{0.1} * small_number_v<Real>));
  CHECK(approx_eq(Real{1.23} - Real{0.1} * small_number_v<Real>, Real{1.23}));
  CHECK(!approx_eq(Real{1.23}, Real{1.23} + Real{2.0} * small_number_v<Real>));
  CHECK(!approx_eq(Real{1.23} - Real{2.0} * small_number_v<Real>, Real{1.23}));
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

TEST_CASE_TEMPLATE("tit::floor", Real, REAL_TYPES) {
  // Check non-negative numbers.
  CHECK(floor(Real{0.0}) == Real{0.0});
  CHECK(floor(Real{1.1}) == Real{1.0});
  CHECK(floor(Real{1.5}) == Real{1.0});
  CHECK(floor(Real{1.9}) == Real{1.0});
  // Check negative numbers.
  CHECK(floor(-Real{1.1}) == -Real{2.0});
  CHECK(floor(-Real{1.5}) == -Real{2.0});
  CHECK(floor(-Real{1.9}) == -Real{2.0});
}

TEST_CASE_TEMPLATE("tit::round", Real, REAL_TYPES) {
  // Check non-negative numbers.
  CHECK(round(Real{0.0}) == Real{0.0});
  CHECK(round(Real{1.1}) == Real{1.0});
  CHECK(round(Real{1.5}) == Real{2.0});
  CHECK(round(Real{1.9}) == Real{2.0});
  // Check negative numbers.
  CHECK(round(-Real{1.1}) == -Real{1.0});
  CHECK(round(-Real{1.5}) == -Real{2.0});
  CHECK(round(-Real{1.9}) == -Real{2.0});
}

TEST_CASE_TEMPLATE("tit::ceil", Real, REAL_TYPES) {
  // Check non-negative numbers.
  CHECK(ceil(Real{0.0}) == Real{0.0});
  CHECK(ceil(Real{1.1}) == Real{2.0});
  CHECK(ceil(Real{1.5}) == Real{2.0});
  CHECK(ceil(Real{1.9}) == Real{2.0});
  // Check negative numbers.
  CHECK(ceil(-Real{1.1}) == -Real{1.0});
  CHECK(ceil(-Real{1.5}) == -Real{1.0});
  CHECK(ceil(-Real{1.9}) == -Real{1.0});
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

TEST_CASE_TEMPLATE("tit::inverse", Num, NUM_TYPES) {
  CHECK(inverse(Num{2}) == 0.5);
  CHECK(inverse(Num{8}) == 0.125);
}

TEST_CASE_TEMPLATE("tit::divide", Num, NUM_TYPES) {
  // Note: the result is always floating-point.
  CHECK(divide(Num{1}, 2) == 0.5);
  CHECK(divide(Num{1}, 2.0F) == 0.5);
  CHECK(divide(Num{1}, 2.0) == 0.5);
}

TEST_CASE_TEMPLATE("tit::safe_inverse", Real, REAL_TYPES) {
  // Check non-"small" numbers.
  CHECK(safe_inverse(Real{2.0}) == Real{0.5});
  CHECK(safe_inverse(Real{10.0}) == Real{0.1});
  // Check "small" numbers.
  CHECK(safe_inverse(Real{0.0}) == Real{0.0});
  CHECK(safe_inverse(small_number_v<Real>) == 0.0);
  CHECK(safe_inverse(Real{0.1} * small_number_v<Real>) == Real{0.0});
  CHECK(safe_inverse(Real{2.0} * small_number_v<Real>) != Real{0.0});
}

TEST_CASE_TEMPLATE("tit::safe_divide", Real, REAL_TYPES) {
  // Check non-"small" divisors.
  CHECK(safe_divide(1, Real{2.0}) == Real{0.5});
  CHECK(safe_divide(1, Real{10.0}) == Real{0.1});
  // Check "small" divisors.
  CHECK(safe_divide(1, Real{0.0}) == Real{0.0});
  CHECK(safe_divide(1, small_number_v<Real>) == Real{0.0});
  CHECK(safe_divide(1, Real{0.1} * small_number_v<Real>) == Real{0.0});
  CHECK(safe_divide(1, Real{2.0} * small_number_v<Real>) != Real{0.0});
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

TEST_CASE_TEMPLATE("tit::pow", Num, NUM_TYPES) {
  CHECK(pow2(+Num{2}) == +Num{4});
  CHECK(pow2(-Num{2}) == +Num{4});
  CHECK(pow3(+Num{2}) == +Num{8});
  CHECK(pow3(-Num{2}) == -Num{8});
  CHECK(pow4(+Num{2}) == +Num{16});
  CHECK(pow4(-Num{2}) == +Num{16});
  CHECK(pow5(+Num{2}) == +Num{32});
  CHECK(pow5(-Num{2}) == -Num{32});
  CHECK(pow6(+Num{2}) == +Num{64});
  CHECK(pow6(-Num{2}) == +Num{64});
  CHECK(pow7(+Num{2}) == +Num{128});
  CHECK(pow7(-Num{2}) == -Num{128});
  CHECK(pow8(+Num{2}) == +Num{256});
  CHECK(pow8(-Num{2}) == +Num{256});
  CHECK(pow9(+Num{2}) == +Num{512});
  CHECK(pow9(-Num{2}) == -Num{512});
  CHECK(pow(+Num{2}, 10) == Num{1024});
  CHECK(pow(-Num{2}, 10) == Num{1024});
}

TEST_CASE_TEMPLATE("tit::horner", Num, NUM_TYPES) {
  CHECK(horner(Num{1}, {Num{1}}) == Num{1});
  CHECK(horner(Num{3}, {Num{1}, -Num{3}, Num{2}}) == Num{10});
  CHECK(horner(-Num{2}, {Num{4}, -Num{1}, Num{3}}) == Num{18});
  CHECK(horner(Num{3}, {Num{6}, Num{1}, -Num{4}, Num{1}}) == Num{0});
}

TEST_CASE_TEMPLATE("tit::sqrt", Real, REAL_TYPES) {
  CHECK(sqrt(Real{0.0}) == Real{0.0});
  CHECK(sqrt(Real{4.0}) == Real{2.0});
}

TEST_CASE_TEMPLATE("tit::cbrt", Real, REAL_TYPES) {
  CHECK(cbrt(Real{0.0}) == Real{0.0});
  CHECK(cbrt(+Real{8.0}) == +Real{2.0});
  CHECK(cbrt(-Real{8.0}) == -Real{2.0});
}

TEST_CASE_TEMPLATE("tit::hypot", Real, REAL_TYPES) {
  CHECK(hypot(Real{3.0}, Real{4.0}) == Real{5.0});
  CHECK(hypot(Real{2.0}, Real{6.0}, Real{9.0}) == Real{11.0});
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

TEST_CASE_TEMPLATE("tit::avg", Num, NUM_TYPES) {
  // Note: the result is always floating-point.
  CHECK(avg(Num{1}, Num{2}) == 1.5);
  CHECK(avg(Num{1}, Num{2}, Num{3}) == 2.0);
}

TEST_CASE_TEMPLATE("tit::gavg", Real, REAL_TYPES) {
  CHECK(havg(Real{1.0}, Real{4.0}) == Real{1.6});
}

TEST_CASE_TEMPLATE("tit::gavg", Real, REAL_TYPES) {
  CHECK(gavg(Real{1.0}, Real{4.0}) == Real{2.0});
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

TEST_CASE_TEMPLATE("tit::merge", Real, REAL_TYPES) {
  CHECK(merge(true, Real{2.0}) == Real{2.0});
  CHECK(merge(true, Real{2.0}, Real{3.0}) == Real{2.0});
  CHECK(merge(false, Real{2.0}) == Real{0.0});
  CHECK(merge(false, Real{2.0}, Real{3.0}) == Real{3.0});
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

TEST_CASE_TEMPLATE("tit::newton_raphson", Real, REAL_TYPES) {
  using enum NewtonRaphsonStatus;
  SUBCASE("quadratic") {
    SUBCASE("success") {
      // Ensure the solver works for basic functions.
      auto x = Real{1.0};
      const auto f = [&x] {
        return std::tuple{pow2(x) - Real{4.0}, Real{2.0} * x};
      };
      constexpr auto root = Real{2.0};
      CHECK(newton_raphson(x, f) == success);
      CHECK(approx_eq(x, root));
    }
    SUBCASE("failure_max_iter") {
      // Ensure the solver fails after the iteration limit is exceeded
      // if no actual root can be found.
      auto x = Real{1.0};
      const auto f = [&x] {
        return std::tuple{pow2(x) + Real{4.0}, Real{2.0} * x};
      };
      CHECK(newton_raphson(x, f) == failure_max_iter);
    }
  }
  SUBCASE("cubic") {
    SUBCASE("failure_zero_derivative") {
      // Ensure the solver fails if the zero derivative was reached during
      // the computations.
      auto x = Real{2.0};
      const auto f = [&x] {
        return std::tuple{pow3(x) - Real{12.0} * x + Real{2.0},
                          Real{3.0} * pow2(x) - Real{12.0}};
      };
      CHECK(newton_raphson(x, f) == failure_zero_deriv);
    }
  }
}

TEST_CASE_TEMPLATE("tit::bisection", Real, REAL_TYPES) {
  using enum BisectionStatus;
  SUBCASE("quadratic") {
    constexpr auto root = Real{2.0};
    const auto f = [](Real x) { return pow2(x) - pow2(root); };
    SUBCASE("success") {
      // Ensure the solver works for basic functions.
      auto min_x = Real{1.5}, max_x = Real{3.5};
      CHECK(bisection(min_x, max_x, f) == success);
      CHECK(approx_eq(min_x, root));
      CHECK(approx_eq(max_x, root));
    }
    SUBCASE("success_early_min") {
      // Ensure the solver completes with a single function evaluation if the
      // root is already located on the left side of the search interval.
      auto min_x = Real{2.0}, max_x = Real{4.0};
      auto counted_f = CountedFunc{f};
      CHECK(bisection(min_x, max_x, counted_f) == success);
      CHECK(approx_eq(min_x, root));
      CHECK(approx_eq(max_x, root));
      CHECK(counted_f.count() == 1);
    }
    SUBCASE("success_early_max") {
      // Ensure the solver completes with two function evaluations if the root
      // is already located on the right side of the search interval.
      auto min_x = Real{0.0}, max_x = Real{2.0};
      auto counted_f = CountedFunc{f};
      CHECK(bisection(min_x, max_x, counted_f) == success);
      CHECK(approx_eq(min_x, root));
      CHECK(approx_eq(max_x, root));
      CHECK(counted_f.count() == 2);
    }
    SUBCASE("failure_sign") {
      // Ensure the solver terminates if function values on the ends of the
      // search interval have same signs.
      auto min_x = Real{2.5}, max_x = Real{5.5};
      CHECK(bisection(min_x, max_x, f) == failure_sign);
    }
  }
  SUBCASE("sin") {
    SUBCASE("success") {
      // Ensure the solver works for a bit more complex functions.
      const auto f = [](Real x) { return std::sin(x) + Real{0.5}; };
      const auto root = Real{7.0} * std::numbers::pi_v<Real> / Real{6.0};
      auto min_x = Real{1.0}, max_x = Real{4.0};
      CHECK(bisection(min_x, max_x, f) == success);
      CHECK(approx_eq(min_x, root));
      CHECK(approx_eq(max_x, root));
    }
    SUBCASE("failure_max_iter") {
      // Ensure the solver fails after the iteration limit is exceeded
      // if no actual root can be found. This case requires 23 iterations for
      // `float` and 72 for `double` to complete, while the default iteration
      // limit is 10.
      const auto f = [](Real x) { return std::sin(x) - inverse(x); };
      auto min_x = Real{0.1}, max_x = Real{1.2};
      CHECK(bisection(min_x, max_x, f) == failure_max_iter);
    }
  }
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace
} // namespace tit
