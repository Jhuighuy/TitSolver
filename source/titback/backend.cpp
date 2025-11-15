/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// Note: False-positives from Crow internals on Fedora 43.
#if defined(__linux__) && defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wstringop-overflow"
#endif

#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

#include <crow/app.h>
#include <crow/http_request.h>
#include <crow/http_response.h>
#include <crow/websocket.h>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

#include "tit/core/checks.hpp"
#include "tit/core/env.hpp"
#include "tit/core/main.hpp"
#include "tit/core/type.hpp"
#include "tit/data/storage.hpp"

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TIT_IMPLEMENT_MAIN([](int /*argc*/, char** argv) {
  namespace json = nlohmann;

  const auto exe_dir = std::filesystem::path{argv[0]}.parent_path();
  const auto root_dir = exe_dir.parent_path();

  // Load the storage.
  const data::DataStorage storage{"particles.ttdb"};

  crow::SimpleApp app;

  // NOLINTNEXTLINE(modernize-type-traits)
  CROW_WEBSOCKET_ROUTE(app, "/ws")
      .onmessage([&storage](crow::websocket::connection& connection,
                            const std::string& data,
                            bool is_binary) {
        TIT_ASSERT(!is_binary, "Binary messages are not supported.");
        const auto request = json::json::parse(data);
        json::json response;
        response["status"] = "success";
        response["requestID"] = request["requestID"];
        const auto frame = storage.last_series().last_frame();
        for (const auto* var : {"r", "rho"}) {
          const auto r = frame.find_array(var);
          const auto r_data = r->read();
          response["result"][var] = std::span{
              safe_bit_ptr_cast<const double*>(r_data.data()),
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

  /// @todo Pass port as a command line argument.
  app.port(get_env<uint16_t>("TIT_BACKEND_PORT", 18080)).run();
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
