/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "main_window.hpp"
#include "ui_main_window.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);
  connect(ui->clear_console_button, &QToolButton::pressed, //
          [this] { ui->console_text_edit->clear(); });
  ui->action_stop->setDisabled(true);
}

MainWindow::~MainWindow() {
  delete ui;
}

void MainWindow::on_actionExit_triggered() {
  this->close();
}

void MainWindow::on_action_run_triggered() {
  ui->console_text_edit->clear();
  connect(&process, &QProcess::readyReadStandardOutput, //
          [this] {
            QTextCursor cursor(ui->console_text_edit->textCursor());
            QTextCharFormat format;
            format.setForeground(QBrush(QColor("black")));
            cursor.setCharFormat(format);
            cursor.insertText(process.readAllStandardOutput());
            ui->console_text_edit->ensureCursorVisible();
          });
  connect(&process, &QProcess::readyReadStandardError, //
          [this] {
            QTextCursor cursor(ui->console_text_edit->textCursor());
            QTextCharFormat format;
            format.setForeground(QBrush(QColor("red")));
            format.setFontWeight(QFont::DemiBold);
            cursor.setCharFormat(format);
            cursor.insertText(process.readAllStandardError());
            ui->console_text_edit->ensureCursorVisible();
          });
  process.start("time", {"output/TIT_ROOT/bin/titwcsph"});
  ui->action_run->setDisabled(true);
  ui->action_stop->setDisabled(false);
}

void MainWindow::on_action_stop_triggered() {
  ui->action_run->setDisabled(false);
  ui->action_stop->setDisabled(true);
  process.kill();
}
