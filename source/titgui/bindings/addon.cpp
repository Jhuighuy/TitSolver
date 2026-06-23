/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <napi.h>

#include "titgui/bindings/properties.hpp"
#include "titgui/bindings/storage.hpp"

namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto init_module(Napi::Env env, Napi::Object exports) -> Napi::Object {
  using namespace tit::gui;
  properties::init_submodule(env, exports);
  storage::init_submodule(env, exports);
  return exports;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace

// NOLINTNEXTLINE(*-include-cleaner,*-use-anonymous-namespace,*-use-trailing-return-type)
NODE_API_MODULE(titgui_bindings, init_module)
