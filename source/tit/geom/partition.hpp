/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include "tit/core/type_utils.hpp"

// IWYU pragma: begin_exports
#include "tit/geom/partition/grid_graph_partition.hpp"
#include "tit/geom/partition/recursive_bisection.hpp"
#include "tit/geom/partition/sort_partition.hpp"
// IWYU pragma: end_exports

namespace tit::geom {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Partition function type.
template<class PF>
concept partition_func = specialization_of<PF, GridGraphPartition> ||
                         specialization_of<PF, RecursiveBisection> ||
                         specialization_of<PF, SortPartition>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::geom
