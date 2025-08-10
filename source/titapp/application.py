# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
# Commercial use, including SaaS, requires a separate license, see /LICENSE.md
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

import argparse
import sys
from typing import final
from PySide6.QtWidgets import QApplication
from .server import Server
from .window import Window

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

@final
class Application(QApplication):

  __server: Server
  __window: Window | None

  def __init__(self) -> None:
    super().__init__(sys.argv)

    args = self.__parse_args()

    self.__server = Server()
    self.__server.start_threaded(args.address, args.port)

    if args.headless:
      print(
          "Running in headless mode, "
          f"server is available at http://{args.address}:{args.port}.",
          flush=True,
      )
      self.__window = None
    else:
      self.__window = Window(f"http://{args.address}:{args.port}")
      self.__window.resize(1600, 900)
      self.__window.show()

  def __parse_args(self) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        prog="titapp",
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

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
