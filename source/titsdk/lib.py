# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
# Commercial use, including SaaS, requires a separate license, see /LICENSE.md
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

import os
from ctypes import CDLL, c_char_p
from typing import Any, Callable, Final, final

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

@final
class _Library:  # pylint: disable=too-few-public-methods
  _dll: CDLL
  _last_error: Callable[[], bytes | None]

  def __init__(self) -> None:
    dirname = os.path.dirname(__file__)
    for filename in ("libtitsdk.so", "libtitsdk.dylib", "titsdk.dll"):
      if os.path.exists(path := os.path.join(dirname, filename)):
        break
    else:
      raise Error("Unable to find the titsdk library.")

    self._dll = CDLL(path)
    self._last_error = self._dll.titsdk__last_error
    self._last_error.restype = c_char_p

  def func(
      self,
      name: str,
      arg_types: tuple[type, ...],
      ret_type: type | None,
  ) -> Any:
    func = getattr(self._dll, name)
    func.argtypes = arg_types
    func.restype = ret_type

    def checked_func(*args: Any) -> Any:
      result = func(*args)
      if error := self._last_error():
        raise Error(error.decode("utf-8"))
      return result

    return checked_func

lib: Final = _Library()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

@final
class Error(RuntimeError):
  """BlueTit SDK error."""

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
