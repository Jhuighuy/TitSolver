/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <filesystem>
#include <fstream>
#include <ios>
#include <iterator>
#include <string>
#include <vector>

#include <miniz.h>
#include <miniz_common.h>
#include <miniz_zip.h>

#include "tit/core/checks.hpp"
#include "tit/core/exception.hpp"
#include "tit/data/zip.hpp"

namespace tit::data {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
namespace {

// ZIP archive writer.
class ZipArchiveWriter final {
public:

  // Create a new ZIP archive writer.
  explicit ZipArchiveWriter(const std::filesystem::path& path) {
    mz_zip_zero_struct(&zip_);
    const auto status = mz_zip_writer_init_file(&zip_, path.c_str(), 0);
    TIT_ENSURE(status == MZ_TRUE, "Failed to initialize ZIP writer.");
  }

  // Close the ZIP archive.
  void close() {
    auto status = mz_zip_writer_finalize_archive(&zip_);
    TIT_ENSURE(status == MZ_TRUE, "Failed to finalize ZIP archive writer.");
    status = mz_zip_writer_end(&zip_);
    TIT_ENSURE(status == MZ_TRUE, "Failed to close ZIP archive writer.");
  }

  // Add a directory to the ZIP archive.
  void add_dir(const std::filesystem::path& zip_path) {
    TIT_ASSERT(!zip_path.empty(), "ZIP directory entry name is empty!");
    auto zip_path_str = zip_path.string();
    if (zip_path_str.back() != '/') zip_path_str.push_back('/');
    const auto status = mz_zip_writer_add_mem(&zip_,
                                              zip_path_str.c_str(),
                                              nullptr,
                                              0,
                                              MZ_BEST_COMPRESSION);
    /// @todo In C++26 there would be no need for `.string()`.
    TIT_ENSURE(status == MZ_TRUE,
               "Failed to add directory '{}' to ZIP archive.",
               zip_path.string());
  }

  // Add a file to the ZIP archive.
  void add_file(const std::filesystem::path& zip_path,
                const std::filesystem::path& file_path) {
    std::ifstream file_stream(file_path, std::ios::binary);
    TIT_ENSURE(file_stream.is_open(), "Failed to open file for reading.");
    const std::vector file_data(std::istreambuf_iterator{file_stream}, {});

    const auto status = mz_zip_writer_add_mem(&zip_,
                                              zip_path.c_str(),
                                              file_data.data(),
                                              file_data.size(),
                                              MZ_BEST_COMPRESSION);
    /// @todo In C++26 there would be no need for `.string()`.
    TIT_ENSURE(status == MZ_TRUE,
               "Failed to add file '{}' located at '{}' to ZIP.",
               zip_path.string(),
               file_path.string());
  }

private:

  mz_zip_archive zip_{};

}; // class ZipArchiveWriter

} // namespace

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void zip_directory(const std::filesystem::path& dir_path,
                   const std::filesystem::path& zip_path) {
  TIT_ENSURE(std::filesystem::exists(dir_path), "Directory does not exist!");
  TIT_ENSURE(std::filesystem::is_directory(dir_path), "Not a directory!");

  ZipArchiveWriter zip_writer{zip_path};

  const auto dir_name = dir_path.filename();
  zip_writer.add_dir(dir_name);

  for (const auto& entry_path :
       std::filesystem::recursive_directory_iterator(dir_path)) {
    const auto rel_path = std::filesystem::relative(entry_path, dir_path);
    const auto zip_rel_path = dir_name / rel_path;
    if (std::filesystem::is_directory(entry_path)) {
      zip_writer.add_dir(zip_rel_path);
    } else if (std::filesystem::is_regular_file(entry_path)) {
      zip_writer.add_file(zip_rel_path, entry_path);
    }
  }

  zip_writer.close();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::data
