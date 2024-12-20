/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <filesystem>
#include <string>

#include <crow/app.h>
#include <crow/http_request.h>
#include <crow/http_response.h>

#include <Python.h>

#include "cpython/initconfig.h"
#include "tit/core/main_func.hpp"
#include "tit/core/sys/utils.hpp"

namespace tit::back {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto run_backend(CmdArgs /*args*/) -> int {
  const auto exe_dir = exe_path().parent_path();
  const auto root_dir = exe_dir.parent_path();

  PyConfig config;
  config.verbose = 1;
  PyConfig_InitIsolatedConfig(&config);
  PyConfig_SetString(&config, &config.program_name, L"tit_backend");
  const auto python_home = root_dir / "python";
  PyConfig_SetBytesString(&config, &config.home, python_home.c_str());

  auto status = Py_InitializeFromConfig(&config);
  if (PyStatus_IsError(status)) {
    std::cerr << "Failed to initialize Python interpreter! " << status.func
              << " " << status.err_msg << std::endl;
    return 1;
  }

  // Print the path to the `collections` module.
  PyRun_SimpleString("import numpy; print(numpy.__file__)");

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

  Py_Finalize();
  return 0;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit::back

TIT_IMPLEMENT_MAIN(back::run_backend)
