/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <QMainWindow>
#include <QUrl>
#include <QWidget>
#include <qtmetamacros.h>

class QWebEngineView;

namespace tit::app {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Application window.
class Window : public QMainWindow {
  Q_OBJECT
public:

  /// Setup the environment for the web view.
  static void setup_web_view_environment();

  /// Constructs a new window.
  explicit Window(const QUrl& url, QWidget* parent = nullptr);

private:

  QWebEngineView* web_view_ = nullptr;

}; // class Window

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::app
