/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>

#include "tit/core/basic_types.hpp"

namespace tit::graph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Dummy uniform partitioning function.
struct UniformPartition final {
  static void operator()(const auto& graph, auto& parts, size_t num_parts) {
    const auto num_nodes = graph.num_nodes();
    const auto part_size = num_nodes / num_parts;
    const auto remainder = num_nodes % num_parts;
    for (size_t part = 0; part < num_parts; ++part) {
      const auto first = part * part_size + std::min(part, remainder);
      const auto last = (part + 1) * part_size + std::min(part + 1, remainder);
      for (size_t i = first; i < last; ++i) parts[i] = part;
    }
  }
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::graph
