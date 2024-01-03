/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <source_location>
#include <string_view>

#include <doctest/doctest.h>

#include "tit/core/exception.hpp"
#include "tit/core/stacktrace.hpp"
#include "tit/core/types.hpp"

namespace tit {
namespace {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

// NOLINTBEGIN(*-avoid-non-const-global-variables)
constexpr std::string_view thrown_message = "Blah-blah!";
std::source_location thrown_location{};
Std::stacktrace thrown_stacktrace{};
// NOLINTEND(*-avoid-non-const-global-variables)

[[gnu::noinline]] void my_function_3() {
  // This macro trick ensures the source location is recorded properly.
#define IMPL()                                                                 \
  thrown_location = std::source_location::current();                           \
  thrown_stacktrace = Std::stacktrace::current();                              \
  throw Exception(thrown_message);
  IMPL();
#undef IMPL
}

[[gnu::noinline]] void my_function_2() {
  my_function_3();
}

[[gnu::noinline]] void my_function_1() {
  my_function_2();
}

TEST_CASE("tit::Exception") {
  try {
    // Call a chain of functions that end with an exception.
    my_function_1();
  } catch (const Exception& e) {
    // Check that the exception's message matches.
    CHECK(e.what() == thrown_message);
    // Check that the exception's source location matches.
    CHECK(e.where().line() == thrown_location.line());
    CHECK(e.where().column() == thrown_location.column());
    CHECK(e.where().file_name() == thrown_location.file_name());
    CHECK(e.where().function_name() == thrown_location.function_name());
    // Check that the exception's stack trace matches.
    // TODO: maybe this will work with `std::stacktrace`.
    // CHECK(e.when() == thrown_stacktrace);
    REQUIRE(e.when().size() == thrown_stacktrace.size());
    for (size_t i = 0; i < e.when().size(); ++i) {
      CHECK(e.when()[i].source_file() == thrown_stacktrace[i].source_file());
      CHECK(e.when()[i].source_line() == thrown_stacktrace[i].source_line());
    }
  }
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace
} // namespace tit
