/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/build_info.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/print.hpp"
#include "tit/core/str.hpp"
#include "tit/prop/app.hpp"
#include "tit/prop/spec.hpp"
#include "tit/prop/tree.hpp"

namespace tit::prop {
namespace {

constexpr std::string_view DEFAULT_CONFIG_FILE = "config.yaml";

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto spec_suffix(const Spec& spec) -> std::string {
  std::vector<std::string> pieces;

  const auto [default_value, min_value, max_value, options] = spec_info(spec);
  if (default_value.has_value()) {
    pieces.push_back(std::format("Default: {}.", default_value.value()));
  }
  if (min_value.has_value() || max_value.has_value()) {
    pieces.push_back(std::format("Range: {}..{}.",
                                 min_value.value_or(""),
                                 max_value.value_or("")));
  }
  if (!options.empty()) {
    pieces.push_back(std::format("Values: {}.", str_join(options, ", ")));
  }

  return str_join(pieces, " ");
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void print_help(const AppSpec& spec) {
  println();
  println("{} - {}", spec.name(), spec.description());
  println();
  println("USAGE");
  println();
  println("  {} [OPTIONS]", spec.name());
  println();
  println("OPTIONS");
  println();
  println("  --help, -h");
  println("    Print this help and exit.");
  println();
  println("  --version, -v");
  println("    Print the application version and exit.");
  println();
  println("  --config <file>");
  println("    Load configuration from <file>. May be repeated.");
  println();
  println("  --init <file>");
  println("    Write a template configuration file and exit.");
  println("    Default: {}.", str_quoted(DEFAULT_CONFIG_FILE));

  bool advanced_skipped = false;
  for (const auto& field : spec.fields()) {
    if (!spec_type_is_scalar(field.spec->type())) {
      advanced_skipped = true;
      continue;
    }

    println();
    println("  --{} <{}>", field.id, spec_type_to_string(field.spec->type()));
    println("    {}.", field.name);
    if (const auto suffix = spec_suffix(*field.spec); !suffix.empty()) {
      println("    {}", suffix);
    }
  }

  if (advanced_skipped) {
    // clang-format off
    println();
    println("  Advanced and nested options are configured in config files. Use --init <file>,");
    println("  or refer to the user manual or current config to override them.");
    // clang-format on
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

enum class ServiceActionType : uint8_t {
  Help,
  HelpJSON,
  Version,
  Init,
};

struct ServiceAction final {
  ServiceActionType type;
  std::string_view flag;
  std::optional<std::filesystem::path> path;
};

auto find_service_action(const std::vector<std::string_view>& args)
    -> std::optional<ServiceAction> {
  std::optional<ServiceAction> action;

  // Register a service action. Only one action is allowed.
  const auto register_action = [&action](ServiceActionType type,
                                         std::string_view flag) {
    TIT_ENSURE(!action.has_value(),
               "Conflicting service flags: '{}' and '{}'. "
               "Use only one service action.",
               action->flag,
               flag);

    action.emplace(type, flag);
  };

  for (size_t index = 1; index < args.size(); ++index) {
    const auto arg = args[index];

    // Parse service flags.
    if (arg == "--help" || arg == "-h") {
      register_action(ServiceActionType::Help, arg);
      continue;
    }
    if (arg == "--help-json") {
      register_action(ServiceActionType::HelpJSON, arg);
      continue;
    }
    if (arg == "--version" || arg == "-v") {
      register_action(ServiceActionType::Version, arg);
      continue;
    }
    if (arg == "--init") {
      register_action(ServiceActionType::Init, arg);
      std::filesystem::path path{DEFAULT_CONFIG_FILE};
      if (const auto next = index + 1;
          next < args.size() && !args[next].starts_with("--")) {
        path = args[next];
        index = next;
      }
      action->path = std::move(path); // NOLINT(*-optional-access)
      continue;
    }

    // Service action conflicts with other flags.
    TIT_ENSURE(!action.has_value(),
               "Service flag '{}' must be used alone.",
               action->flag);
  }

  return action;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto parse_args(const AppSpec& spec, const std::vector<std::string_view>& args)
    -> Tree {
  TIT_ALWAYS_ASSERT(!args.empty(), "Command-line arguments are empty!");

  // Find and execute the service action, if any.
  if (const auto action = find_service_action(args); action.has_value()) {
    using enum ServiceActionType;
    switch (action->type) {
      case Help: {
        print_help(spec);
        std::exit(0);
      }
      case HelpJSON: {
        spec_dump_json(spec, std::cout);
        std::exit(0);
      }
      case Version: {
        println("{} {}", spec.name(), build_info::version());
        std::exit(0);
      }
      case Init: {
        const auto& path = action->path.value(); // NOLINT(*-optional-access)
        TIT_ENSURE(!std::filesystem::exists(path),
                   "Init file '{}' already exists.",
                   path.string());

        std::ofstream stream{path};
        TIT_ENSURE(stream.is_open(),
                   "Cannot create init file '{}'.",
                   path.string());

        spec_dump_yaml_config(spec, stream);
        TIT_ENSURE(stream.good(),
                   "Failed to write init file '{}'.",
                   path.string());

        std::exit(0);
      }
    }
    std::unreachable();
  }

  // Parse and validate actual arguments.
  // Note: `tree_from_args` does not skip the first argument.
  auto tree = tree_from_args(std::vector(args.begin() + 1, args.end()));
  spec.validate(tree, "");
  return tree;
}

} // namespace

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void exec_app(App& app, int argc, char** argv) {
  TIT_ALWAYS_ASSERT(argc > 0 && argv != nullptr, "Invalid app arguments.");
  try {
    const auto spec = app.spec();
    app.run(parse_args(spec, std::vector<std::string_view>(argv, argv + argc)));
  } catch (const tit::Exception& exc) {
    eprintln("Error: {}", exc.what());
    std::exit(1);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::prop
