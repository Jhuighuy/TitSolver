/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// On macOS with GCC, compiler is wrongly identified as Clang, and
// `QT_IGNORE_DEPRECATIONS` is defined in a non-GCC compatible way.
#if defined(__APPLE__) && defined(__GNUC__)
#define QT_IGNORE_DEPRECATIONS(...) __VA_ARGS__
#endif

#include <QMainWindow>
#include <QUrl>
#include <QWidget>
#include <QtEnvironmentVariables>
#include <QtWebEngineWidgets/QtWebEngineWidgets> // IWYU pragma: keep

#include "titapp/window.hpp"

namespace tit::app {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Window::Window(const QUrl& url, QWidget* parent) : QMainWindow{parent} {
  qputenv("QTWEBENGINE_CHROMIUM_FLAGS",
          "--disable-logging "
          "--log-level=3 "
          "--enable-gpu "
          "--enable-gpu-rasterization "
          "--enable-zero-copy "
          "--enable-features=CanvasOopRasterization,UseSkiaRenderer "
          "--disable-software-rasterizer "
          "--ignore-gpu-blocklist "
          "--disable-gpu-driver-bug-workarounds "
          "--enable-native-gpu-memory-buffers");

  // NOLINTNEXTLINE(*-prefer-member-initializer,*-include-cleaner)
  web_view_ = new QWebEngineView{this};
  web_view_->setUrl(url);

  connect(web_view_,
          &QWebEngineView::titleChanged,
          this,
          &Window::setWindowTitle);

  connect(web_view_,
          &QWebEngineView::iconChanged,
          this,
          &Window::setWindowIcon);

  setCentralWidget(web_view_);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::app
