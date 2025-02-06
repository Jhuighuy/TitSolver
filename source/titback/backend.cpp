/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <bit>
#include <filesystem>
#include <string>
#include <vector>

#include <crow/app.h>
#include <crow/http_request.h>
#include <crow/http_response.h>
#include <crow/websocket.h>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/cmd.hpp"
#include "tit/core/sys/utils.hpp"

#include "tit/data/storage.hpp"

namespace tit::back {
namespace {

namespace json = nlohmann;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto run_backend(CmdArgs /*args*/) -> int {
  const auto exe_dir = exe_path().parent_path();
  const auto root_dir = exe_dir.parent_path();

  // Load the storage.
  const data::DataStorage storage{"particles.ttdb"};

  crow::SimpleApp app;

  CROW_WEBSOCKET_ROUTE(app, "/ws")
      .onmessage([&storage](crow::websocket::connection& connection,
                            const std::string& data,
                            bool is_binary) {
        TIT_ASSERT(!is_binary, "Binary messages are not supported.");
        const auto request = json::json::parse(data);
        json::json response;
        response["status"] = "success";
        response["requestID"] = request["requestID"];
        const auto varyings = storage.last_series().last_time_step().varyings();
        for (const auto* var : {"r", "rho"}) {
          const auto r = varyings.find_array(var);
          std::vector<byte_t> r_data(r->size() * r->type().width());
          r->open_read()->read(r_data);
          response["result"][var] = std::span{
              std::bit_cast<const double*>(r_data.data()),
              r_data.size() / sizeof(double),
          };
        }
        connection.send_text(response.dump());
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
  app.port(get_env<uint16_t>("TIT_BACKEND_PORT", 18080)).run();

  return 0;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit::back

TIT_IMPLEMENT_MAIN(back::run_backend)
