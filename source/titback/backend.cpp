/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// Note: False-positives from Crow internals on Fedora 43.
#if defined(__linux__) && defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic ignored "-Wstringop-overflow"
#endif

#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <format>
#include <iterator>
#include <memory>
#include <mutex>
#include <ranges>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-literal-operator"
#endif
#include <crow/app.h>
#include <crow/common.h>
#include <crow/http_request.h>
#include <crow/http_response.h>
#include <crow/middlewares/cors.h>
#include <crow/utility.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

#include "tit/core/env.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/main.hpp"
#include "tit/core/posix.hpp"
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

  crow::App<crow::CORSHandler> app;
  app.get_middleware<crow::CORSHandler>()
      .prefix("/api")
      .origin("*")
      .methods(crow::HTTPMethod::Get,
               crow::HTTPMethod::Post,
               crow::HTTPMethod::Options)
      .headers("Content-Type", "Accept")
      .max_age(600);
  app.get_middleware<crow::CORSHandler>()
      .prefix("/export")
      .origin("*")
      .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Options)
      .headers("Content-Type", "Accept")
      .max_age(600);
  const data::Storage storage{"particles.ttdb"};

  const auto solver_path = exe_dir / "titwcsph";
  std::mutex solver_mutex;
  std::jthread solver_thread;
  Process* solver_process = nullptr; // Owned by `solver_thread`.
  std::string solver_output;

  auto send_json =
      [](crow::response& response, const json::json& body, int code = 200) {
        response.code = code;
        response.set_header("Content-Type", "application/json");
        response.write(body.dump());
        response.end();
      };

  auto send_error = [&send_json](crow::response& response,
                                 std::string_view error,
                                 int code = 400) {
    send_json(response,
              json::json{{"status", "error"}, {"error", error}},
              code);
  };

  // ---------------------------------------------------------------------------
  // API: Storage.

  CROW_ROUTE(app, "/api/storage/num-frames")
      .methods(crow::HTTPMethod::Get)([&storage,
                                       &send_json](const crow::request&,
                                                   crow::response& response) {
        send_json(response,
                  json::json{{"status", "success"},
                             {"result", storage.last_series().num_frames()}});
      });

  CROW_ROUTE(app, "/api/storage/frame/<uint>")
      .methods(crow::HTTPMethod::Get)([&storage, &send_json, &send_error](
                                          const crow::request&,
                                          crow::response& response,
                                          size_t frame_index) {
        try {
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

          send_json(
              response,
              json::json{{"status", "success"}, {"result", std::move(result)}});
        } catch (const std::exception& error) {
          send_error(response, error.what());
        }
      });

  // ---------------------------------------------------------------------------
  // API: Solver.

  CROW_ROUTE(app, "/api/solver/run")
      .methods(crow::HTTPMethod::Post)(
          [&solver_mutex,
           &solver_thread,
           &solver_process,
           &solver_output,
           &solver_path,
           &send_json,
           &send_error](const crow::request&, crow::response& response) {
            const std::scoped_lock lock{solver_mutex};
            if (solver_process != nullptr && solver_process->is_running()) {
              send_error(response, "Solver is already running.");
              return;
            }

            solver_output.clear();

            auto solver_process_holder = std::make_unique<Process>();
            solver_process = solver_process_holder.get();

            solver_process->on_stdout(
                [&solver_mutex, &solver_output](std::string_view data) {
                  const std::scoped_lock lock_{solver_mutex};
                  solver_output += data;
                });

            solver_process->on_stderr(
                [&solver_mutex, &solver_output](std::string_view data) {
                  const std::scoped_lock lock_{solver_mutex};
                  solver_output += data;
                });

            solver_process->on_exit([&solver_mutex,
                                     &solver_output](int code, int signal) {
              const std::scoped_lock lock_{solver_mutex};
              solver_output +=
                  std::format("\n[Process exited with code {}, signal {}]\n",
                              code,
                              signal);
            });

            solver_process->spawn_child(solver_path);

            solver_thread =
                std::jthread{[&solver_mutex,
                              &solver_process,
                              holder = std::move(solver_process_holder)] {
                  holder->wait_child();

                  const std::scoped_lock lock_{solver_mutex};
                  solver_process = nullptr;
                }};

            send_json(response, json::json{{"status", "success"}});
          });

  CROW_ROUTE(app, "/api/solver/stop")
      .methods(crow::HTTPMethod::Post)(
          [&solver_mutex, &solver_process, &send_json, &send_error](
              const crow::request&,
              crow::response& response) {
            const std::scoped_lock lock{solver_mutex};

            if (solver_process == nullptr || !solver_process->is_running()) {
              send_error(response, "Solver is not running.");
              return;
            }

            solver_process->kill_child();
            send_json(response, json::json{{"status", "success"}});
          });

  CROW_ROUTE(app, "/api/solver/status")
      .methods(crow::HTTPMethod::Get)(
          [&solver_mutex, &solver_process, &solver_output, &send_json](
              const crow::request&,
              crow::response& response) {
            const std::scoped_lock lock{solver_mutex};

            send_json(response,
                      json::json{
                          {"status", "success"},
                          {"result",
                           json::json{
                               {"isRunning",
                                solver_process != nullptr &&
                                    solver_process->is_running()},
                               {"output", solver_output},
                           }},
                      });
          });

  // ---------------------------------------------------------------------------
  // API: Export.

  CROW_ROUTE(app, "/api/export")
      .methods(crow::HTTPMethod::Post)(
          [&storage, &tmp_dir, &export_dir, &send_json, &send_error](
              const crow::request&,
              crow::response& response) {
            try {
              const auto out_dir = tmp_dir / "particles";
              std::filesystem::create_directories(out_dir);
              data::export_hdf5(out_dir, storage.last_series());

              static constexpr std::string_view zip_name = "particles.zip";
              ZipWriter zip_writer{export_dir / zip_name};
              zip_writer.add_dir(out_dir);
              zip_writer.close();

              send_json(
                  response,
                  json::json{{"status", "success"}, {"result", zip_name}});
            } catch (const std::exception& error) {
              send_error(response, error.what(), 500);
            }
          });

  // ---------------------------------------------------------------------------
  // Static files.

  CROW_ROUTE(app, "/export/<path>")
  ([&export_dir](const crow::request&,
                 crow::response& response,
                 const std::filesystem::path& file_name) {
    TIT_ENSURE(file_name == file_name.filename(), "Invalid file name!");
    const auto file_path = export_dir / file_name;
    response.set_static_file_info_unsafe(file_path.native());
    response.end();
  });

  CROW_ROUTE(app, "/manual/")
  ([&manual_dir](const crow::request&, crow::response& response) {
    const auto index_html = manual_dir / "index.html";
    response.set_static_file_info(index_html.native());
    response.end();
  });

  CROW_ROUTE(app, "/manual/<path>")
  ([&manual_dir](const crow::request&,
                 crow::response& response,
                 const std::filesystem::path& file_name) {
    auto file_path = manual_dir / file_name;
    if (std::filesystem::is_directory(file_path)) file_path /= "index.html";
    response.set_static_file_info(file_path.native());
    response.end();
  });

  CROW_ROUTE(app, "/")
  ([&gui_dir](const crow::request&, crow::response& response) {
    const auto index_html = gui_dir / "index.html";
    response.set_static_file_info(index_html.native());
    response.end();
  });

  CROW_ROUTE(app, "/<path>")
  ([&gui_dir](const crow::request&,
              crow::response& response,
              const std::filesystem::path& file_name) {
    auto file_path = gui_dir / file_name;
    if (std::filesystem::is_directory(file_path)) file_path /= "index.html";
    response.set_static_file_info(file_path.native());
    response.end();
  });

  app.port(get_env<uint16_t>("TIT_BACKEND_PORT", 18080)).run();
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
