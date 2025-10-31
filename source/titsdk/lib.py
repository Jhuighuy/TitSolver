# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

import os
from ctypes import CDLL, c_char_p
from typing import Any, Callable, Final, final

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

@final
class Error(RuntimeError):
  """BlueTit SDK error."""

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

@final
class __Library:
  __dll: CDLL
  __last_error: Callable[[], bytes | None]

  def __init__(self) -> None:
    if isinstance(CDLL, type):  # pyright: ignore[reportUnnecessaryIsInstance]
      dirname = os.path.dirname(__file__)
      for filename in ("libtitsdk.so", "libtitsdk.dylib", "titsdk.dll"):
        if os.path.exists(path := os.path.join(dirname, filename)):
          break
      else:
        raise Error("Unable to find the titsdk library.")
    else:
      path = "mock-titsdk"

    self.__dll = CDLL(path)
    self.__last_error = self.__dll.titsdk__last_error
    self.__last_error.restype = c_char_p

  def func(
      self,
      name: str,
      arg_types: tuple[type, ...],
      ret_type: type | None,
  ) -> Any:
    func = getattr(self.__dll, name)
    func.argtypes = arg_types
    func.restype = ret_type

    def checked_func(*args: Any) -> Any:
      result = func(*args)
      if error := self.__last_error():
        raise Error(error.decode("utf-8"))
      return result

    return checked_func

lib: Final = __Library()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
