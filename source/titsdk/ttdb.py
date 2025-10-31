# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

from __future__ import annotations
from collections.abc import Iterator
from ctypes import c_char_p, c_double, c_uint32, c_uint64, c_void_p
from enum import Enum
from typing import Final, Callable, NewType, cast, final
import numpy as np
from numpy.typing import NDArray
from .lib import lib

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

ttdb_type_t = NewType("ttdb_type_t", c_uint32)
ttdb_type__kind: Final = cast(
    Callable[[ttdb_type_t], bytes],
    lib.func("ttdb_type__kind", (c_uint32,), c_char_p),
)
ttdb_type__rank: Final = cast(
    Callable[[ttdb_type_t], int],
    lib.func("ttdb_type__rank", (c_uint32,), c_uint32),
)
ttdb_type__dim: Final = cast(
    Callable[[ttdb_type_t], int],
    lib.func("ttdb_type__dim", (c_uint32,), c_uint32),
)

@final
class Kind(Enum):
  """Kind of data in an array."""
  unknown = "unknown"
  int8 = "int8_t"
  uint8 = "uint8_t"
  int16 = "int16_t"
  uint16 = "uint16_t"
  int32 = "int32_t"
  uint32 = "uint32_t"
  int64 = "int64_t"
  uint64 = "uint64_t"
  float32 = "float32_t"
  float64 = "float64_t"

  def to_numpy(self) -> np.dtype[np.generic]:
    """Convert the kind to a NumPy data type."""
    kind_to_type: dict[Kind, type] = {
        Kind.int8: np.int8,
        Kind.uint8: np.uint8,
        Kind.int16: np.int16,
        Kind.uint16: np.uint16,
        Kind.int32: np.int32,
        Kind.uint32: np.uint32,
        Kind.int64: np.int64,
        Kind.uint64: np.uint64,
        Kind.float32: np.float32,
        Kind.float64: np.float64,
    }
    return cast(np.dtype[np.generic], kind_to_type[self])

@final
class Rank(Enum):
  """Rank of an array."""
  scalar = 0
  vector = 1
  matrix = 2

@final
class Type:
  """Type of data in an array."""

  __type: ttdb_type_t

  def __init__(self, type_: ttdb_type_t) -> None:
    """Initialize the type with a C type value."""
    self.__type = type_

  def __repr__(self) -> str:
    """Get the string representation of the type."""
    match self.rank:
      case Rank.scalar:
        return self.kind.value
      case Rank.vector:
        return f"Vec<{self.kind.value}, {self.dim}>"
      case Rank.matrix:
        return f"Mat<{self.kind.value}, {self.dim}>"

  @property
  def kind(self) -> Kind:
    """Get the kind of the type."""
    return Kind(ttdb_type__kind(self.__type).decode("utf-8"))

  @property
  def rank(self) -> Rank:
    """Get the rank of the type."""
    return Rank(ttdb_type__rank(self.__type))

  @property
  def dim(self) -> int:
    """Get the dimension of the type."""
    return ttdb_type__dim(self.__type)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

ttdb_array_t = NewType("ttdb_array_t", c_void_p)
ttdb_array__close: Final = cast(
    Callable[[ttdb_array_t], None],
    lib.func("ttdb_array__close", (c_void_p,), None),
)
ttdb_array__name: Final = cast(
    Callable[[ttdb_array_t], bytes],
    lib.func("ttdb_array__name", (c_void_p,), c_char_p),
)
ttdb_array__size: Final = cast(
    Callable[[ttdb_array_t], int],
    lib.func("ttdb_array__size", (c_void_p,), c_uint64),
)
ttdb_array__type: Final = cast(
    Callable[[ttdb_array_t], ttdb_type_t],
    lib.func("ttdb_array__type", (c_void_p,), c_void_p),
)
ttdb_array__read: Final = cast(
    Callable[[ttdb_array_t, c_void_p], None],
    lib.func("ttdb_array__read", (c_void_p, c_void_p), None),
)

@final
class Array:
  """Array of data."""

  __array: ttdb_array_t

  def __init__(self, array: ttdb_array_t) -> None:
    """Initialize the array with a C pointer."""
    self.__array = array

  def __del__(self) -> None:
    """Close the array."""
    ttdb_array__close(self.__array)

  @property
  def name(self) -> str:
    """Get the name of the array."""
    return ttdb_array__name(self.__array).decode("utf-8")

  @property
  def size(self) -> int:
    """Get the size of the array."""
    return ttdb_array__size(self.__array)

  @property
  def type(self) -> Type:
    """Get the type of the array."""
    return Type(ttdb_array__type(self.__array))

  def read(self, data: c_void_p) -> None:
    """Read the array data into the provided buffer."""
    ttdb_array__read(self.__array, data)

  @property
  def data(self) -> NDArray[np.generic]:
    """Get the array data as a NumPy array."""
    shape = (self.size,) + (self.type.dim,) * self.type.rank.value
    array = np.empty(shape, dtype=self.type.kind.to_numpy())
    self.read(c_void_p(array.ctypes.data))
    return array

