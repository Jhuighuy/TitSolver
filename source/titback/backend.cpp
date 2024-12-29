/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <filesystem>
#include <format>
#include <string>
#include <utility>

#include <crow/app.h>
#include <crow/http_request.h>
#include <crow/http_response.h>
#include <cxxopts.hpp>

#include "tit/core/io.hpp"
#include "tit/core/main_func.hpp"
#include "tit/core/sys/utils.hpp"

#include "titback/interpreter.hpp"

namespace tit::back {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto run_backend(CmdArgs args) -> int {
  // Parse the command line arguments.
  cxxopts::Options options("titback", "Backend for the Tit Solver.");
  options.add_options()( //
      "c",
      "Execute the given Python statement.",
      cxxopts::value<std::string>())( //
      "file",
      "Execute the given Python file.",
      cxxopts::value<std::string>())( //
      "h,help",
      "Print help message.");
  options.parse_positional({"file"});
  const auto result = options.parse(static_cast<int>(args.size()), args.data());
  if (result.count("help") != 0) {
    println("{}", options.help());
    return 0;
  }

  // Setup paths.
  const auto exe_dir = exe_path().parent_path();
  const auto root_dir = exe_dir.parent_path();

  // Setup the Python interpreter.
  PythonConfig config;
  config.set_home((root_dir / "python").c_str());
  config.set_prog_name("titback");
  config.set_cmd_args(args);
  const PythonInterpreter interpreter{std::move(config)};
  interpreter.append_path((root_dir / "python").c_str());

  // Execute the Python statement or file.
  if (result.count("c") != 0) {
    const auto statement = result["c"].as<std::string>();
    return interpreter.exec(statement.c_str()) ? 0 : 1;
  }
  if (result.count("file") != 0) {
    const auto file_name = result["file"].as<std::string>();
    return interpreter.exec_file(file_name.c_str()) ? 0 : 1;
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
