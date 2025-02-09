/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <array>
#include <tuple>
#include <utility>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/serialization.hpp"
#include "tit/core/stream.hpp"

#include "tit/core/serialization.testing.hpp"
#include "tit/testing/test.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("serialize<trivial-type>") {
  testing::test_serialization(1, sizeof(int32_t));
  testing::test_serialization(1.0, sizeof(float64_t));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("serialize<std::pair<...>>") {
  testing::test_serialization(std::pair{1, 2.0},
                              sizeof(int32_t) + sizeof(float64_t));
}

TEST_CASE("serialize<std::tuple<...>>") {
  testing::test_serialization( //
      std::tuple{1, 2.0, 3.0F},
      sizeof(int32_t) + sizeof(float64_t) + sizeof(float32_t));
}

TEST_CASE("serialize<std::array<...>>") {
  testing::test_serialization(std::array{1, 2, 3}, sizeof(int32_t) * 3);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("StreamSerializer") {
  std::vector<byte_t> bytes{};
  make_stream_serializer<int32_t>(make_container_output_stream(bytes))
      ->write(std::array{1, 2, 3});

  std::vector<int32_t> result(10);
  SUBCASE("full") {
    CHECK(make_stream_deserializer<int32_t>(make_range_input_stream(bytes))
              ->read(result) == 3);
    CHECK(result >= std::vector{1, 2, 3});
  }
  SUBCASE("truncated") {
    bytes.pop_back();
    CHECK_THROWS_MSG(
        make_stream_deserializer<int32_t>(make_range_input_stream(bytes))
            ->read(result),
        Exception,
        "truncated stream");
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