ttdb_array_iter_t = NewType("ttdb_array_iter_t", c_void_p)
ttdb_array_iter__close: Final = cast(
    Callable[[ttdb_array_iter_t], None],
    lib.func("ttdb_array_iter__close", (c_void_p,), None),
)
ttdb_array_iter__next: Final = cast(
    Callable[[ttdb_array_iter_t], ttdb_array_t | None],
    lib.func("ttdb_array_iter__next", (c_void_p,), c_void_p),
)

@final
class ArrayIter(Iterator[Array]):
  """Iterator over arrays in a dataset."""

  __iter: ttdb_array_iter_t

  def __init__(self, iterator: ttdb_array_iter_t) -> None:
    """Initialize the iterator with a C pointer."""
    self.__iter = iterator

  def __del__(self) -> None:
    """Close the iterator."""
    ttdb_array_iter__close(self.__iter)

  def __iter__(self) -> ArrayIter:
    """Return the iterator itself."""
    return self

  def __next__(self) -> Array:
    """Get the next array from the iterator."""
    if (array := ttdb_array_iter__next(self.__iter)) is None:
      raise StopIteration
    return Array(array)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

ttdb_frame_t = NewType("ttdb_frame_t", c_void_p)
ttdb_frame__close: Final = cast(
    Callable[[ttdb_frame_t], None],
    lib.func("ttdb_frame__close", (c_void_p,), None),
)
ttdb_frame__time: Final = cast(
    Callable[[ttdb_frame_t], float],
    lib.func("ttdb_frame__time", (c_void_p,), c_double),
)
ttdb_frame__num_arrays: Final = cast(
    Callable[[ttdb_frame_t], int],
    lib.func("ttdb_frame__num_arrays", (c_void_p,), c_uint64),
)
ttdb_frame__find_array: Final = cast(
    Callable[[ttdb_frame_t, bytes], ttdb_array_t | None],
    lib.func("ttdb_frame__find_array", (c_void_p, c_char_p), c_void_p),
)
ttdb_frame__arrays: Final = cast(
    Callable[[ttdb_frame_t], ttdb_array_iter_t],
    lib.func("ttdb_frame__arrays", (c_void_p,), c_void_p),
)

@final
class Frame:
  """Frame containing arrays of data."""

  __frame: ttdb_frame_t

  def __init__(self, frame: ttdb_frame_t) -> None:
    """Initialize the frame with a C pointer."""
    self.__frame = frame

  def __del__(self) -> None:
    """Close the frame."""
    ttdb_frame__close(self.__frame)

  @property
  def time(self) -> float:
    """Get the time of the time step."""
    return ttdb_frame__time(self.__frame)

  @property
  def num_arrays(self) -> int:
    """Number of arrays in the frame."""
    return ttdb_frame__num_arrays(self.__frame)

  def find_array(self, name: str) -> Array | None:
    """Find an array by name in the frame."""
    array = ttdb_frame__find_array(self.__frame, name.encode("utf-8"))
    return Array(array) if array is not None else None

  def arrays(self) -> ArrayIter:
    """Iterate over all arrays in the frame."""
    return ArrayIter(ttdb_frame__arrays(self.__frame))

ttdb_frame_iter_t = NewType("ttdb_frame_iter_t", c_void_p)
ttdb_frame_iter__next: Final = cast(
    Callable[[ttdb_frame_iter_t], ttdb_frame_t | None],
    lib.func("ttdb_frame_iter__next", (c_void_p,), c_void_p),
)
ttdb_frame_iter__close: Final = cast(
    Callable[[ttdb_frame_iter_t], None],
    lib.func("ttdb_frame_iter__close", (c_void_p,), None),
)

@final
class FrameIter(Iterator[Frame]):
  """Iterator over frames in a series."""

  __iter: ttdb_frame_iter_t

  def __init__(self, iterator: ttdb_frame_iter_t) -> None:
    """Initialize the iterator with a C pointer."""
    self.__iter = iterator

  def __del__(self) -> None:
    """Close the iterator."""
    ttdb_frame_iter__close(self.__iter)

  def __iter__(self) -> FrameIter:
    """Return the iterator itself."""
    return self

  def __next__(self) -> Frame:
    """Get the next frame from the iterator."""
    if (frame := ttdb_frame_iter__next(self.__iter)) is None:
      raise StopIteration
    return Frame(frame)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

