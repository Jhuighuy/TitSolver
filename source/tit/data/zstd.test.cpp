/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <algorithm>
#include <numbers>
#include <random>
#include <ranges>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/range.hpp"
#include "tit/core/stream.hpp"
#include "tit/data/zstd.hpp"
#include "tit/testing/test.hpp"

namespace tit {
namespace {

using data::zstd::make_stream_compressor;
using data::zstd::make_stream_decompressor;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("data::zstd::empty") {
  SUBCASE("compress and decompress") {
    std::vector<byte_t> compressed_data;
    make_stream_compressor(make_container_output_stream(compressed_data))
        ->write({});

    // Note: we cannot make any assumptions about the compression data, size,
    // it may be empty, or it may contain a few bytes with ZSTD header.

    std::vector<byte_t> result(1024);
    auto decompressor =
        make_stream_decompressor(make_range_input_stream(compressed_data));
    CHECK(decompressor->read(result) == 0);
    CHECK(decompressor->read(result) == 0);
  }
  SUBCASE("decompress") {
    std::vector<byte_t> empty{};
    std::vector<byte_t> result(1024);
    auto decompressor =
        make_stream_decompressor(make_range_input_stream(empty));
    CHECK(decompressor->read(result) == 0);
    CHECK(decompressor->read(result) == 0);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("data::zstd::small_data") {
  const auto small_data = to_bytes(std::numbers::pi);

  std::vector<byte_t> compressed_data;
  make_stream_compressor(make_container_output_stream(compressed_data))
      ->write(small_data);
  REQUIRE(!compressed_data.empty());

  std::vector<byte_t> decompressed_data(small_data.size() * 2);
  auto decompressor =
      make_stream_decompressor(make_range_input_stream(compressed_data));
  REQUIRE(decompressor->read(decompressed_data) == small_data.size());
  CHECK(decompressed_data >= small_data);
  CHECK(decompressor->read(decompressed_data) == 0);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("data::zstd::large_data") {
  constexpr auto run_test = [](size_t size_multiplier) {
    const auto large_data =
        std::views::repeat(to_byte_array(std::numbers::pi), size_multiplier) |
        std::views::join | std::ranges::to<std::vector>();

    // Note: ZSTD's preferred chunk size is around 128 KiB.
    constexpr size_t small_chunk_size = 8;
    constexpr size_t moderate_chunk_size = small_chunk_size * 125;
    constexpr size_t large_chunk_size = moderate_chunk_size * 1000;

    SUBCASE("single chunk") {
      std::vector<byte_t> compressed_data;
      make_stream_compressor(make_container_output_stream(compressed_data))
          ->write(large_data);
      REQUIRE(!compressed_data.empty());

      // We are compressing a single repeating pattern, so I expect the
      // compressed data to be smaller than 1 KiB.
      CHECK(compressed_data.size() <= 1024);

      std::vector<byte_t> decompressed_data(large_data.size() * 3 / 2);
      auto decompressor =
          make_stream_decompressor(make_range_input_stream(compressed_data));
      REQUIRE(decompressor->read(decompressed_data) == large_data.size());
      CHECK(decompressed_data >= large_data);
      CHECK(decompressor->read(decompressed_data) == 0);
    }

    SUBCASE("compress in chunks") {
      const auto run_subcase = [&large_data](size_t chunk_size) {
        std::vector<byte_t> compressed_data;
        auto compressor = make_stream_compressor(
            make_container_output_stream(compressed_data));
        for (const auto chunk : large_data | std::views::chunk(chunk_size)) {
          compressor->write(chunk);
        }
        compressor->flush();
        REQUIRE(!compressed_data.empty());

        std::vector<byte_t> decompressed_data(large_data.size() * 3 / 2);
        auto decompressor =
            make_stream_decompressor(make_range_input_stream(compressed_data));
        REQUIRE(decompressor->read(decompressed_data) == large_data.size());
        CHECK(decompressed_data >= large_data);
        CHECK(decompressor->read(decompressed_data) == 0);
      };

      SUBCASE("small") {
        run_subcase(small_chunk_size);
      }
      SUBCASE("moderate") {
        run_subcase(moderate_chunk_size);
      }
      SUBCASE("large") {
        run_subcase(large_chunk_size);
      }
    }

    SUBCASE("decompress in chunks") {
      const auto run_subcase = [&large_data](size_t chunk_size) {
        std::vector<byte_t> compressed_data;
        make_stream_compressor(make_container_output_stream(compressed_data))
            ->write(large_data);
        REQUIRE(!compressed_data.empty());

        std::vector<byte_t> decompressed_data(chunk_size);
        auto decompressor =
            make_stream_decompressor(make_range_input_stream(compressed_data));
        for (size_t offset = 0; offset < large_data.size();
             offset += chunk_size) {
          const auto copied = std::min(chunk_size, large_data.size() - offset);
          REQUIRE(decompressor->read(decompressed_data) == copied);
          CHECK_RANGE_EQ(std::span{decompressed_data}.subspan(0, copied),
                         std::span{large_data}.subspan(offset, copied));
        }
      };

      SUBCASE("small") {
        run_subcase(small_chunk_size);
      }
      SUBCASE("moderate") {
        run_subcase(moderate_chunk_size);
      }
      SUBCASE("large") {
        run_subcase(large_chunk_size);
      }
    }
  };

  SUBCASE("even size") {
    run_test(1024 * 1024UZ); // 8 MiB.
  }
  SUBCASE("uneven size") {
    run_test(1000 * 1000UZ); // 8 MB.
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("data::zstd::errors") {
  static std::minstd_rand rng{std::random_device{}()};
  SUBCASE("completely invalid data") {
    std::vector<byte_t> garbage(1024);
    std::ranges::generate(garbage, [] { return static_cast<byte_t>(rng()); });

    std::vector<byte_t> result(garbage.size() * 2);
    CHECK_THROWS_MSG(make_stream_decompressor(make_range_input_stream(garbage))
                         ->read(result),
                     Exception,
                     "Unknown frame descriptor.");
  }
  SUBCASE("partially invalid data") {
    // 1MiB of data.
    constexpr size_t size_multiplier = 1024 * 1024UZ;
    const auto data =
        std::views::repeat(to_byte_array(std::numbers::pi), size_multiplier) |
        std::views::join | std::ranges::to<std::vector>();

    std::vector<byte_t> compressed_data;
    make_stream_compressor(make_container_output_stream(compressed_data))
        ->write(data);
    REQUIRE(!compressed_data.empty());

    std::vector<byte_t> result(data.size() * 2);
    SUBCASE("corrupted header") {
      std::ranges::shuffle(compressed_data.begin(),
                           compressed_data.begin() + 64,
                           rng);
      CHECK_THROWS_MSG(
          make_stream_decompressor(make_range_input_stream(compressed_data))
              ->read(result),
          Exception,
          "Unknown frame descriptor.");
    }
    SUBCASE("trailing garbage") {
      const auto old_size = compressed_data.size();
      compressed_data.resize(old_size + 128);
      std::ranges::generate( //
          compressed_data.begin() + static_cast<ssize_t>(old_size),
          compressed_data.end(),
          [] { return static_cast<byte_t>(rng()); });
      CHECK_THROWS_MSG(
          make_stream_decompressor(make_range_input_stream(compressed_data))
              ->read(result),
          Exception,
          "Unknown frame descriptor.");
    }
    SUBCASE("truncated frame") {
      const auto old_size = compressed_data.size();
      compressed_data.resize(old_size - 128);
      CHECK_THROWS_MSG(
          make_stream_decompressor(make_range_input_stream(compressed_data))
              ->read(result),
          Exception,
          "truncated frame");
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
