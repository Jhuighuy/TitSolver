/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License
 * See /LICENSE.md for license information.
 * SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "main_window.hpp"
#include "tit/app/wrap_main.hpp"
#include <QApplication>

int main(int argc, char** argv) {
  return tit::app::wrap_main(argc, argv, [](int the_argc, char** the_argv) {
    QApplication a(the_argc, the_argv);
    MainWindow w;
    w.show();
    return a.exec();
  });
}
