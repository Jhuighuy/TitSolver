/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License
 * See /LICENSE.md for license information.
 * SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtCore/QProcess>
#include <QtWidgets/QMainWindow>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
  Q_OBJECT

private:

  Ui::MainWindow* ui;
  QProcess process;

public:

  explicit MainWindow(QWidget* parent = 0);
  ~MainWindow();

private slots:
  void on_actionExit_triggered();
  void on_action_run_triggered();
  void on_action_stop_triggered();
};

#endif // MAINWINDOW_H
