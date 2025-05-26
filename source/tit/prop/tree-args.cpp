/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/str.hpp"
#include "tit/prop/tree.hpp"

namespace tit::prop {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto tree_from_args(const std::vector<std::string_view>& args) -> Tree {
  Tree result{Tree::Map{}};
  for (size_t index = 0; index < args.size();) {
    // Get the current argument.
    auto arg = args[index];
    TIT_ENSURE(arg.size() >= 3 && arg.starts_with("--"),
               "Invalid argument '{}': expected '--key' or '--key=value'.",
               arg);
    arg = arg.substr(2);

    // Parse the argument into a key and value.
    std::string_view key;
    std::optional<std::string_view> value;
    if (const auto eq = arg.find('='); eq != std::string_view::npos) {
      key = arg.substr(0, eq);
      value = arg.substr(eq + 1);
      index += 1;
    } else {
      key = arg;
      if (const auto next = index + 1;
          next < args.size() && !args[next].starts_with("--")) {
        value = args[next];
        index += 2;
      } else {
        index += 1;
      }
    }

    // Validate the key.
    for (const auto seg : str_split(key, '.')) {
      TIT_ENSURE(!seg.empty(), "'--{}': empty identifier segment.", key);
      TIT_ENSURE(str_is_identifier(seg),
                 "'--{}': '{}' is not a valid identifier.",
                 key,
                 seg);
    }

    // Dispatch the argument.
    if (key == "config") {
      TIT_ENSURE(value.has_value(), "'--config' requires a file argument.");
      result.merge(tree_from_file(std::filesystem::path{value.value()}));
    } else {
      result.get_path(key) = value.has_value() ?
                                 Tree{std::string{value.value()}} :
                                 Tree{true}; // treat boolean flags as true.
    }
  }
  return result;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::prop
