/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <filesystem>
#include <memory>

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Zip archive writer.
class ZipWriter final {
public:

  /// Create a new Zip archive writer.
  explicit ZipWriter(const std::filesystem::path& path);

  /// Move constructor.
  ZipWriter(ZipWriter&&) noexcept;

  /// This class is not copy-constructible.
  ZipWriter(const ZipWriter&) = delete;

  /// Move assignment.
  auto operator=(ZipWriter&&) noexcept -> ZipWriter&;

  /// This class is not copy-assignable.
  auto operator=(const ZipWriter&) -> ZipWriter& = delete;

  /// Destructor.
  ~ZipWriter() noexcept;

  /// Close the Zip archive.
  void close();

  /// Add a file to the Zip archive.
  /// @{
  void add_file(const std::filesystem::path& file_path);
  void add_file(const std::filesystem::path& file_path,
                const std::filesystem::path& zip_path);
  /// @}

  /// Recursively adds all files in a directory to the Zip archive.
  /// @{
  void add_dir(const std::filesystem::path& dir_path);
  void add_dir(const std::filesystem::path& dir_path,
               const std::filesystem::path& zip_path);
  /// @}

  /// Create a new empty directory in the Zip archive.
  void new_dir(const std::filesystem::path& zip_path);

private:

  struct ZipArchive_;
  std::unique_ptr<ZipArchive_> zip_;

}; // class ZipWriter

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
