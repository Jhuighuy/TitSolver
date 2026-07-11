/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <cstdint>

#include "tit/prop/spec.hpp"

namespace tit::wcsph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Version of the case file schema described by `make_case_spec()`.
///
/// Stored in the case file under the "schema" key. Bump it together with the
/// explicit migration steps for the files written by the older versions.
inline constexpr std::int64_t case_schema_version = 1;

/// Build the WCSPH case specification.
///
/// The specification is the single source of truth for the case file layout,
/// shared by the solver and the GUI.
auto make_case_spec() -> prop::SpecPtr;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::wcsph
