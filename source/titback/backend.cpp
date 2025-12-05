/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <ranges>
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
#include "tit/data/storage.hpp"

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace {

auto encode_base64(const std::vector<std::byte>& data) -> std::string {
  static constexpr auto alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                   "abcdefghijklmnopqrstuvwxyz"
                                   "0123456789+/";

  const size_t len = data.size();

  std::string result;
  result.reserve(((len + 2) / 3) * 4);

  size_t i = 0;
  while (i + 3 <= len) {
    const auto triple =
        (uint32_t(std::to_integer<unsigned char>(data[i])) << 16) |
        (uint32_t(std::to_integer<unsigned char>(data[i + 1])) << 8) |
        (uint32_t(std::to_integer<unsigned char>(data[i + 2])));

    result.push_back(alphabet[(triple >> 18) & 0x3F]);
    result.push_back(alphabet[(triple >> 12) & 0x3F]);
    result.push_back(alphabet[(triple >> 6) & 0x3F]);
    result.push_back(alphabet[triple & 0x3F]);

    i += 3;
  }

  if (i + 1 == len) {
    const auto triple =
        (uint32_t(std::to_integer<unsigned char>(data[i])) << 16);

    result.push_back(alphabet[(triple >> 18) & 0x3F]);
    result.push_back(alphabet[(triple >> 12) & 0x3F]);
    result.push_back('=');
    result.push_back('=');
  } else if (i + 2 == len) {
    const auto triple =
        (uint32_t(std::to_integer<unsigned char>(data[i])) << 16) |
        (uint32_t(std::to_integer<unsigned char>(data[i + 1])) << 8);

    result.push_back(alphabet[(triple >> 18) & 0x3F]);
    result.push_back(alphabet[(triple >> 12) & 0x3F]);
    result.push_back(alphabet[(triple >> 6) & 0x3F]);
    result.push_back('=');
  }

  return result;
}

} // namespace

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TIT_IMPLEMENT_MAIN([](int /*argc*/, char** argv) {
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

        const auto& message = request["message"];
        const auto type = message["type"].get<std::string>();
        if (type == "num-frames") {
          response["result"] = storage.last_series().num_frames();
        } else if (type == "frame") {
          const auto frame_index = message["index"].get<std::size_t>();
          const auto frame =
              (storage.last_series().frames() | std::ranges::to<std::vector>())
                  .at(frame_index);

          for (const auto& array : frame.arrays()) {
            std::vector<std::byte> bytes(array.size() * array.type().width());
            array.read(std::span{bytes});

            json::json field_result;
            field_result["kind"] = array.type().kind().name();
            field_result["data"] = encode_base64(bytes);

            response["result"][array.name()] = field_result;
          }
        } else {
          response["status"] = "error";
          response["error"] = "Unknown message type: " + type;
        }

        connection.send_text(response.dump());
      });

  CROW_ROUTE(app, "/")
  ([&root_dir](const crow::request& /*request*/, crow::response& response) {
    const auto index_html = root_dir / "lib" / "gui" / "index.html";
    response.set_static_file_info_unsafe(index_html.native());
    response.end();
  });
  CROW_ROUTE(app, "/<path>")
  ([&root_dir](const crow::request& /*request*/,
               crow::response& response,
               const std::filesystem::path& file_name) {
    auto file_path = root_dir / "lib" / "gui" / file_name;
    if (std::filesystem::is_directory(file_path)) file_path /= "index.html";
    response.set_static_file_info_unsafe(file_path.native());
    response.end();
  });

  /// @todo Pass port as a command line argument.
  app.port(get_env<uint16_t>("TIT_BACKEND_PORT", 18080)).run();
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
