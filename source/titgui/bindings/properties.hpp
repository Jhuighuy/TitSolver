/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <memory>

#include <napi.h>

#include "tit/prop/spec.hpp"
#include "tit/prop/tree.hpp"

namespace tit::gui::properties {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// JavaScript wrapper around `tit::prop::Tree`.
class PropTreeWrap final : public Napi::ObjectWrap<PropTreeWrap> {
public:

  /// Register the wrapped tree class.
  static void init(Napi::Env env, Napi::Object exports);

  /// Create a wrapped tree object.
  static auto New(Napi::Env env, prop::Tree tree) -> Napi::Object;

  /// Unwrap a JavaScript tree object.
  static auto UnwrapTree(const Napi::Value& value) -> prop::Tree&;

  static auto fromJSON(const Napi::CallbackInfo& info) -> Napi::Value;
  static auto fromJSONText(const Napi::CallbackInfo& info) -> Napi::Value;
  static auto fromFile(const Napi::CallbackInfo& info) -> Napi::Value;

  auto toJSON(const Napi::CallbackInfo& info) -> Napi::Value;
  auto toJSONText(const Napi::CallbackInfo& info) -> Napi::Value;
  auto saveToFile(const Napi::CallbackInfo& info) -> Napi::Value;

private:

  friend class Napi::ObjectWrap<PropTreeWrap>;
  using Napi::ObjectWrap<PropTreeWrap>::ObjectWrap;

  static auto constructor() -> Napi::FunctionReference&;

  prop::Tree tree_;

}; // class PropTreeWrap

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// JavaScript wrapper around `tit::prop::Spec`.
class PropSpecWrap final : public Napi::ObjectWrap<PropSpecWrap> {
public:

  /// Register the wrapped spec class.
  static void init(Napi::Env env);

  /// Create a wrapped spec object.
  static auto New(Napi::Env env, prop::SpecPtr spec) -> Napi::Object;

  auto toJSON(const Napi::CallbackInfo& info) -> Napi::Value;
  auto materialize(const Napi::CallbackInfo& info) -> Napi::Value;

private:

  friend class Napi::ObjectWrap<PropSpecWrap>;
  using Napi::ObjectWrap<PropSpecWrap>::ObjectWrap;

  static auto constructor() -> Napi::FunctionReference&;

  std::shared_ptr<const prop::Spec> spec_;

}; // class PropSpecWrap

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Return the temporary solver property specification.
auto solverSpec(const Napi::CallbackInfo& info) -> Napi::Value;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Register the properties submodule on the addon exports.
void init_submodule(Napi::Env env, Napi::Object exports);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::gui::properties
