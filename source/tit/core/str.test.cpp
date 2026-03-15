/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "tit/core/str.hpp"
#include "tit/testing/test.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("str_nocase_equal") {
  SUBCASE("empty") {
    CHECK(str_nocase_equal("", ""));
  }
  SUBCASE("single character") {
    CHECK(str_nocase_equal("a", "a"));
    CHECK(str_nocase_equal("a", "A"));
    CHECK_FALSE(str_nocase_equal("a", "b"));
  }
  SUBCASE("multiple characters") {
    CHECK(str_nocase_equal("aBc", "AbC"));
    CHECK_FALSE(str_nocase_equal("aBc", "AcC"));
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("str_to") {
  SUBCASE("int") {
    SUBCASE("valid") {
      CHECK(str_to<int>("123") == 123);
      CHECK(str_to<int>("-123") == -123);
    }
    SUBCASE("invalid") {
      CHECK_FALSE(str_to<int>("123abc").has_value());
      CHECK_FALSE(str_to<int>("not an integer").has_value());
    }
  }
  SUBCASE("bool") {
    SUBCASE("literals") {
      CHECK(str_to<bool>("true").value_or(false));
      CHECK(str_to<bool>("True").value_or(false));
      CHECK(str_to<bool>("TRUE").value_or(false));
      CHECK_FALSE(str_to<bool>("false").value_or(false));
      CHECK_FALSE(str_to<bool>("False").value_or(false));
      CHECK_FALSE(str_to<bool>("FALSE").value_or(false));
    }
    SUBCASE("integer values") {
      CHECK(str_to<bool>("1").value_or(false));
      CHECK(str_to<bool>("2").value_or(false));
      CHECK(str_to<bool>("-1").value_or(false));
      CHECK_FALSE(str_to<bool>("0").value_or(false));
    }
    SUBCASE("invalid") {
      CHECK_FALSE(str_to<bool>("trueee").has_value());
      CHECK_FALSE(str_to<bool>("not a bool").has_value());
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("str_quoted") {
  SUBCASE("simple") {
    CHECK(str_quoted("hello") == "\"hello\"");
  }
  SUBCASE("escapes quotes") {
    CHECK(str_quoted("he\"llo") == "\"he\\\"llo\"");
  }
  SUBCASE("escapes backslashes") {
    CHECK(str_quoted("path\\name") == "\"path\\\\name\"");
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("str_is_identifier") {
  SUBCASE("valid") {
    CHECK(str_is_identifier("foo"));
    CHECK(str_is_identifier("_bar"));
    CHECK(str_is_identifier("foo_1"));
    CHECK(str_is_identifier("_"));
  }
  SUBCASE("invalid") {
    CHECK_FALSE(str_is_identifier(""));
    CHECK_FALSE(str_is_identifier("1foo"));
    CHECK_FALSE(str_is_identifier("foo-bar"));
    CHECK_FALSE(str_is_identifier("foo.bar"));
    CHECK_FALSE(str_is_identifier("foo bar"));
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("str_split") {
  SUBCASE("basic split") {
    CHECK_RANGE_EQ(str_split("a.b.c", '.'), {"a", "b", "c"});
  }
  SUBCASE("no delimiter") {
    CHECK_RANGE_EQ(str_split("hello", '.'), {"hello"});
  }
  SUBCASE("empty parts") {
    CHECK_RANGE_EQ(str_split("..", '.'), {"", "", ""});
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("str_join") {
  SUBCASE("multiple parts") {
    CHECK(str_join({"a", "b", "c"}, ",") == "a,b,c");
  }
  SUBCASE("empty range") {
    CHECK(str_join({}, ",") == "");
  }
  SUBCASE("single part") {
    CHECK(str_join({"only"}, ",") == "only");
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("str_join_nonempty") {
  SUBCASE("filters empty parts") {
    CHECK(str_join_nonempty({"a", "", "c"}, ",") == "a,c");
  }
  SUBCASE("all empty") {
    CHECK(str_join_nonempty({"", ""}, ",") == "");
  }
  SUBCASE("no empty parts") {
    CHECK(str_join_nonempty({"x", "y"}, ".") == "x.y");
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("fmt_real") {
  SUBCASE("whole number keeps decimal") {
    CHECK(fmt_real(1.0) == "1.0");
    CHECK(fmt_real(2.0) == "2.0");
  }
  SUBCASE("fractional number unchanged") {
    CHECK(fmt_real(1.5) == "1.5");
  }
  SUBCASE("scientific notation preserved") {
    CHECK(fmt_real(1e20).contains('e'));
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("fmt_memsize") {
  SUBCASE("zero") {
    CHECK(fmt_memsize(0) == "0 bytes");
  }
  SUBCASE("bytes") {
    CHECK(fmt_memsize(1) == "1.0 bytes");
    CHECK(fmt_memsize(1023) == "1023.0 bytes");
  }
  SUBCASE("kibibytes") {
    CHECK(fmt_memsize(1024) == "1.0 KiB");
    CHECK(fmt_memsize(1536) == "1.5 KiB");
  }
  SUBCASE("mebibytes") {
    CHECK(fmt_memsize(1024ULL * 1024) == "1.0 MiB");
  }
  SUBCASE("gibibytes") {
    CHECK(fmt_memsize(1024ULL * 1024 * 1024) == "1.0 GiB");
  }
  SUBCASE("precision") {
    CHECK(fmt_memsize(1536, 2) == "1.50 KiB");
    CHECK(fmt_memsize(1536, 0) == "2 KiB");
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("fmt_quantity") {
  SUBCASE("zero") {
    CHECK(fmt_quantity(0.0L, "s") == "0 s");
  }
  SUBCASE("base unit") {
    CHECK(fmt_quantity(1.0L, "s") == "1.0 s");
    CHECK(fmt_quantity(999.0L, "s") == "999.0 s");
  }
  SUBCASE("milli") {
    CHECK(fmt_quantity(0.001L, "s") == "1.0 ms");
  }
  SUBCASE("kilo") {
    CHECK(fmt_quantity(1000.0L, "s") == "1.0 ks");
  }
  SUBCASE("negative") {
    CHECK(fmt_quantity(-1.0L, "s") == "-1.0 s");
    CHECK(fmt_quantity(-0.001L, "s") == "-1.0 ms");
  }
  SUBCASE("precision") {
    CHECK(fmt_quantity(1500.0L, "s", 2) == "1.50 ks");
    CHECK(fmt_quantity(1500.0L, "s", 0) == "2 ks");
  }
  SUBCASE("template overload") {
    CHECK(fmt_quantity(1, "m") == "1.0 m");
    CHECK(fmt_quantity(1.0F, "m") == "1.0 m");
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
