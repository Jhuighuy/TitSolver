/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// Note: False-positives from Crow internals on Fedora 43.
#if defined(__linux__) && defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wstringop-overflow"
#endif

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <format>
#include <iterator>
#include <mutex>
#include <ranges>
#include <span>
#include <string>
#include <vector>

#include <crow/app.h>
#include <crow/http_request.h>
#include <crow/http_response.h>
#include <crow/utility.h>
#include <crow/websocket.h>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

#include "tit/core/checks.hpp"
#include "tit/core/env.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/main.hpp"
#include "tit/core/type.hpp"
#include "tit/data/storage.hpp"

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TIT_IMPLEMENT_MAIN([](int /*argc*/, char** argv) {
  namespace json = nlohmann;

  const auto exe_dir = std::filesystem::path{argv[0]}.parent_path();
  const auto root_dir = exe_dir.parent_path();
  const auto gui_dir = root_dir / "lib" / "gui";

  // ---------------------------------------------------------------------------
  //
  // Application state
  //

  crow::SimpleApp app;

  const data::DataStorage storage{"particles.ttdb"};

  // ---------------------------------------------------------------------------
  //
  // WebSocket connection
  //

  std::mutex connection_mutex;

  crow::websocket::connection* current_connection = nullptr;

  std::vector<std::string> pending_messages;

  const auto send_response = [&connection_mutex,
                              &current_connection,
                              &pending_messages](const std::string& response) {
    const std::scoped_lock lock{connection_mutex};
    if (current_connection != nullptr) {
      current_connection->send_text(response);
    } else {
      pending_messages.push_back(response);
    }
  };

  CROW_WEBSOCKET_ROUTE(app, "/ws")
      .onaccept([&connection_mutex,
                 &current_connection](const crow::request& /*request*/,
                                      void** /*user_data*/) {
        // Accept only one connection.
        const std::scoped_lock lock{connection_mutex};
        return current_connection == nullptr;
      })
      .onopen([&connection_mutex, //
               &current_connection,
               &pending_messages](crow::websocket::connection& connection) {
        const std::scoped_lock lock{connection_mutex};

        TIT_ALWAYS_ASSERT(current_connection == nullptr,
                          "Only one connection is allowed.");

        current_connection = &connection;

        for (const auto& message : pending_messages) {
          current_connection->send_text(message);
        }

        pending_messages.clear();
      })
      .onclose([&connection_mutex, //
                &current_connection](crow::websocket::connection& connection,
                                     const std::string& /*reason*/,
                                     uint16_t /*code*/) {
        const std::scoped_lock lock{connection_mutex};

        TIT_ALWAYS_ASSERT(current_connection == &connection,
                          "Unexpected connection.");

        current_connection = nullptr;
      })
      .onmessage([&send_response,
                  &storage](crow::websocket::connection& /*connection*/,
                            const std::string& raw_request,
                            bool is_binary) {
        TIT_ALWAYS_ASSERT(!is_binary, "Binary messages are not supported.");

        const auto request = json::json::parse(raw_request);
        const auto& message = request["message"];
        const auto type = message["type"].get<std::string>();

        json::json response{
            {"requestID", request["requestID"]},
            {"status", "success"},
        };

        // ---------------------------------------------------------------------
        if (type == "num-frames") {
          response["result"] = storage.last_series().num_frames();
          send_response(response.dump());
          return;
        }

        // ---------------------------------------------------------------------
        if (type == "frame") {
          const auto frame_index = message["index"].get<std::size_t>();
          auto frames = storage.last_series().frames();
          auto frame_iter = std::ranges::begin(frames);
          const auto frames_end = std::ranges::end(frames);
          std::ranges::advance(frame_iter, frame_index, frames_end);
          TIT_ENSURE(frame_iter != frames_end, "Frame index out of bounds.");
          const auto& frame = *frame_iter;

          json::json result;
          for (const auto& array : frame.arrays()) {
            std::vector<std::byte> bytes(array.size() * array.type().width());
            array.read(std::span{bytes});

            result[array.name()] = json::json{
                {"kind", array.type().kind().name()},
                {"data",
                 crow::utility::base64encode(
                     safe_bit_ptr_cast<const uint8_t*>(bytes.data()),
                     bytes.size())},
            };
          }

          response["result"] = result;
          send_response(response.dump());
          return;
        }

        // ---------------------------------------------------------------------
        response["status"] = "error";
        response["error"] = std::format("Unknown message type: '{}'.", type);
        send_response(response.dump());
      });

  // ---------------------------------------------------------------------------
  //
  // Static files
  //

  CROW_ROUTE(app, "/")
  ([&gui_dir](const crow::request& /*request*/, crow::response& response) {
    const auto index_html = gui_dir / "index.html";
    response.set_static_file_info(index_html.native());
    response.end();
  });

  CROW_ROUTE(app, "/<path>")
  ([&gui_dir](const crow::request& /*request*/,
              crow::response& response,
              const std::filesystem::path& file_name) {
    auto file_path = gui_dir / file_name;
    if (std::filesystem::is_directory(file_path)) file_path /= "index.html";
    response.set_static_file_info(file_path.native());
    response.end();
  });

  // ---------------------------------------------------------------------------

  /// @todo Pass port as a command line argument.
  app.port(get_env<uint16_t>("TIT_BACKEND_PORT", 18080)).run();
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
