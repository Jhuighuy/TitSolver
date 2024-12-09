/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <utility>

#include <zstd.h>
#include <zstd_errors.h>

#include "tit/core/checks.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/serialization/source_sink.hpp"

#include "tit/data/zstd.hpp"

namespace tit::data::zstd {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Compressor::Compressor() : context_{ZSTD_createCCtx()} {}

void Compressor::ContextDeleter_::operator()(ZSTD_CCtx_s* context) noexcept {
  ZSTD_freeCCtx(context);
}

void Compressor::compress(DataSource& source, DataSink& sink) {
  TIT_ASSERT(context_ != nullptr, "ZSTD context was not initialized!");

  // Prepare the buffers.
  const auto chunk_size = ZSTD_CStreamInSize();
  if (buffer_.empty()) buffer_.resize(ZSTD_CStreamOutSize());

  while (true) {
    // Read the next chunk of data from the source.
    const auto data = source.pull(chunk_size);
    const auto last_chunk = data.size() < chunk_size;
    const auto mode = last_chunk ? ZSTD_e_end : ZSTD_e_continue;

    // Compress the input within a few iterations.
    ZSTD_inBuffer input{
        .src = data.data(),
        .size = data.size(),
        .pos = 0,
    };
    while (true) {
      // Compress the remaining input.
      ZSTD_outBuffer output = {
          .dst = buffer_.data(),
          .size = buffer_.size(),
          .pos = 0,
      };
      const auto remaining =
          ZSTD_compressStream2(context_.get(), &output, &input, mode);
      if (bool(ZSTD_isError(remaining))) {
        const auto status = remaining;
        TIT_THROW("ZSTD compression failed ({}): {} ",
                  std::to_underlying(ZSTD_getErrorCode(status)),
                  ZSTD_getErrorName(status));
      }

      // Flush the output to the sink.
      sink.push(std::span{buffer_.data(), output.pos});

      // Check if we are done: either if the last chunk was completely flushed,
      // or if the input was exhausted.
      if (last_chunk && remaining == 0) break;
      if (input.pos == input.size) break;
    }

    if (last_chunk) break;
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Decompressor::Decompressor() : context_{ZSTD_createDCtx()} {}

void Decompressor::ContextDeleter_::operator()(ZSTD_DCtx_s* context) noexcept {
  ZSTD_freeDCtx(context);
}

void Decompressor::decompress(DataSource& source, DataSink& sink) {
  TIT_ASSERT(context_ != nullptr, "ZSTD context was not initialized!");

  // Prepare the buffers.
  const auto chunk_size = ZSTD_DStreamInSize();
  if (buffer_.empty()) buffer_.resize(ZSTD_DStreamOutSize());

  while (true) {
    // Read the next chunk of data from the source.
    const auto data = source.pull(chunk_size);

    // Decompress the input within a few iterations.
    ZSTD_inBuffer input{
        .src = data.data(),
        .size = data.size(),
        .pos = 0,
    };
    while (input.pos < input.size) {
      // Decompress the remaining input.
      ZSTD_outBuffer output = {
          .dst = buffer_.data(),
          .size = buffer_.size(),
          .pos = 0,
      };
      const auto status =
          ZSTD_decompressStream(context_.get(), &output, &input);
      if (bool(ZSTD_isError(status))) {
        TIT_THROW("ZSTD decompression failed ({}): {} ",
                  std::to_underlying(ZSTD_getErrorCode(status)),
                  ZSTD_getErrorName(status));
      }

      // Flush the output to the sink.
      sink.push(std::span{buffer_.data(), output.pos});
    }

    if (const auto last_chunk = data.size() < chunk_size; last_chunk) break;
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::data::zstd
