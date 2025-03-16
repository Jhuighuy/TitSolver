/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/serialization.hpp"
#include "tit/core/stream.hpp"

#include "tit/testing/test.hpp"

namespace tit::testing {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Test serialization of a type.
template<class T>
void test_serialization(const T& input, size_t expected_size) {
  std::vector<byte_t> bytes{};
  serialize(*make_container_output_stream(bytes), input);
  CHECK(bytes.size() == expected_size);
  SUBCASE("success") {
    T output{};
    deserialize(*make_range_input_stream(bytes), output);
    CHECK(input == output);
  }
  SUBCASE("failure") {
    bytes.pop_back();
    T output{};
    CHECK_THROWS_MSG(deserialize(*make_range_input_stream(bytes), output),
                     Exception,
                     "truncated stream");
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::testing
