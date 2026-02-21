/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <algorithm>
#include <filesystem>
#include <string_view>

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QPointer>
#include <QScopedPointer>
#include <QUrl>
#include <QtEnvironmentVariables>

#include "tit/core/build_info.hpp"
#include "tit/core/exception.hpp"
#include "tit/core/main.hpp"
#include "tit/data/storage.hpp"
#include "titapp/server.hpp"
#include "titapp/window.hpp"

using namespace tit;
using namespace tit::app;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TIT_IMPLEMENT_MAIN([](int argc, char** argv) {
  // Set up paths.
  const auto exe_path = std::filesystem::absolute(argv[0]);
  const auto exe_dir = exe_path.parent_path();
  const auto root_dir = exe_dir.parent_path();

  // Detect headless mode. In headless mode, we don't want to create a
  // `QApplication` instance, and we cannot upgrade the `QCoreApplication`
  // instance to a `QApplication` instance.
  const bool headless =
      std::ranges::contains(argv, argv + argc, std::string_view{"--headless"});

  // Create the application as `QCoreApplication` or `QApplication`.
  QScopedPointer<QCoreApplication> app;
  if (headless) {
    app.reset(new QCoreApplication(argc, argv));
  } else {
    qputenv("QT_MAC_WANTS_LAYER", "1");
    app.reset(new QApplication(argc, argv));
  }
  QCoreApplication::setApplicationName("BlueTit Solver");
  QCoreApplication::setApplicationVersion(build_info::version());

  // Parse command line arguments.
  const QCommandLineOption headless_option{
      "headless",
      "Run in headless mode.",
  };
  const QCommandLineOption port_option{
      {"p", "port"},
      "Port to listen on.",
      "PORT",
      "18080",
  };
  QCommandLineParser parser;
  parser.addHelpOption();
  parser.addVersionOption();
  parser.addOption(headless_option);
  parser.addOption(port_option);
  parser.process(*app);

  // Run the server.
  const auto port = parser.value(port_option).toUShort();
  const Server server(port,
                      exe_dir,
                      data::Storage{root_dir / ".." / ".." / "particles.ttdb"});

  // In GUI mode, we want to show a window with the web view.
  QScopedPointer<Window> window;
  if (!headless) {
    // Set up the environment.
    Window::setup_web_view_environment();

    // Locate the HTML file.
    const auto index_html = root_dir / "lib" / "gui" / "index.html";
    TIT_ENSURE(std::filesystem::exists(index_html),
               "Index HTML file not found '{}'",
               index_html.string());

    // Create and show the main window.
    const auto url = QUrl::fromLocalFile(QString::fromStdString(index_html));
    window.reset(new Window{url});
    window->showMaximized();
  }

  // Run the event loop.
  app->exec();
})

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
