/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <array>
#include <cstddef>
#include <tuple>
#include <utility>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/mat.hpp"
#include "tit/core/serialization.hpp"
#include "tit/core/stream.hpp"
#include "tit/core/vec.hpp"
#include "tit/testing/test.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Test serialization of a type.
template<class T>
void test_serialization(const T& input, size_t expected_size) {
  std::vector<std::byte> bytes{};
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

TEST_CASE("serialize<trivial-type>") {
  test_serialization(1, sizeof(int32_t));
  test_serialization(1.0, sizeof(float64_t));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("serialize<std::pair<...>>") {
  test_serialization(std::pair{1, 2.0}, sizeof(int32_t) + sizeof(float64_t));
}

TEST_CASE("serialize<std::tuple<...>>") {
  test_serialization(std::tuple{1, 2.0, 3.0F},
                     sizeof(int32_t) + sizeof(float64_t) + sizeof(float32_t));
}

TEST_CASE("serialize<std::array<...>>") {
  test_serialization(std::array{1, 2, 3}, sizeof(int32_t) * 3);
}

TEST_CASE("serialize<Vec<...>>") {
  const Vec<float32_t, 3> vec{1.0, 2.0, 3.0};
  test_serialization(vec, 3 * sizeof(float32_t));
}

TEST_CASE("serialize<Mat<...>>") {
  const Mat<float32_t, 3> mat{
      {1.0, 2.0, 3.0},
      {4.0, 5.0, 6.0},
      {7.0, 8.0, 9.0},
  };
  test_serialization(mat, 9 * sizeof(float32_t));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("StreamSerializer") {
  std::vector<std::byte> bytes{};
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
