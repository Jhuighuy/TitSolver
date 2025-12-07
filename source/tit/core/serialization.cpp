/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <array>
#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/serialization.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto encode_base64(std::span<const std::byte> data) -> std::string {
  static constexpr std::string_view alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                               "abcdefghijklmnopqrstuvwxyz"
                                               "0123456789+/";

  const size_t length = data.size();

  std::string result;
  result.reserve(((length + 2) / 3) * 4);

  size_t i = 0;
  for (; i + 3 <= length; i += 3) {
    const auto triple = (uint32_t(std::to_integer<uint8_t>(data[i])) << 16) |
                        (uint32_t(std::to_integer<uint8_t>(data[i + 1])) << 8) |
                        (uint32_t(std::to_integer<uint8_t>(data[i + 2])));

    result.push_back(alphabet[(triple >> 18) & 0x3F]);
    result.push_back(alphabet[(triple >> 12) & 0x3F]);
    result.push_back(alphabet[(triple >> 6) & 0x3F]);
    result.push_back(alphabet[triple & 0x3F]);
  }
  if (i + 1 == length) {
    const auto triple = (uint32_t(std::to_integer<uint8_t>(data[i])) << 16);

    result.push_back(alphabet[(triple >> 18) & 0x3F]);
    result.push_back(alphabet[(triple >> 12) & 0x3F]);
    result.push_back('=');
    result.push_back('=');
  } else if (i + 2 == length) {
    const auto triple = (uint32_t(std::to_integer<uint8_t>(data[i])) << 16) |
                        (uint32_t(std::to_integer<uint8_t>(data[i + 1])) << 8);

    result.push_back(alphabet[(triple >> 18) & 0x3F]);
    result.push_back(alphabet[(triple >> 12) & 0x3F]);
    result.push_back(alphabet[(triple >> 6) & 0x3F]);
    result.push_back('=');
  }

  return result;
}

auto decode_base64(std::string_view data) -> std::vector<std::byte> {
  static constexpr auto decode_table = std::to_array<uint8_t>({
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 62,  255,
      255, 255, 63,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  255, 255,
      255, 0,   255, 255, 255, 0,   1,   2,   3,   4,   5,   6,   7,   8,   9,
      10,  11,  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,
      25,  255, 255, 255, 255, 255, 255, 26,  27,  28,  29,  30,  31,  32,  33,
      34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48,
      49,  50,  51,  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255,
  });

  static constexpr auto decode = [](char c) {
    const auto result = decode_table[static_cast<uint8_t>(c)];
    TIT_ENSURE(result != 255, "Invalid Base64 character '{}'.", c);
    return static_cast<uint32_t>(result);
  };

  const auto length = data.size();
  TIT_ENSURE(length % 4 == 0, "Invalid Base64 string length '{}'.", length);

  size_t padding = 0;
  if (length >= 1 && data[length - 1] == '=') padding++;
  if (length >= 2 && data[length - 2] == '=') padding++;

  std::vector<std::byte> result;
  result.reserve((length / 4) * 3 - padding);

  for (size_t i = 0; i < length; i += 4) {
    uint32_t triple = (decode(data[i]) << 18) | (decode(data[i + 1]) << 12);
    if (data[i + 2] != '=') triple |= (decode(data[i + 2]) << 6);
    if (data[i + 3] != '=') triple |= decode(data[i + 3]);

    result.push_back(std::byte((triple >> 16) & 0xFF));
    if (data[i + 2] != '=') result.push_back(std::byte((triple >> 8) & 0xFF));
    if (data[i + 3] != '=') result.push_back(std::byte(triple & 0xFF));
  }

  return result;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
