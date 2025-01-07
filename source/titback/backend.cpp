/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <cstring>
#include <filesystem>
#include <string>
#include <utility>

#include <crow/app.h>
#include <crow/http_request.h>
#include <crow/http_response.h>

#include "tit/core/main_func.hpp"
#include "tit/core/sys/utils.hpp"

#include "tit/py/interpreter.hpp"

namespace tit::back {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto run_backend(CmdArgs args) -> int {
  // Setup paths.
  const auto exe_dir = exe_path().parent_path();
  const auto root_dir = exe_dir.parent_path();

  // Setup the Python interpreter.
  /// @todo We should check for presence of the home directory.
  const auto home_dir = root_dir / "python";
  py::embed::Config config;
  config.set_home(home_dir.c_str());
  config.set_prog_name("titback");
  config.set_cmd_args(args);
  const py::embed::Interpreter interpreter{std::move(config)};
  interpreter.append_path(home_dir.c_str());

  // Execute the Python statement or file.
  /// @todo Proper command line parsing.
  if (args.size() >= 3 && std::strcmp(args[1], "-c") == 0) {
    const auto* const statement = args[2];
    return interpreter.exec(statement) ? 0 : 1;
  }
  if (args.size() >= 2) {
    const auto* const file_name = args[1];
    return interpreter.exec_file(file_name) ? 0 : 1;
  }

  crow::SimpleApp app;
  CROW_ROUTE(app, "/")
  ([&root_dir](const crow::request& /*request*/, crow::response& response) {
    const auto index_html = root_dir / "frontend" / "index.html";
    response.set_static_file_info_unsafe(index_html.native());
    response.end();
  });
  CROW_ROUTE(app, "/<path>")
  ([&root_dir](const crow::request& /*request*/,
               crow::response& response,
               const std::filesystem::path& file_name) {
    auto file_path = root_dir / "frontend" / file_name;
    if (std::filesystem::is_directory(file_path)) file_path /= "index.html";
    response.set_static_file_info_unsafe(file_path.native());
    response.end();
  });

  app.port(18080).run();

  return 0;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit::back

TIT_IMPLEMENT_MAIN(back::run_backend)
