/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <cstddef>
#include <filesystem>
#include <optional>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

#include "tit/core/exception.hpp"
#include "tit/core/main.hpp"
#include "tit/core/str.hpp"
#include "tit/data/legacy_ttdb.hpp"

namespace tit::convert {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

struct Options final {
  std::filesystem::path source;
  std::filesystem::path destination;
  std::optional<std::size_t> series_index;
};

auto parse_options(std::span<char*> arguments) -> Options {
  std::optional<std::size_t> series_index;
  std::vector<std::filesystem::path> paths;
  for (std::size_t index = 1; index < arguments.size(); ++index) {
    const std::string_view argument{arguments[index]};
    if (argument == "--series") {
      TIT_ENSURE(index + 1 < arguments.size(),
                 "Option '--series' requires a zero-based index.");
      const auto value = str_to<std::size_t>(arguments[++index]);
      TIT_ENSURE(value.has_value(),
                 "Option '--series' requires a zero-based index.");
      series_index = value;
    } else {
      paths.emplace_back(argument);
    }
  }
  TIT_ENSURE(paths.size() == 2,
             "Usage: titconvert [--series INDEX] SOURCE.ttdb "
             "DESTINATION.tit-run");
  return {.source = std::move(paths[0]),
          .destination = std::move(paths[1]),
          .series_index = series_index};
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void run(std::span<char*> arguments) {
  const auto options = parse_options(arguments);
  data::convert_ttdb(options.source, options.destination, options.series_index);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit::convert

TIT_IMPLEMENT_MAIN([](int argc, char** argv) {
  convert::run(std::span<char*>{argv, static_cast<std::size_t>(argc)});
});
