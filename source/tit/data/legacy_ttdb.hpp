/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <cstddef>
#include <filesystem>
#include <optional>

namespace tit::data {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Convert one legacy SQLite-backed `.ttdb` series to a `.tit-run` directory.
///
/// If no series index is specified, the most recently stored series is used.
/// Legacy files did not persist particle identity or kind, so missing fields
/// are reconstructed from stable row order and the historical all-fluid
/// visualization convention.
void convert_ttdb(const std::filesystem::path& source,
                  const std::filesystem::path& destination,
                  std::optional<std::size_t> series_index = std::nullopt);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::data
