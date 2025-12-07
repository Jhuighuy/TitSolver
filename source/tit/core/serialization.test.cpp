/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
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

TEST_CASE("encode_base64") {
  SUBCASE("empty") {
    CHECK(encode_base64(std::vector<std::byte>{}).empty());
  }
  SUBCASE("one byte") {
    std::vector<std::byte> data{std::byte{0x4D}};
    CHECK(encode_base64(data) == "TQ==");
  }
  SUBCASE("two bytes") {
    std::vector<std::byte> data{std::byte{0x4D}, std::byte{0x61}};
    CHECK(encode_base64(data) == "TWE=");
  }
  SUBCASE("three bytes") {
    std::vector<std::byte> data{std::byte{0x4D},
                                std::byte{0x61},
                                std::byte{0x6E}};
    CHECK(encode_base64(data) == "TWFu");
  }
  SUBCASE("multiple blocks") {
    std::vector<std::byte> data{std::byte{0x48},
                                std::byte{0x65},
                                std::byte{0x6C},
                                std::byte{0x6C},
                                std::byte{0x6F}};
    CHECK(encode_base64(data) == "SGVsbG8=");
  }
}

TEST_CASE("decode_base64") {
  SUBCASE("success") {
    SUBCASE("empty") {
      CHECK(decode_base64("").empty());
    }
    SUBCASE("one byte with padding") {
      const auto result = decode_base64("TQ==");
      CHECK_RANGE_EQ(result, {std::byte{0x4D}});
    }
    SUBCASE("two bytes with padding") {
      const auto result = decode_base64("TWE=");
      CHECK_RANGE_EQ(result, {std::byte{0x4D}, std::byte{0x61}});
    }
    SUBCASE("three bytes no padding") {
      const auto result = decode_base64("TWFu");
      CHECK_RANGE_EQ(result,
                     {std::byte{0x4D}, std::byte{0x61}, std::byte{0x6E}});
    }
    SUBCASE("multiple blocks") {
      const auto result = decode_base64("SGVsbG8=");
      CHECK_RANGE_EQ(result,
                     {std::byte{0x48},
                      std::byte{0x65},
                      std::byte{0x6C},
                      std::byte{0x6C},
                      std::byte{0x6F}});
    }
    SUBCASE("roundtrip") {
      const std::vector<std::byte> data{std::byte{0x00},
                                        std::byte{0xFF},
                                        std::byte{0xAB},
                                        std::byte{0xCD},
                                        std::byte{0xEF}};
      CHECK_RANGE_EQ(decode_base64(encode_base64(data)), data);
    }
  }
  SUBCASE("failure") {
    SUBCASE("invalid length") {
      CHECK_THROWS_MSG(decode_base64("ABC"),
                       Exception,
                       "Invalid Base64 string length '3'.");
    }
    SUBCASE("invalid character") {
      CHECK_THROWS_MSG(decode_base64("TW@="),
                       Exception,
                       "Invalid Base64 character '@'.");
    }
  }
}

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

TEST_CASE("serialize<trivial-type>") {
  test_serialization(1, sizeof(int32_t));
  test_serialization(1.0, sizeof(float64_t));
}

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
