# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
# Commercial use, including SaaS, requires a separate license, see /LICENSE.md
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

import io
import json
import sys
import traceback
from typing import Any, final, Literal, TypedDict
from PySide6.QtCore import QObject
from PySide6.QtNetwork import QHostAddress
from PySide6.QtWebSockets import QWebSocketServer, QWebSocket
import titsdk
from titsdk import open_storage, Storage

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class Response(TypedDict):
  status: Literal["success", "error"]
  result: Any

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

@final
class Server(QObject):

  __client: QWebSocket | None = None
  __server: QWebSocketServer

  __storage: Storage | None
  __globals: dict[str, Any]

  # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  def __init__(self, parent: QObject | None = None) -> None:
    super().__init__(parent)
    self.__client = None
    self.__server = QWebSocketServer(
        "titback",
        QWebSocketServer.SslMode.NonSecureMode,
    )

    try:
      self.__storage = open_storage("particles.ttdb")
    except titsdk.Error as e:
      print(f"Warning: Failed to open storage: {e}")
      self.__storage = None

    self.__globals = {"titsdk": titsdk, "storage": self.__storage}

  # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  def start(self, address: QHostAddress, port: int) -> None:
    if not self.__server.listen(address, port):
      raise RuntimeError(f"Failed to start server at '{address}:{port}', "
                         f"error: {self.__server.errorString()}")

    self.__server.newConnection.connect(self.__on_new_connection)

  # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  def __on_new_connection(self) -> None:
    # Allow only one client connection.
    client = self.__server.nextPendingConnection()
    if self.__client is not None:
      print("Warning: Rejecting new client connection.")
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

  def __process_message(self, message: str) -> str:
    try:
      request = json.loads(message)
    except json.JSONDecodeError as e:
      return json.dumps({"status": "error", "result": f"Invalid JSON: {e}"})

    message = request.get("message")
    response: Any = {"requestID": request.get("requestID")}
    if message and message.startswith("#"):
      response.update(self.__process_get_time_step_message(message))
    elif not message:
      response.update(self.__process_get_num_time_steps_message(message))
    else:
      response.update(self.__process_execute_message(message))

    return json.dumps(response)

  # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  def __process_get_num_time_steps_message(self, message: str) -> Response:
    assert not message

    if self.__storage is None:
      return {"status": "error", "result": "Storage is not available."}

    return {
        "status": "success",
        "result": self.__storage.last_series.num_time_steps,
    }

  # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  def __process_get_time_step_message(self, message: str) -> Response:
    assert message.startswith("#")

    if self.__storage is None:
      return {"status": "error", "result": "Storage is not available."}

    try:
      index = int(message[1:])
    except ValueError:
      return {"status": "error", "result": "Invalid time step index."}

    num_steps = self.__storage.last_series.num_time_steps
    if index < 0 or index >= num_steps:
      return {
          "status": "error",
          "result": f"Index {index} out of range [0, {num_steps})."
      }

    iterator = self.__storage.last_series.time_steps()
    if index > 0:
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

    return {
        "status": "success",
        "result": result,
    }

  # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  def __process_execute_message(self, message: str) -> Response:
    stdout_capture = io.StringIO()
    stderr_capture = io.StringIO()

    old_stdout, old_stderr = sys.stdout, sys.stderr
    sys.stdout, sys.stderr = stdout_capture, stderr_capture

    # pylint: disable=exec-used,broad-except
    try:
      exec(message, self.__globals, self.__globals)
      status = True
    except Exception:
      traceback.print_exc(file=sys.stderr)
      status = False

    sys.stdout, sys.stderr = old_stdout, old_stderr
    output = '\n'.join((
        stdout_capture.getvalue(),
        stderr_capture.getvalue(),
    ))

    return {
        "status": "success" if status else "error",
        "result": output,
    }

  # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
