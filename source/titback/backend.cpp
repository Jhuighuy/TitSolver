/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// Note: False-positives from Crow internals on Fedora 43.
#if defined(__linux__) && defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic ignored "-Wstringop-overflow"
#endif

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <format>
#include <iterator>
#include <memory>
#include <mutex>
#include <ranges>
#include <stop_token>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-literal-operator"
#endif
#include <crow/app.h>
#include <crow/http_request.h>
#include <crow/http_response.h>
#include <crow/utility.h>
#include <crow/websocket.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

#include "tit/core/checks.hpp"
#include "tit/core/env.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/main.hpp"
#include "tit/core/posix.hpp"
#include "tit/core/proc_info.hpp"
#include "tit/core/type.hpp"
#include "tit/core/zip.hpp"
#include "tit/data/hdf5.hpp"
#include "tit/data/storage.hpp"

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TIT_IMPLEMENT_MAIN([](int /*argc*/, char** argv) {
  namespace json = nlohmann;

  const auto exe_dir = std::filesystem::path{argv[0]}.parent_path();
  const auto root_dir = exe_dir.parent_path();
  const auto gui_dir = root_dir / "lib" / "gui";
  const auto manual_dir = root_dir / "manual";

  const auto tmp_dir = std::filesystem::temp_directory_path() / "tit-tmp";
  const auto export_dir = std::filesystem::temp_directory_path() / "tit-export";
  std::filesystem::create_directories(tmp_dir);
  std::filesystem::create_directories(export_dir);

  // ---------------------------------------------------------------------------
  //
  // Application state
  //

  crow::SimpleApp app;

  const data::Storage storage{"particles.ttdb"};

  const auto solver_path = exe_dir / "titwcsph";
  std::mutex solver_mutex;
  std::jthread solver_thread;
  Process* solver_process = nullptr; // Owned by `solver_thread`.
  std::mutex solver_telemetry_mutex;
  std::jthread solver_telemetry_thread;
  struct SolverTelemetry final {
    uint64_t timestamp = 0;
    float64_t cpu_percent = 0.0;
    uint64_t memory_bytes = 0;
    bool has_value = false;
  } solver_telemetry;

  std::jthread export_thread;

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
                  &storage,
                  &solver_path,
                  &solver_mutex,
                  &solver_telemetry_mutex,
                  &solver_telemetry_thread,
                  &solver_telemetry,
                  &solver_thread,
                  &solver_process,
                  &tmp_dir,
                  &export_dir,
                  &export_thread](crow::websocket::connection& /*connection*/,
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
          const auto frame_index = message["index"].get<size_t>();
          auto frames = storage.last_series().frames();
          auto frame_iter = std::ranges::begin(frames);
          const auto frames_end = std::ranges::end(frames);
          std::ranges::advance(frame_iter, frame_index, frames_end);
          TIT_ENSURE(frame_iter != frames_end, "Frame index out of bounds.");
          const auto& frame = *frame_iter;

          json::json result;
          for (const auto& array : frame.arrays()) {
            const auto bytes = array.read();

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
        if (type == "run") {
          const std::scoped_lock lock{solver_mutex};

          TIT_ALWAYS_ASSERT(solver_process == nullptr,
                            "Solver is already running.");

          auto solver_process_holder = std::make_unique<Process>();
          solver_process = solver_process_holder.get();

          const auto append_telemetry =
              [&solver_telemetry_mutex, &solver_telemetry](json::json& result) {
                const std::scoped_lock telemetry_lock{solver_telemetry_mutex};
                if (!solver_telemetry.has_value) return;

                result["timestamp"] = solver_telemetry.timestamp;
                result["cpuPercent"] = solver_telemetry.cpu_percent;
                result["memoryBytes"] = solver_telemetry.memory_bytes;
              };

          solver_process->on_stdout(
              [response, &send_response, append_telemetry](
                  std::string_view data) mutable {
                response["repeat"] = true;
                auto result = json::json{
                    {"kind", "stdout"},
                    {"data", data},
                };
                append_telemetry(result);
                response["result"] = std::move(result);
                send_response(response.dump());
              });

          solver_process->on_stderr(
              [response, &send_response, append_telemetry](
                  std::string_view data) mutable {
                response["repeat"] = true;
                auto result = json::json{
                    {"kind", "stderr"},
                    {"data", data},
                };
                append_telemetry(result);
                response["result"] = std::move(result);
                send_response(response.dump());
              });

          solver_process->on_exit(
              [response,
               &send_response,
               &solver_telemetry_thread](int code, int signal) mutable {
                solver_telemetry_thread.request_stop();
                response["result"] = json::json{
                    {"kind", "exit"},
                    {"code", code},
                    {"signal", signal},
                };
                send_response(response.dump());
              });

          solver_process->spawn_child(solver_path);

          {
            const std::scoped_lock telemetry_lock{solver_telemetry_mutex};
            solver_telemetry = {};
          }

          const auto solver_pid = solver_process->pid();
          try {
            const auto snapshot = proc_info::query_usage(solver_pid);
            const auto now =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch())
                    .count();
            const std::scoped_lock telemetry_lock{solver_telemetry_mutex};
            solver_telemetry = {
                .timestamp = static_cast<uint64_t>(now),
                .cpu_percent = 0.0,
                .memory_bytes = snapshot.memory_bytes,
                .has_value = true,
            };
          } catch (const std::exception& /*e*/) {
          }

          solver_telemetry_thread = std::jthread{
              [solver_pid, &solver_telemetry_mutex, &solver_telemetry](
                  const std::stop_token& stop_token) {
                try {
                  auto previous_snapshot = proc_info::query_usage(solver_pid);
                  auto previous_time = std::chrono::steady_clock::now();

                  while (!stop_token.stop_requested()) {
                    std::this_thread::sleep_for(std::chrono::seconds{1});
                    if (stop_token.stop_requested()) break;

                    const auto now = std::chrono::steady_clock::now();
                    const auto snapshot = proc_info::query_usage(solver_pid);
                    const auto timestamp =
                        std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::system_clock::now().time_since_epoch())
                            .count();
                    const auto cpu_percent = proc_info::compute_cpu_percent(
                        previous_snapshot,
                        snapshot,
                        std::chrono::duration_cast<std::chrono::nanoseconds>(
                            now - previous_time));

                    {
                      const std::scoped_lock telemetry_lock{
                          solver_telemetry_mutex};
                      solver_telemetry = {
                          .timestamp = static_cast<uint64_t>(timestamp),
                          .cpu_percent = cpu_percent,
                          .memory_bytes = snapshot.memory_bytes,
                          .has_value = true,
                      };
                    }

                    previous_snapshot = snapshot;
                    previous_time = now;
                  }
                } catch (const std::exception& /*e*/) {
                }
              }};

          solver_thread =
              std::jthread{[&solver_mutex,
                            &solver_process,
                            holder = std::move(solver_process_holder)] {
                holder->wait_child();

                const std::scoped_lock lock_{solver_mutex};
                solver_process = nullptr;
              }};

          return;
        }

        // ---------------------------------------------------------------------
        if (type == "stop") {
          const std::scoped_lock lock{solver_mutex};

          TIT_ALWAYS_ASSERT(
              (solver_process != nullptr && solver_process->is_running()),
              "Solver is not running.");

          solver_telemetry_thread.request_stop();
          solver_process->kill_child();

          send_response(response.dump());
          return;
        }

        // ---------------------------------------------------------------------
        if (type == "export") {
          export_thread = std::jthread{[&storage,
                                        &tmp_dir,
                                        &export_dir,
                                        response,
                                        &send_response] mutable {
            const auto out_dir = tmp_dir / "particles";
            std::filesystem::create_directories(out_dir);
            data::export_hdf5(out_dir, storage.last_series());

            static constexpr std::string_view zip_name = "particles.zip";
            ZipWriter zip_writer{export_dir / zip_name};
            zip_writer.add_dir(out_dir);
            zip_writer.close();

            response["result"] = zip_name;
            send_response(response.dump());
          }};

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
  ([&manual_dir](const crow::request& /*request*/, crow::response& response) {
    const auto index_html = manual_dir / "index.html";
    response.set_static_file_info(index_html.native());
    response.end();
  });

  CROW_ROUTE(app, "/manual/<path>")
  ([&manual_dir](const crow::request& /*request*/,
                 crow::response& response,
                 const std::filesystem::path& file_name) {
    auto file_path = manual_dir / file_name;
    if (std::filesystem::is_directory(file_path)) file_path /= "index.html";
    response.set_static_file_info(file_path.native());
    response.end();
  });

  // ---------------------------------------------------------------------------

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
