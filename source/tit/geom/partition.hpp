/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include "tit/core/type.hpp"

// IWYU pragma: begin_exports
#include "tit/geom/partition/pixelated_partition.hpp"
#include "tit/geom/partition/recursive_bisection.hpp"
#include "tit/geom/partition/sort_partition.hpp"
// IWYU pragma: end_exports

namespace tit::geom {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Partition function type.
template<class PF>
concept partition_func = specialization_of<PF, PixelatedPartition> ||
                         specialization_of<PF, RecursiveBisection> ||
                         specialization_of<PF, SortPartition>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::geom
