/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <cmath>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <memory>
#include <string>
#include <utility>

#include <napi.h>

#include "tit/core/exception.hpp"
#include "tit/core/float.hpp"
#include "tit/prop/issue.hpp"
#include "tit/prop/spec.hpp"
#include "tit/prop/tree.hpp"
#include "tit/prop/unit.hpp"
#include "tit/prop/validation.hpp"
#include "titgui/bindings/properties.hpp"

namespace tit::gui::properties {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto js_to_tree(const Napi::Value& value) -> prop::Tree;
auto tree_to_js(Napi::Env env, const prop::Tree& tree) -> Napi::Value;

auto js_to_tree(const Napi::Value& value) -> prop::Tree {
  if (value.IsNull() || value.IsUndefined()) return prop::Tree{};
  if (value.IsBoolean()) return prop::Tree{value.As<Napi::Boolean>().Value()};
  if (value.IsNumber()) {
    const auto number = value.As<Napi::Number>().DoubleValue();
    const auto min_int =
        static_cast<float64_t>(std::numeric_limits<std::int64_t>::min());
    const auto max_int =
        static_cast<float64_t>(std::numeric_limits<std::int64_t>::max());
    if (std::isfinite(number) && std::trunc(number) == number &&
        min_int <= number && number <= max_int) {
      return prop::Tree{static_cast<std::int64_t>(number)};
    }
    return prop::Tree{number};
  }
  if (value.IsString()) {
    return prop::Tree{value.As<Napi::String>().Utf8Value()};
  }
  if (value.IsArray()) {
    const auto array = value.As<Napi::Array>();
    auto result = prop::Tree::Array{};
    result.reserve(array.Length());
    for (std::uint32_t index = 0; index < array.Length(); index++) {
      result.push_back(js_to_tree(array.Get(index)));
    }
    return prop::Tree{std::move(result)};
  }
  TIT_ENSURE(value.IsObject(), "Expected JSON-compatible property tree.");

  const auto object = value.As<Napi::Object>();
  const auto keys = object.GetPropertyNames();
  auto result = prop::Tree::Map{};
  for (std::uint32_t index = 0; index < keys.Length(); index++) {
    const auto key = keys.Get(index).As<Napi::String>().Utf8Value();
    result.emplace(key, js_to_tree(object.Get(key)));
  }
  return prop::Tree{std::move(result)};
}

auto tree_to_js(Napi::Env env, const prop::Tree& tree) -> Napi::Value {
  if (tree.is_null()) return env.Null();
  if (tree.is_bool()) return Napi::Boolean::New(env, tree.as_bool());
  if (tree.is_int()) {
    return Napi::Number::New(env, static_cast<double>(tree.as_int()));
  }
  if (tree.is_real()) return Napi::Number::New(env, tree.as_real());
  if (tree.is_string()) return Napi::String::New(env, tree.as_string());
  if (tree.is_array()) {
    const auto& tree_array = tree.as_array();
    auto array = Napi::Array::New(env, tree_array.size());
    for (std::uint32_t index = 0; index < tree_array.size(); index++) {
      array.Set(index, tree_to_js(env, tree_array[index]));
    }
    return array;
  }
  if (tree.is_map()) {
    auto object = Napi::Object::New(env);
    for (const auto& key : tree.keys()) {
      object.Set(key, tree_to_js(env, tree.get(key)));
    }
    return object;
  }
  std::unreachable();
}

auto json_text_to_js(Napi::Env env, const std::string& json) -> Napi::Value {
  return tree_to_js(env, prop::tree_from_json(json));
}

auto namespace_table_to_js(Napi::Env env,
                           const prop::NamespaceTable& namespace_table)
    -> Napi::Object {
  auto result = Napi::Object::New(env);
  for (const auto& [ns, namespace_symbols] : namespace_table) {
    auto js_symbols = Napi::Object::New(env);
    for (const auto& [value, path] : namespace_symbols) {
      js_symbols.Set(value, path.string());
    }
    result.Set(ns, js_symbols);
  }
  return result;
}

auto issue_to_js(Napi::Env env, const prop::Issue& issue) -> Napi::Object {
  auto object = Napi::Object::New(env);
  object.Set(
      "code",
      Napi::String::New(env,
                        std::string{prop::issue_code_to_string(issue.code)}));
  object.Set("path", issue.path.string());
  object.Set("message", issue.message);
  return object;
}

auto make_mock_solver_spec() -> prop::SpecPtr {
  return prop::RecordSpec{}
      .field("simulation",
             "Simulation",
             prop::RecordSpec{}
                 .field("title",
                        "Title",
                        prop::StringSpec{}.default_value("Untitled"))
                 .field("gravity",
                        "Gravity",
                        prop::RealSpec{}.min(0.0).default_value(9.81).unit(
                            prop::Unit{"m/s^2"}))
                 .field("end_time", "End Time", prop::RealSpec{}.min(0.0))
                 .field("verbose", "Verbose", prop::BoolSpec{}))
      .field(
          "time_integration",
          "Time Integration",
          prop::VariantSpec{}
              .option("explicit_euler",
                      "Explicit Euler",
                      prop::RecordSpec{}.field(
                          "time_step",
                          "Time Step",
                          prop::RealSpec{}.min(0.0).default_value(0.001).unit(
                              prop::Unit{"s"})))
              .option(
                  "runge_kutta_2",
                  "Runge-Kutta 2",
                  prop::RecordSpec{}
                      .field(
                          "time_step",
                          "Time Step",
                          prop::RealSpec{}.min(0.0).default_value(0.001).unit(
                              prop::Unit{"s"}))
                      .field("adaptive", "Adaptive", prop::BoolSpec{}))
              .default_value("explicit_euler"))
      .field(
          "materials",
          "Materials",
          prop::ArraySpec{}.item(
              prop::RecordSpec{}
                  .field("id", "ID", prop::SymbolSpec{"materials"})
                  .field("density",
                         "Density",
                         prop::RealSpec{}.min(0.0).unit(prop::Unit{"kg/m^3"}))))
      .field(
          "bodies",
          "Bodies",
          prop::ArraySpec{}.item(
              prop::RecordSpec{}
                  .field("name", "Name", prop::StringSpec{})
                  .field("material", "Material", prop::RefSpec{"materials"})))
      .field("output",
             "Output",
             prop::RecordSpec{}
                 .field("directory",
                        "Directory",
                        prop::StringSpec{}.default_value("output"))
                 .field("format",
                        "Format",
                        prop::EnumSpec{}
                            .option("vtk", "VTK")
                            .option("hdf5", "HDF5")
                            .option("csv", "CSV")
                            .default_value("vtk"))
                 .field("interval",
                        "Write Interval",
                        prop::IntSpec{}.min(1).default_value(100)))
      .box();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto PropTreeWrap::constructor() -> Napi::FunctionReference& {
  static Napi::FunctionReference constructor_;
  return constructor_;
}

void PropTreeWrap::init(Napi::Env env, Napi::Object exports) {
  auto ctor = DefineClass( //
      env,
      "PropTree",
      {
          StaticMethod("fromJSON", &PropTreeWrap::fromJSON),
          StaticMethod("fromJSONText", &PropTreeWrap::fromJSONText),
          StaticMethod("fromFile", &PropTreeWrap::fromFile),
          InstanceMethod("toJSON", &PropTreeWrap::toJSON),
          InstanceMethod("toJSONText", &PropTreeWrap::toJSONText),
          InstanceMethod("saveToFile", &PropTreeWrap::saveToFile),
      });
  constructor() = Persistent(ctor);
  constructor().SuppressDestruct();
  exports.Set("PropTree", ctor);
}

auto PropTreeWrap::New(Napi::Env /*env*/, prop::Tree tree) -> Napi::Object {
  auto object = constructor().New({});
  auto* const self = Napi::ObjectWrap<PropTreeWrap>::Unwrap(object);
  self->tree_ = std::move(tree);
  return object;
}

auto PropTreeWrap::UnwrapTree(const Napi::Value& value) -> prop::Tree& {
  TIT_ENSURE(value.IsObject(), "Argument must be a PropTree.");
  const auto object = value.As<Napi::Object>();
  TIT_ENSURE(object.InstanceOf(constructor().Value()),
             "Argument must be a PropTree.");
  return Napi::ObjectWrap<PropTreeWrap>::Unwrap(object)->tree_;
}

auto PropTreeWrap::fromJSON(const Napi::CallbackInfo& info) -> Napi::Value {
  TIT_ENSURE(info.Length() == 1, "Expected one argument.");
  return New(info.Env(), js_to_tree(info[0]));
}

auto PropTreeWrap::fromJSONText(const Napi::CallbackInfo& info) -> Napi::Value {
  TIT_ENSURE(info.Length() == 1, "Expected one argument.");
  TIT_ENSURE(info[0].IsString(), "Argument must be a string.");
  return New(info.Env(),
             prop::tree_from_json(info[0].As<Napi::String>().Utf8Value()));
}

auto PropTreeWrap::fromFile(const Napi::CallbackInfo& info) -> Napi::Value {
  TIT_ENSURE(info.Length() == 1, "Expected one argument.");
  TIT_ENSURE(info[0].IsString(), "Argument must be a string.");
  const std::filesystem::path path{info[0].As<Napi::String>().Utf8Value()};
  return New(info.Env(), prop::tree_from_file(path));
}

auto PropTreeWrap::toJSON(const Napi::CallbackInfo& info) -> Napi::Value {
  TIT_ENSURE(info.Length() == 0, "Unexpected arguments.");
  return tree_to_js(info.Env(), tree_);
}

auto PropTreeWrap::toJSONText(const Napi::CallbackInfo& info) -> Napi::Value {
  TIT_ENSURE(info.Length() == 0, "Unexpected arguments.");
  return Napi::String::New(info.Env(), prop::tree_dump_json(tree_));
}

auto PropTreeWrap::saveToFile(const Napi::CallbackInfo& info) -> Napi::Value {
  TIT_ENSURE(info.Length() == 1, "Expected one argument.");
  TIT_ENSURE(info[0].IsString(), "Argument must be a string.");
  const std::filesystem::path path{info[0].As<Napi::String>().Utf8Value()};
  prop::tree_dump_file(tree_, path);
  return info.Env().Undefined();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto PropSpecWrap::constructor() -> Napi::FunctionReference& {
  static Napi::FunctionReference constructor_;
  return constructor_;
}

void PropSpecWrap::init(Napi::Env env) {
  auto ctor = DefineClass( //
      env,
      "PropSpec",
      {
          InstanceMethod("toJSON", &PropSpecWrap::toJSON),
          InstanceMethod("materialize", &PropSpecWrap::materialize),
      });
  constructor() = Persistent(ctor);
  constructor().SuppressDestruct();
}

auto PropSpecWrap::New(Napi::Env /*env*/, prop::SpecPtr spec) -> Napi::Object {
  auto object = constructor().New({});
  auto* const self = Napi::ObjectWrap<PropSpecWrap>::Unwrap(object);
  self->spec_ = std::shared_ptr<const prop::Spec>{std::move(spec)};
  return object;
}

auto PropSpecWrap::toJSON(const Napi::CallbackInfo& info) -> Napi::Value {
  TIT_ENSURE(info.Length() == 0, "Unexpected arguments.");
  TIT_ENSURE(spec_ != nullptr, "Property spec is not initialized.");
  return json_text_to_js(info.Env(), prop::spec_dump_json(*spec_));
}

auto PropSpecWrap::materialize(const Napi::CallbackInfo& info) -> Napi::Value {
  TIT_ENSURE(info.Length() == 1, "Expected one argument.");
  TIT_ENSURE(spec_ != nullptr, "Property spec is not initialized.");

  auto tree = prop::Tree{PropTreeWrap::UnwrapTree(info[0])};
  auto context = prop::validate(*spec_, tree);

  auto object = Napi::Object::New(info.Env());
  object.Set("tree", PropTreeWrap::New(info.Env(), std::move(tree)));

  const auto context_issues = context.issues();
  auto issues = Napi::Array::New(info.Env(), context_issues.size());
  for (std::uint32_t index = 0; index < context_issues.size(); index++) {
    issues.Set(index, issue_to_js(info.Env(), context_issues[index]));
  }
  object.Set("issues", issues);
  object.Set("namespaceTable",
             namespace_table_to_js(info.Env(), context.namespaces()));

  return object;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto solverSpec(const Napi::CallbackInfo& info) -> Napi::Value {
  TIT_ENSURE(info.Length() == 0, "Unexpected arguments.");
  return PropSpecWrap::New(info.Env(), make_mock_solver_spec());
}

void init_submodule(Napi::Env env, Napi::Object exports) {
  PropTreeWrap::init(env, exports);
  PropSpecWrap::init(env);
  exports.Set("solverSpec", Napi::Function::New(env, solverSpec));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::gui::properties
