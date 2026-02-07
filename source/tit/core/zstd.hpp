/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <cstddef>
#include <memory>
#include <span>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/stream.hpp"

struct ZSTD_CCtx_s;
struct ZSTD_DCtx_s;

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Stream that compresses data using ZSTD and writes it to the underlying
/// output stream.
class ZSTDStreamCompressor final : public OutputStream<std::byte> {
public:

  /// Construct a stream compressor.
  explicit ZSTDStreamCompressor(OutputStreamPtr<std::byte> stream);

  /// Compress the data and write it to the underlying stream.
  void write(std::span<const std::byte> data) override;

  /// Flush the stream.
  void flush() override;

private:

  struct Deleter_ final {
    static void operator()(ZSTD_CCtx_s* context) noexcept;
  };

  OutputStreamPtr<std::byte> stream_;
  std::unique_ptr<ZSTD_CCtx_s, Deleter_> context_;
  std::vector<std::byte> in_buffer_;
  std::vector<std::byte> out_buffer_;

  static const size_t in_chunk_size_;
  static const size_t out_chunk_size_;

}; // class ZSTDStreamCompressor

/// Make a stream compressor.
auto make_zstd_stream_compressor(OutputStreamPtr<std::byte> stream)
    -> OutputStreamPtr<std::byte>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Stream that reads data from the underlying input stream and decompresses
/// it using ZSTD.
class ZSTDStreamDecompressor final : public InputStream<std::byte> {
public:

  /// Construct a stream decompressor.
  explicit ZSTDStreamDecompressor(InputStreamPtr<std::byte> stream);

  /// Decompress the data.
  auto read(std::span<std::byte> data) -> size_t override;

private:

  struct Deleter_ final {
    static void operator()(ZSTD_DCtx_s* context) noexcept;
  };

  InputStreamPtr<std::byte> stream_;
  std::unique_ptr<ZSTD_DCtx_s, Deleter_> context_;
  std::vector<std::byte> in_buffer_;
  std::vector<std::byte> out_buffer_;
  size_t in_offset_ = 0;
  size_t out_offset_ = 0;
  size_t last_status_ = 0;

  static const size_t in_chunk_size_;
  static const size_t out_chunk_size_;

}; // class ZSTDStreamDecompressor

/// Make a stream decompressor.
auto make_zstd_stream_decompressor(InputStreamPtr<std::byte> stream)
    -> InputStreamPtr<std::byte>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
