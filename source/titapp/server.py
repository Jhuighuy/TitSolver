# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
# Commercial use, including SaaS, requires a separate license, see /LICENSE.md
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

from abc import ABCMeta, abstractmethod
import asyncio
import io
import json
import http
import mimetypes
from pathlib import Path
import sys
import threading
import traceback
from typing import Any, final, override
import websockets as ws
import websockets.exceptions as wse
import titsdk
from titsdk import open_storage

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class BasicServer(metaclass=ABCMeta):

  __conn: ws.ServerConnection | None

  def __init__(self) -> None:
    self.__conn = None

  @final
  def start_sync(self, address: str, port: int) -> None:
    asyncio.run(self.__start(address, port))

  @final
  def start_threaded(self, address: str, port: int) -> None:
    threading.Thread(target=self.start_sync, args=(address, port)).start()

  @abstractmethod
  async def on_message(self, message: str, /) -> str:
    ...

  @abstractmethod
  async def on_static_file_request(self, route: str, /) -> Path | None:
    ...

  async def __start(self, address: str, port: int) -> None:
    async with ws.serve(
        self.__handle_client,
        address,
        port,
        process_request=self.__process_request,
    ) as server:
      await server.serve_forever()

  async def __process_request(
      self,
      conn: ws.ServerConnection,
      request: ws.Request,
  ) -> ws.Response | None:
    # Handle WebSocket upgrade requests.
    if request.path == "/ws":
      return None

    # Otherwise, hijack the request and serve static files.
    path = await self.on_static_file_request(request.path)
    if path is None or not path.exists():
      return conn.respond(404, "Not Found")
    status = http.HTTPStatus(200)
    mime = mimetypes.guess_type(str(path))[0] or "application/octet-stream"
    with open(path, "rb") as f:
      body = f.read()
    headers = ws.Headers([
        ("Connection", "close"),
        ("Content-Length", str(len(body))),
        ("Content-Type", mime),
    ])
    return ws.Response(status.value, status.phrase, headers, body)

  async def __handle_client(self, conn: ws.ServerConnection) -> None:
    # Reject new connections if one is already active.
    if self.__conn is not None:
      await conn.close()
      return

    # Handle the connection.
    try:
      self.__conn = conn
      async for message in conn:
        if isinstance(message, str):
          await conn.send(await self.on_message(message))
        else:
          raise wse.InvalidMessage("Binary messages are not supported.")
    except wse.ConnectionClosed:
      pass
    finally:
      self.__conn = None

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

@final
class Server(BasicServer):

  __globals: dict[str, Any]
  __locals: dict[str, Any]

  def __init__(self) -> None:
    super().__init__()
    self.__storage = open_storage("particles.ttdb")
    self.__globals = {"titsdk": titsdk, "storage": self.__storage}
    self.__locals = self.__globals

  @override
  async def on_message(self, message: str, /) -> str:
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
        result[array.name] = array.data.ravel().tolist()
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
        exec(message, self.__globals, self.__locals)
      except Exception:
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

  @override
  async def on_static_file_request(self, route: str, /) -> Path | None:
    path = Path(__file__).parent / ".." / "frontend" / route.lstrip("/")
    if path.is_dir():
      path /= "index.html"
    return path

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
