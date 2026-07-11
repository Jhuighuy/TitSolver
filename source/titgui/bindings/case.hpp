/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <napi.h>

namespace tit::gui::cases {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Return a promise resolving to the case specification as JSON text.
auto caseSpec(const Napi::CallbackInfo& info) -> Napi::Value;

/// Load a case tree from a file, resolving to the tree as JSON text.
auto loadCaseTree(const Napi::CallbackInfo& info) -> Napi::Value;

/// Save a case tree, given as JSON text, to a file.
auto saveCaseTree(const Napi::CallbackInfo& info) -> Napi::Value;

/// Materialize a case tree, given as JSON text, against the case
/// specification. Resolves to `{tree, issues, namespaces}` as JSON text.
auto materializeCase(const Napi::CallbackInfo& info) -> Napi::Value;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Register the case submodule on the addon exports.
void init_submodule(Napi::Env env, Napi::Object exports);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::gui::cases
