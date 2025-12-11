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
#include <ranges>
#include <span>
#include <string>
#include <thread>
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
#include "tit/data/export-hdf5.hpp"
#include "tit/data/storage.hpp"
#include "tit/data/zip.hpp"

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TIT_IMPLEMENT_MAIN([](int /*argc*/, char** argv) {
  namespace json = nlohmann;

  const auto exe_dir = std::filesystem::path{argv[0]}.parent_path();
  const auto root_dir = exe_dir.parent_path();

  // Load the storage.
  const data::DataStorage storage{"particles.ttdb"};

  crow::SimpleApp app;

  const auto tmp_dir = std::filesystem::temp_directory_path() / "tit-tmp";
  std::filesystem::create_directories(tmp_dir);
  const auto export_dir = std::filesystem::temp_directory_path() / "tit-export";
  std::filesystem::create_directories(export_dir);
  std::jthread export_thread;

  // ---------------------------------------------------------------------------

  CROW_WEBSOCKET_ROUTE(app, "/ws")
      .onmessage([&storage, &tmp_dir, &export_dir, &export_thread](
                     crow::websocket::connection& connection,
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
          // -------------------------------------------------------------------
          response["result"] = storage.last_series().num_frames();
        } else if (type == "frame") {
          // -------------------------------------------------------------------
          const auto frame_index = message["index"].get<std::size_t>();
          const auto frame =
              (storage.last_series().frames() | std::ranges::to<std::vector>())
                  .at(frame_index);

          for (const auto& array : frame.arrays()) {
            std::vector<std::byte> bytes(array.size() * array.type().width());
            array.read(std::span{bytes});

            json::json field_result;
            field_result["kind"] = array.type().kind().name();
            field_result["data"] = crow::utility::base64encode(
                safe_bit_ptr_cast<const uint8_t*>(bytes.data()),
                bytes.size());

            response["result"][array.name()] = field_result;
          }
        } else if (type == "export") {
          // -------------------------------------------------------------------
          TIT_ENSURE(!export_thread.joinable(), "Export already running!");
          export_thread = std::jthread(
              [&storage, &tmp_dir, &export_dir, &connection, response] mutable {
                const auto out_dir = tmp_dir / "particles";
                std::filesystem::create_directories(out_dir);
                data::export_hdf5(out_dir, storage.last_series());
                static constexpr std::string_view zip_name = "particles.zip";
                data::zip_directory(out_dir, export_dir / zip_name);
                response["result"] = zip_name;
                connection.send_text(response.dump());
              });
          return; // No response needed.
        } else {
          // -------------------------------------------------------------------
          response["status"] = "error";
          response["error"] = "Unknown message type: " + type;
        }

        connection.send_text(response.dump());
      });

  // ---------------------------------------------------------------------------

  CROW_ROUTE(app, "/export/<path>")
  ([&export_dir](const crow::request& /*request*/,
                 crow::response& response,
                 const std::filesystem::path& file_name) {
    TIT_ENSURE(file_name == file_name.filename(), "Invalid file name!");
    const auto file_path = export_dir / file_name;
    response.set_static_file_info_unsafe(file_path.native());
    response.end();
  });

  // ---------------------------------------------------------------------------

  CROW_ROUTE(app, "/manual/")
  ([&root_dir](const crow::request& /*request*/, crow::response& response) {
    const auto index_html = root_dir / "manual" / "index.html";
    response.set_static_file_info(index_html.native());
    response.end();
  });
  CROW_ROUTE(app, "/manual/<path>")
  ([&root_dir](const crow::request& /*request*/,
               crow::response& response,
               const std::filesystem::path& file_name) {
    auto file_path = root_dir / "manual" / file_name;
    if (std::filesystem::is_directory(file_path)) file_path /= "index.html";
    response.set_static_file_info(file_path.native());
    response.end();
  });

  // ---------------------------------------------------------------------------

  CROW_ROUTE(app, "/")
  ([&root_dir](const crow::request& /*request*/, crow::response& response) {
    const auto index_html = root_dir / "lib" / "gui" / "index.html";
    response.set_static_file_info(index_html.native());
    response.end();
  });
  CROW_ROUTE(app, "/<path>")
  ([&root_dir](const crow::request& /*request*/,
               crow::response& response,
               const std::filesystem::path& file_name) {
    auto file_path = root_dir / "lib" / "gui" / file_name;
    if (std::filesystem::is_directory(file_path)) file_path /= "index.html";
    response.set_static_file_info(file_path.native());
    response.end();
  });

  // ---------------------------------------------------------------------------

  /// @todo Pass port as a command line argument.
  app.port(get_env<uint16_t>("TIT_BACKEND_PORT", 18080)).run();
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
