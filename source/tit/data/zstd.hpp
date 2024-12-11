/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <memory>
#include <span>
#include <utility>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/stream.hpp"

struct ZSTD_CCtx_s;
struct ZSTD_DCtx_s;

namespace tit::data::zstd {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Stream that compresses data using ZSTD and writes it to the underlying
/// output stream.
class StreamCompressor final : public OutputStream<byte_t> {
public:

  /// Construct a stream compressor.
  explicit StreamCompressor(OutputStreamPtr<byte_t> stream);

  /// Compress the data and write it to the underlying stream.
  void write(std::span<const byte_t> data) override;

  /// Flush the stream.
  void flush() override;

private:

  struct Deleter_ final {
    static void operator()(ZSTD_CCtx_s* context) noexcept;
  };

  OutputStreamPtr<byte_t> stream_;
  std::unique_ptr<ZSTD_CCtx_s, Deleter_> context_;
  std::vector<byte_t> in_buffer_;
  std::vector<byte_t> out_buffer_;
  static const size_t in_chunk_size_;
  static const size_t out_chunk_size_;

}; // class StreamCompressor

/// Make a stream compressor.
inline auto make_stream_compressor(OutputStreamPtr<byte_t> stream)
    -> OutputStreamPtr<byte_t> {
  return make_flushable<StreamCompressor>(std::move(stream));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Stream that reads data from the underlying input stream and decompresses
/// it using ZSTD.
class StreamDecompressor final : public InputStream<byte_t> {
public:

  /// Construct a stream decompressor.
  explicit StreamDecompressor(InputStreamPtr<byte_t> stream);

  /// Decompress the data.
  auto read(std::span<byte_t> data) -> size_t override;

private:

  struct Deleter_ final {
    static void operator()(ZSTD_DCtx_s* context) noexcept;
  };

  InputStreamPtr<byte_t> stream_;
  std::unique_ptr<ZSTD_DCtx_s, Deleter_> context_;
  std::vector<byte_t> in_buffer_;
  std::vector<byte_t> out_buffer_;
  size_t in_offset_ = 0;
  size_t out_offset_ = 0;
  size_t last_status_ = 0;
  static const size_t in_chunk_size_;
  static const size_t out_chunk_size_;

}; // class StreamDecompressor

/// Make a stream decompressor.
inline auto make_stream_decompressor(InputStreamPtr<byte_t> stream)
    -> InputStreamPtr<byte_t> {
  return std::make_unique<StreamDecompressor>(std::move(stream));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::data::zstd
