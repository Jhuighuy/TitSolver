/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <filesystem>
#include <fstream>
#include <ios>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

#include <miniz.h>
#include <miniz_common.h>
#include <miniz_zip.h>

#include "tit/core/checks.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/str.hpp"
#include "tit/core/zip.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Read entire contents of a file into a vector.
auto read_file(const std::filesystem::path& path) {
  TIT_ASSERT(std::filesystem::is_regular_file(path), "File does not exist!");

  /// @todo In C++26 there would be no need for `.string()`.
  std::ifstream file_stream{path, std::ios::binary};
  TIT_ENSURE(file_stream.is_open(),
             "Failed to open file '{}' for reading.",
             path.string());

  return std::vector(std::istreambuf_iterator{file_stream}, {});
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace

struct ZipWriter::ZipArchive_ : mz_zip_archive {};

ZipWriter::~ZipWriter() noexcept = default;

ZipWriter::ZipWriter(ZipWriter&&) noexcept = default;

auto ZipWriter::operator=(ZipWriter&&) noexcept -> ZipWriter& = default;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

ZipWriter::ZipWriter(const std::filesystem::path& path)
    : zip_{std::make_unique<ZipArchive_>()} {
  mz_zip_zero_struct(zip_.get());
  const auto status = mz_zip_writer_init_file(zip_.get(), path.c_str(), 0);
  TIT_ENSURE(status == MZ_TRUE, "Failed to initialize Zip writer.");
}

void ZipWriter::close() {
  auto* const zip = zip_.get();
  TIT_ASSERT(zip != nullptr, "Zip archive is already closed!");

  auto status = mz_zip_writer_finalize_archive(zip);
  TIT_ENSURE(status == MZ_TRUE, "Failed to finalize Zip archive writer.");

  status = mz_zip_writer_end(zip);
  TIT_ENSURE(status == MZ_TRUE, "Failed to close Zip archive writer.");

  zip_.reset();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void ZipWriter::add_file(const std::filesystem::path& file_path) {
  add_file(file_path, file_path.filename());
}

void ZipWriter::add_file(const std::filesystem::path& file_path,
                         const std::filesystem::path& zip_path) {
  TIT_ASSERT(!zip_path.empty(), "Zip path is empty!");
  TIT_ASSERT(std::filesystem::is_regular_file(file_path), "Not a file!");

  auto* const zip = zip_.get();
  TIT_ASSERT(zip != nullptr, "Zip archive is already closed!");

  /// @todo In C++26 there would be no need for `.string()`.
  const auto file_data = read_file(file_path);
  const auto status = mz_zip_writer_add_mem(zip,
                                            zip_path.c_str(),
                                            file_data.data(),
                                            file_data.size(),
                                            MZ_BEST_COMPRESSION);
  TIT_ENSURE(status == MZ_TRUE,
             "Failed to add file '{}' located of size {} to Zip archive.",
             zip_path.string(),
             fmt_memsize(file_data.size()));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void ZipWriter::add_dir(const std::filesystem::path& dir_path) {
  add_dir(dir_path, dir_path.filename());
}

void ZipWriter::add_dir(const std::filesystem::path& dir_path,
                        const std::filesystem::path& zip_path) {
  TIT_ASSERT(!zip_path.empty(), "Zip path is empty!");
  TIT_ASSERT(std::filesystem::is_directory(dir_path), "Not a directory!");

  new_dir(zip_path);

  for (const auto& entry_path :
       std::filesystem::recursive_directory_iterator(dir_path)) {
    const auto rel_path = std::filesystem::relative(entry_path, dir_path);
    const auto zip_rel_path = zip_path / rel_path;
    if (std::filesystem::is_directory(entry_path)) {
      new_dir(zip_rel_path);
    } else if (std::filesystem::is_regular_file(entry_path)) {
      add_file(entry_path, zip_rel_path);
    }
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void ZipWriter::new_dir(const std::filesystem::path& zip_path) {
  TIT_ASSERT(!zip_path.empty(), "Zip path is empty!");

  auto* const zip = zip_.get();
  TIT_ASSERT(zip != nullptr, "Zip archive is already closed!");

  /// @todo In C++26 there would be no need for `.string()`.
  auto zip_path_str = zip_path.string();
  if (zip_path_str.back() != '/') zip_path_str.push_back('/');
  const auto status = mz_zip_writer_add_mem(zip,
                                            zip_path_str.c_str(),
                                            nullptr,
                                            0,
                                            MZ_BEST_COMPRESSION);
  TIT_ENSURE(status == MZ_TRUE,
             "Failed to add directory '{}' to Zip archive.",
             zip_path.string());
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
