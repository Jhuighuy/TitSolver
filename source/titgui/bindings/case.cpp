/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <cstddef>
#include <filesystem>
#include <string>
#include <utility>
#include <variant>

#include <napi.h>
#include <nlohmann/json.hpp> // NOLINT(misc-include-cleaner)
#include <nlohmann/json_fwd.hpp>

#include "tit/core/exception.hpp"
#include "tit/prop/spec.hpp"
#include "tit/prop/tree.hpp"
#include "tit/prop/validation.hpp"
#include "titgui/bindings/case.hpp"
#include "titgui/bindings/utils.hpp"
#include "titwcsph/case.hpp"

namespace tit::gui::cases {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

using JSON = nlohmann::ordered_json;

auto string_arg(const Napi::CallbackInfo& info, std::size_t index)
    -> std::string {
  TIT_ENSURE(info.Length() > index, "Missing argument.");
  TIT_ENSURE(info[index].IsString(), "Argument must be a string.");
  return info[index].As<Napi::String>().Utf8Value();
}

auto resolve_string(Napi::Env env, const std::string& text) -> Napi::Value {
  return Napi::String::New(env, text);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto caseSpec(const Napi::CallbackInfo& info) -> Napi::Value {
  TIT_ENSURE(info.Length() == 0, "Unexpected arguments.");

  return enqueue(
      info.Env(),
      [] { return prop::spec_dump_json(*wcsph::make_case_spec()); },
      resolve_string);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto loadCaseTree(const Napi::CallbackInfo& info) -> Napi::Value {
  TIT_ENSURE(info.Length() == 1, "Expected one argument.");
  const std::filesystem::path path{string_arg(info, 0)};

  return enqueue(
      info.Env(),
      [path] { return prop::tree_dump_json(prop::tree_from_file(path)); },
      resolve_string);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto saveCaseTree(const Napi::CallbackInfo& info) -> Napi::Value {
  TIT_ENSURE(info.Length() == 2, "Expected two arguments.");
  const std::filesystem::path path{string_arg(info, 0)};
  auto tree_text = string_arg(info, 1);

  return enqueue(
      info.Env(),
      [path, tree_text = std::move(tree_text)] {
        prop::tree_dump_file(prop::tree_from_json(tree_text), path);
        return std::monostate{};
      },
      [](Napi::Env env, std::monostate) { return env.Undefined(); });
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto materializeCase(const Napi::CallbackInfo& info) -> Napi::Value {
  TIT_ENSURE(info.Length() == 1, "Expected one argument.");
  auto tree_text = string_arg(info, 0);

  return enqueue(
      info.Env(),
      [tree_text = std::move(tree_text)] {
        auto tree = prop::tree_from_json(tree_text);
        const auto spec = wcsph::make_case_spec();
        const auto context = prop::validate(*spec, tree);

        JSON result;
        result["tree"] = JSON::parse(prop::tree_dump_json(tree));

        auto issues = JSON::array();
        for (const auto& issue : context.issues()) {
          issues.push_back({
              {"code", std::string{prop::issue_code_to_string(issue.code)}},
              {"path", issue.path.string()},
              {"message", issue.message},
          });
        }
        result["issues"] = std::move(issues);

        auto namespaces = JSON::object();
        for (const auto& [ns, symbols] : context.namespaces()) {
          auto table = JSON::object();
          for (const auto& [id, path] : symbols) table[id] = path.string();
          namespaces[ns] = std::move(table);
        }
        result["namespaces"] = std::move(namespaces);

        return result.dump();
      },
      resolve_string);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void init_submodule(Napi::Env env, Napi::Object exports) {
  exports.Set("caseSpec", Napi::Function::New(env, caseSpec));
  exports.Set("loadCaseTree", Napi::Function::New(env, loadCaseTree));
  exports.Set("saveCaseTree", Napi::Function::New(env, saveCaseTree));
  exports.Set("materializeCase", Napi::Function::New(env, materializeCase));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::gui::cases
