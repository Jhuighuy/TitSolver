/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <cstring>
#include <filesystem>
#include <span>
#include <string>
#include <utility>

#include <crow/app.h>
#include <crow/http_request.h>
#include <crow/http_response.h>
#include <crow/websocket.h>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/cmd.hpp"
#include "tit/core/sys/utils.hpp"

#include "tit/py/error.hpp"
#include "tit/py/gil.hpp"
#include "tit/py/interpreter.hpp"
#include "tit/py/mapping.hpp"
#include "tit/py/module.hpp"
#include "tit/py/object.hpp"

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
  const std::span argspan{args.argv(), static_cast<size_t>(args.argc())};
  if (argspan.size() >= 3 && std::strcmp(argspan[1], "-c") == 0) {
    const auto* const statement = argspan[2];
    return interpreter.exec(statement) ? 0 : 1;
  }
  if (argspan.size() >= 2) {
    const auto* const file_name = argspan[1];
    return interpreter.exec_file(file_name) ? 0 : 1;
  }

  crow::SimpleApp app;

  CROW_WEBSOCKET_ROUTE(app, "/ws")
      .onmessage([&interpreter](crow::websocket::connection& connection,
                                const std::string& data,
                                bool is_binary) { //
        TIT_ASSERT(!is_binary, "Binary messages are not supported.");
        const py::AcquireGIL acquire_gil{};
        const auto json = py::import_("json");
        const py::Dict response;
        try {
          const auto request = py::expect<py::Dict>(json.attr("loads")(data));
          response["requestID"] = request.at("requestID"); /// @todo Fix it!
          const auto expr = py::extract<std::string>(request["expression"]);
          response["result"] = interpreter.eval(expr);
          response["status"] = "success";
        } catch (const py::ErrorException& e) {
          const py::Dict error;
          error["type"] = py::type(e.error()).fully_qualified_name();
          error["error"] = py::str(e.error());
          if (const auto tb = e.error().traceback(); tb) {
            response["traceback"] = py::expect<py::Traceback>(tb).render();
          }
          response["result"] = error;
          response["status"] = "error";
        }
        connection.send_text(
            py::extract<std::string>(json.attr("dumps")(response)));
      });

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

  /// @todo Pass port as a command line argument.
  const py::ReleaseGIL release_gil{};
  app.port(get_env<uint16_t>("TIT_BACKEND_PORT", 18080)).run();

  return 0;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit::back

TIT_IMPLEMENT_MAIN(back::run_backend)