ttdb_series_t = NewType("ttdb_series_t", c_void_p)
ttdb_series__close: Final = cast(
    Callable[[ttdb_series_t], None],
    lib.func("ttdb_series__close", (c_void_p,), None),
)
ttdb_series__num_frames: Final = cast(
    Callable[[ttdb_series_t], int],
    lib.func("ttdb_series__num_frames", (c_void_p,), c_uint64),
)
ttdb_series__last_frame: Final = cast(
    Callable[[ttdb_series_t], ttdb_frame_t],
    lib.func("ttdb_series__last_frame", (c_void_p,), c_void_p),
)
ttdb_series__frames: Final = cast(
    Callable[[ttdb_series_t], ttdb_frame_iter_t],
    lib.func("ttdb_series__frames", (c_void_p,), c_void_p),
)

@final
class Series:
  """Time series in a storage."""

  __series: ttdb_series_t

  def __init__(self, series: ttdb_series_t) -> None:
    """Initialize the series with a C pointer."""
    self.__series = series

  def __del__(self) -> None:
    """Close the series."""
    ttdb_series__close(self.__series)

  @property
  def num_frames(self) -> int:
    """Number of frames in the series."""
    return ttdb_series__num_frames(self.__series)

  @property
  def last_frame(self) -> Frame:
    """Last frame in the series."""
    return Frame(ttdb_series__last_frame(self.__series))

  def frames(self) -> FrameIter:
    """Iterate over all frames in the series."""
    return FrameIter(ttdb_series__frames(self.__series))

ttdb_series_iter_t = NewType("ttdb_series_iter_t", c_void_p)
ttdb_series_iter__next: Final = cast(
    Callable[[ttdb_series_iter_t], ttdb_series_t | None],
    lib.func("ttdb_series_iter__next", (c_void_p,), c_void_p),
)
ttdb_series_iter__close: Final = cast(
    Callable[[ttdb_series_iter_t], None],
    lib.func("ttdb_series_iter__close", (c_void_p,), None),
)

@final
class SeriesIter(Iterator[Series]):
  """Iterator over series in a storage."""

  __iter: ttdb_series_iter_t

  def __init__(self, iterator: ttdb_series_iter_t) -> None:
    """Initialize the iterator with a C pointer."""
    self.__iter = iterator

  def __del__(self) -> None:
    """Close the iterator."""
    ttdb_series_iter__close(self.__iter)

  def __iter__(self) -> SeriesIter:
    """Return the iterator itself."""
    return self

  def __next__(self) -> Series:
    """Get the next series from the iterator."""
    if (series := ttdb_series_iter__next(self.__iter)) is None:
      raise StopIteration
    return Series(series)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

ttdb_t = NewType("ttdb_t", c_void_p)
ttdb__close: Final = cast(
    Callable[[ttdb_t], None],
    lib.func("ttdb__close", (c_void_p,), None),
)
ttdb__open: Final = cast(
    Callable[[bytes], ttdb_t],
    lib.func("ttdb__open", (c_char_p,), c_void_p),
)
ttdb__num_series: Final = cast(
    Callable[[ttdb_t], int],
    lib.func("ttdb__num_series", (c_void_p,), c_uint64),
)
ttdb__last_series: Final = cast(
    Callable[[ttdb_t], ttdb_series_t],
    lib.func("ttdb__last_series", (c_void_p,), c_void_p),
)
ttdb__series: Final = cast(
    Callable[[ttdb_t], ttdb_series_iter_t],
    lib.func("ttdb__series", (c_void_p,), c_void_p),
)

@final
class Storage:
  """BlueTit particle storage."""

  __ttdb: ttdb_t

  def __init__(self, ttdb: ttdb_t) -> None:
    """Initialize the storage with a C pointer."""
    self.__ttdb = ttdb

  def __del__(self) -> None:
    """Close the storage."""
    ttdb__close(self.__ttdb)

  @property
  def num_series(self) -> int:
    """Number of series in the storage."""
    return ttdb__num_series(self.__ttdb)

  @property
  def last_series(self) -> Series:
    """Last series in the storage."""
    return Series(ttdb__last_series(self.__ttdb))

  def series(self) -> SeriesIter:
    """Iterate over all series in the storage."""
    return SeriesIter(ttdb__series(self.__ttdb))

def open_storage(path: str) -> Storage:
  """Open the storage at the given path."""
  return Storage(ttdb__open(path.encode("utf-8")))

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
