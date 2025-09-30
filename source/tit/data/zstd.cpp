/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <span>
#include <utility>

#include <zstd.h>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/stream.hpp"
#include "tit/data/zstd.hpp"

namespace tit::data::zstd {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// NOLINTBEGIN(cert-err58-cpp)
const size_t StreamCompressor::in_chunk_size_ = ZSTD_CStreamInSize();
const size_t StreamCompressor::out_chunk_size_ = ZSTD_CStreamOutSize();
// NOLINTEND(cert-err58-cpp)

StreamCompressor::StreamCompressor(OutputStreamPtr<std::byte> stream)
    : stream_{std::move(stream)}, context_{ZSTD_createCCtx()} {
  TIT_ASSERT(stream_ != nullptr, "Stream is null!");
}

void StreamCompressor::Deleter_::operator()(ZSTD_CCtx_s* context) noexcept {
  ZSTD_freeCCtx(context);
}

void StreamCompressor::write(std::span<const std::byte> data) {
  // Prepare the buffer.
  if (in_buffer_.capacity() == 0) in_buffer_.reserve(in_chunk_size_);

  // Copy the remaining data into the buffer in chunks.
  while (!data.empty()) {
    const auto copied =
        std::min(in_chunk_size_ - in_buffer_.size(), data.size());
    std::copy_n(data.begin(), copied, std::back_inserter(in_buffer_));
    data = data.subspan(copied);
    if (in_buffer_.size() == in_chunk_size_) {
      flush();
      TIT_ASSERT(in_buffer_.empty(), "Buffer must be empty after flushing!");
    }
  }
}

void StreamCompressor::flush() {
  TIT_ASSERT(context_ != nullptr, "ZSTD context is null!");
  TIT_ASSERT(stream_ != nullptr, "Stream is null!");

  // Prepare the buffers.
  // Note: do not exit early if `in_buffer_` is empty. This may happen in two
  //       scenarios: nothing was actually written to the stream, or due to
  //       some miracle the data size is a multiple of `in_chunk_size_`. We
  //       cannot easliy distinguish between these two cases, so we just
  //       always flush the stream.
  if (out_buffer_.empty()) out_buffer_.resize(out_chunk_size_);
  const auto is_last_chunk = in_buffer_.size() < in_chunk_size_;
  const auto mode = is_last_chunk ? ZSTD_e_end : ZSTD_e_continue;

  // Compress the input buffer within a few iterations.
  ZSTD_inBuffer input{
      .src = in_buffer_.data(),
      .size = in_buffer_.size(),
      .pos = 0,
  };
  while (true) {
    // Compress the remaining input.
    ZSTD_outBuffer output = {
        .dst = out_buffer_.data(),
        .size = out_buffer_.size(),
        .pos = 0,
    };
    const auto remaining =
        ZSTD_compressStream2(context_.get(), &output, &input, mode);
    TIT_ENSURE(ZSTD_isError(remaining) == 0,
               "ZSTD compression failed ({}): {}.",
               std::to_underlying(ZSTD_getErrorCode(remaining)),
               ZSTD_getErrorName(remaining));

    // Write the compressed data to the underlying stream.
    stream_->write({out_buffer_.data(), output.pos});

    // Check if we are done: either if the input was exhausted or
    // if the last chunk was completely flushed.
    if ((input.pos == input.size) && (!is_last_chunk || remaining == 0)) break;
  }

  // Flush the stream.
  stream_->flush();

  // Reset the buffers.
  in_buffer_.clear();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// NOLINTBEGIN(cert-err58-cpp)
const size_t StreamDecompressor::in_chunk_size_ = ZSTD_DStreamInSize();
const size_t StreamDecompressor::out_chunk_size_ = ZSTD_DStreamOutSize();
// NOLINTEND(cert-err58-cpp)

StreamDecompressor::StreamDecompressor(InputStreamPtr<std::byte> stream)
    : stream_{std::move(stream)}, context_{ZSTD_createDCtx()} {
  TIT_ASSERT(stream_ != nullptr, "Stream is null!");
}

void StreamDecompressor::Deleter_::operator()(ZSTD_DCtx_s* context) noexcept {
  ZSTD_freeDCtx(context);
}

auto StreamDecompressor::read(std::span<std::byte> data) -> size_t {
  TIT_ASSERT(context_ != nullptr, "ZSTD context is null!");
  TIT_ASSERT(stream_ != nullptr, "Stream is null!");

  // Prepare the buffers.
  if (in_buffer_.capacity() == 0) in_buffer_.reserve(in_chunk_size_);
  if (out_buffer_.capacity() == 0) out_buffer_.reserve(out_chunk_size_);

  size_t total_copied = 0;
  while (!data.empty()) {
    // If the output buffer is exhausted, decompress more data.
    if (out_offset_ == out_buffer_.size()) {
      // If the input buffer, read more data.
      if (in_offset_ == in_buffer_.size()) {
        in_offset_ = 0;
        in_buffer_.resize(in_chunk_size_);
        in_buffer_.resize(stream_->read(in_buffer_));
        if (in_buffer_.empty()) {
          TIT_ENSURE(last_status_ == 0,
                     "ZSTD decompression failed: truncated frame.");
          break; // Input stream is exhausted.
        }
      }

      // Decompress the input buffer.
      out_buffer_.resize(out_chunk_size_);
      ZSTD_inBuffer input{
          .src = in_buffer_.data(),
          .size = in_buffer_.size(),
          .pos = in_offset_,
      };
      ZSTD_outBuffer output = {
          .dst = out_buffer_.data(),
          .size = out_buffer_.size(),
          .pos = 0,
      };
      // Store the status in order to identify truncated frames.
      last_status_ = ZSTD_decompressStream(context_.get(), &output, &input);
      TIT_ENSURE(ZSTD_isError(last_status_) == 0,
                 "ZSTD decompression failed ({}): {}.",
                 std::to_underlying(ZSTD_getErrorCode(last_status_)),
                 ZSTD_getErrorName(last_status_));
      TIT_ASSERT(input.pos > in_offset_, "Offset was not updated!");
      TIT_ASSERT(input.pos <= in_buffer_.size(), "Offset is out of range!");
      in_offset_ = input.pos;
      TIT_ASSERT(output.pos <= out_buffer_.size(), "Offset is out of range!");
      out_buffer_.resize(output.pos);
      out_offset_ = 0;
    }

    // Copy what we have in the output buffer.
    TIT_ASSERT(out_offset_ <= out_buffer_.size(), "Offset is out of range!");
    const auto copied = std::min(out_buffer_.size() - out_offset_, data.size());
    std::copy_n(out_buffer_.begin() + static_cast<ssize_t>(out_offset_),
                copied,
                data.begin());
    total_copied += copied;
    out_offset_ += copied;
    data = data.subspan(copied);
  }

  return total_copied;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::data::zstd
