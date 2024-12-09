/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <memory>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/serialization/source_sink.hpp"

struct ZSTD_CCtx_s;
struct ZSTD_DCtx_s;

namespace tit::data::zstd {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Data compressor that uses ZSTD.
class Compressor final {
public:

  /// Construct a compressor.
  Compressor();

  /// Compress the data.
  void compress(DataSource& source, DataSink& sink);

private:

  struct ContextDeleter_ final {
    static void operator()(ZSTD_CCtx_s* context) noexcept;
  };

  std::unique_ptr<ZSTD_CCtx_s, ContextDeleter_> context_;
  std::vector<byte_t> buffer_;

}; // class Compressor

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Data decompressor that uses ZSTD.
class Decompressor final {
public:

  /// Construct a decompressor.
  Decompressor();

  /// Decompress the data.
  void decompress(DataSource& source, DataSink& sink);

private:

  struct ContextDeleter_ final {
    static void operator()(ZSTD_DCtx_s* context) noexcept;
  };

  std::unique_ptr<ZSTD_DCtx_s, ContextDeleter_> context_;
  std::vector<byte_t> buffer_;

}; // class Decompressor

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::data::zstd
