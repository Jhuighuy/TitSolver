/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <cstddef>
#include <cstring>
#include <filesystem>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include <crow/app.h>
#include <crow/http_request.h>
#include <crow/http_response.h>
#include <crow/websocket.h>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

#include "tit/core/assert.hpp"
#include "tit/core/basic_types.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/main.hpp"
#include "tit/core/str.hpp"
#include "tit/core/type.hpp"
#include "tit/data/storage.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void backend_main(int argc, char** argv) {
  namespace json = nlohmann;

  const auto exe_dir = std::filesystem::path{argv[0]}.parent_path();
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
        const auto varyings = storage.last_series().last_frame();
        for (const auto* var : {"r", "rho"}) {
          const auto r = varyings.find_array(var);
          std::vector<std::byte> r_data(r->size() * r->type().width());
          r->read(std::span{r_data});
          response["result"][var] = std::span{
              safe_bit_ptr_cast<const double*>(std::as_const(r_data).data()),
              r_data.size() / sizeof(double),
          };
        }
        connection.send_text(response.dump());
      });

  CROW_ROUTE(app, "/")
  ([&root_dir](const crow::request& /*request*/, crow::response& response) {
    const auto index_html = root_dir / "lib" / "frontend" / "index.html";
    response.set_static_file_info_unsafe(index_html.native());
    response.end();
  });

  CROW_ROUTE(app, "/<path>")
  ([&root_dir](const crow::request& /*request*/,
               crow::response& response,
               const std::filesystem::path& file_name) {
    auto file_path = root_dir / "lib" / "frontend" / file_name;
    if (std::filesystem::is_directory(file_path)) file_path /= "index.html";
    response.set_static_file_info_unsafe(file_path.native());
    response.end();
  });

  uint16_t port = 18080;
  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], "--port") == 0) {
      TIT_ENSURE(i + 1 < argc, "Missing port number!");
      const auto p = str_to<uint16_t>(argv[i + 1]);
      TIT_ENSURE(p.has_value(), "Invalid port number!");
      port = p.value();
      break;
    }
  }

  app.port(port).run();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit

TIT_IMPLEMENT_MAIN(&backend_main);
