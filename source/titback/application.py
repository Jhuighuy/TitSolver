# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
# Commercial use, including SaaS, requires a separate license, see /LICENSE.md
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

import argparse
import os
import sys
from typing import final
from PySide6.QtCore import QUrl
from PySide6.QtNetwork import QHostAddress
from PySide6.QtWidgets import QApplication
from .server import Server
from .window import Window

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

@final
class Application(QApplication):

  __server: Server
  __window: Window | None

  # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  def __init__(self) -> None:
    super().__init__(sys.argv)

    args = self.__parse_args()

    self.__server = Server()

    if args.headless:
      print(
          "Running in headless mode, "
          f"server is available at http://{args.address}:{args.port}.",
          flush=True,
      )
      if args.address in ("localhost", "127.0.0.1", "::1"):
        host = QHostAddress(QHostAddress.SpecialAddress.LocalHost)
      elif args.address == "0.0.0.0":
        host = QHostAddress(QHostAddress.SpecialAddress.Any)
      else:
        host = QHostAddress(args.address)
      self.__server.start(host, args.port)
      self.__window = None
    else:
      host = QHostAddress(QHostAddress.SpecialAddress.LocalHost)
      path = os.path.abspath(
          os.path.join(
              os.path.dirname(__file__),
              "..",
              "frontend",
              "index.html",
          ))
      if not os.path.isfile(path):
        raise FileNotFoundError(
            f"Frontend not found at {path}. "
            "Ensure the application is installed correctly.")
      self.__server.start(host, args.port)
      self.__window = Window(QUrl.fromLocalFile(path))
      self.__window.showMaximized()

  # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  def __parse_args(self) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        prog="titback",
        description="BlueTit CAE.",
        add_help=False,
    )

    # GUI configuration.
    parser.add_argument(
        "--headless",
        action="store_true",
        help="Run the application in headless mode.",
    )

    # Server configuration.
    parser.add_argument(
        "-A",
        "--address",
        metavar="ADDRESS",
        type=str,
        default="localhost",
        help="Address to bind to.",
    )
    parser.add_argument(
        "-P",
        "--port",
        metavar="PORT",
        type=int,
        default=8080,
        help="Port.",
    )

    # Keep it last.
    parser.add_argument(
        "-h",
        "--help",
        action="help",
        help="Show this help message and exit.",
    )

    return parser.parse_args()

  # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
