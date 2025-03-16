/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "tit/core/basic_types.hpp"
#include "tit/core/enum_utils.hpp"

#include "tit/testing/test.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

enum class Flags : uint8_t {
  flag_1 = 1 << 0,
  flag_2 = 1 << 1,
  flag_3 = 1 << 2,
  flag_12 = flag_1 | flag_2,
  flag_23 = flag_2 | flag_3,
  flag_123 = flag_1 | flag_2 | flag_3,
};

template<>
inline constexpr bool is_flags_enum_v<Flags> = true;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("Flags::operator|") {
  using enum Flags;
  STATIC_CHECK((flag_1 | flag_1) == flag_1);
  STATIC_CHECK((flag_1 | flag_2) == flag_12);
  STATIC_CHECK((flag_12 | flag_23) == flag_123);
}

TEST_CASE("Flags::operator&") {
  using enum Flags;
  STATIC_CHECK((flag_12 & flag_1));
  STATIC_CHECK((flag_12 & flag_2));
  STATIC_CHECK((flag_12 & flag_23));
  STATIC_CHECK_FALSE((flag_12 & flag_3));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
