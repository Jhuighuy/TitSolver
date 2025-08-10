# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
# Commercial use, including SaaS, requires a separate license, see /LICENSE.md
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

from abc import ABCMeta, abstractmethod
import asyncio
import json
import http
import mimetypes
from pathlib import Path
import threading
from typing import Any, final, override
import websockets as ws
import websockets.exceptions as wse
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

  def __init__(self) -> None:
    super().__init__()
    self.__storage = open_storage("particles.ttdb")

  @override
  async def on_message(self, message: str, /) -> str:
    request = json.loads(message)
    response: Any = {
        "status": "success",
        "requestID": request.get("requestID"),
        "result": {},
    }
    varyings = self.__storage.last_series.last_time_step.varyings
    for array in varyings.arrays():
      assert array is not None
      response["result"][array.name] = array.data.ravel().tolist()
    return json.dumps(response)

  @override
  async def on_static_file_request(self, route: str, /) -> Path | None:
    path = Path(__file__).parent / ".." / "frontend" / route.lstrip("/")
    if path.is_dir():
      path /= "index.html"
    return path

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
