# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
# Commercial use, including SaaS, requires a separate license, see /LICENSE.md
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

import io
import json
import sys
import traceback
from typing import Any, final
from PySide6.QtCore import QObject
from PySide6.QtNetwork import QHostAddress
from PySide6.QtWebSockets import QWebSocketServer, QWebSocket
import titsdk
from titsdk import open_storage, Storage

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

@final
class Server(QObject):

  __client: QWebSocket | None = None
  __server: QWebSocketServer

  __storage: Storage
  __globals: dict[str, Any]

  # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  def __init__(self, parent: QObject | None = None) -> None:
    super().__init__(parent)
    self.__client = None
    self.__server = QWebSocketServer(
        "titapp",
        QWebSocketServer.SslMode.NonSecureMode,
    )

    self.__storage = open_storage("particles.ttdb")
    self.__globals = {"titsdk": titsdk, "storage": self.__storage}

  # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  def start(self, address: QHostAddress, port: int) -> None:
    if not self.__server.listen(address, port):
      raise RuntimeError(
          f"Failed to start server at '{address}:{port}', error: {self.__server.errorString()}"
      )

    self.__server.newConnection.connect(self.__on_new_connection)

  # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  def __on_new_connection(self) -> None:
    # Allow only one client connection.
    client = self.__server.nextPendingConnection()
    if self.__client is not None:
      client.close()
      return

    self.__client = client
    self.__client.disconnected.connect(self.__on_client_disconnected)
    self.__client.textMessageReceived.connect(self.__on_client_message)

  # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  def __on_client_disconnected(self) -> None:
    assert self.__client is not None
    self.__client = None

  # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  def __on_client_message(self, message: str) -> None:
    assert self.__client is not None
    self.__client.sendTextMessage(self.__process_message(message))

  # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  # TODO: This is a temporary solution.
  def __process_message(self, message: str) -> str:
    request = json.loads(message)
    response: Any = {"requestID": request.get("requestID")}
    message = request.get("message")
    if message and message.startswith("#"):
      iterator = self.__storage.last_series.time_steps()
      if (index := int(message[1:])) > 0:
        while index > 0:
          next(iterator)
          index -= 1
      varyings = next(iterator).varyings
      result = {}
      for array in varyings.arrays():
        data = array.data
        result[array.name] = {
            "min": float(data.min()),
            "max": float(data.max()),
            "data": data.ravel().tolist()
        }
      response["status"] = "success"
      response["result"] = result
    elif not message:
      response["status"] = "success"
      response["result"] = self.__storage.last_series.num_time_steps
    else:
      stdout_capture = io.StringIO()
      stderr_capture = io.StringIO()

      old_stdout, old_stderr = sys.stdout, sys.stderr
      sys.stdout, sys.stderr = stdout_capture, stderr_capture

      ok = True
      try:
        exec(message, self.__globals, self.__globals)  # pylint: disable=exec-used
      except Exception:  # pylint: disable=broad-except
        ok = False
        traceback.print_exc(file=sys.stderr)
      finally:
        sys.stdout, sys.stderr = old_stdout, old_stderr

      response["status"] = "success" if ok else "error"
      response["result"] = '\n'.join((
          stdout_capture.getvalue(),
          stderr_capture.getvalue(),
      ))

    return json.dumps(response)

  # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
