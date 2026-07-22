/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include "tit/io/type.hpp"

namespace tit::data {

// Legacy storage uses the backend-neutral I/O type descriptors while the
// SQLite conversion window remains supported.
using io::Kind;
using io::kind_of;
using io::known_kind_of;
using io::known_type_of;
using io::Rank;
using io::Type;
using io::type_of;

} // namespace tit::data
