# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
# Commercial use, including SaaS, requires a separate license, see /LICENSE.md
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

from __future__ import annotations
import os
from collections.abc import Iterator
from ctypes import CDLL, c_char_p, c_double, c_uint32, c_uint64, c_void_p
from enum import Enum
from typing import Any, Final, Callable, NewType, cast, final
import numpy as np
from numpy.typing import NDArray

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

@final
class _Library:  # pylint: disable=too-few-public-methods
  _dll: CDLL
  _last_error: Callable[[], bytes | None]

  def __init__(self) -> None:
    self._dll = CDLL(os.path.join(os.path.dirname(__file__), "libttdb.so"))
    self._last_error = self._dll.ttdb__last_error
    self._last_error.restype = c_char_p

  def func(self, name: str, arg_types: tuple[type, ...],
           ret_type: type | None) -> Any:
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
  """BlueTit Particle Storage error."""

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

ttdb_type_t = NewType("ttdb_type_t", c_uint32)
ttdb_type__kind: Final = cast(
    Callable[[ttdb_type_t], bytes],
    lib.func("ttdb_type__kind", (c_uint32,), c_char_p))
ttdb_type__rank: Final = cast(
    Callable[[ttdb_type_t], int],
    lib.func("ttdb_type__rank", (c_uint32,), c_uint32))
ttdb_type__dim: Final = cast(  #
    Callable[[ttdb_type_t], int],
    lib.func("ttdb_type__dim", (c_uint32,), c_uint32))

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

  def to_numpy(self) -> np.dtype:
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
    return np.dtype(kind_to_type[self])

@final
class Rank(Enum):
  """Rank of an array."""
  scalar = 0
  vector = 1
  matrix = 2

@final
class Type:
  """Type of data in an array."""

  _type: ttdb_type_t

  def __init__(self, type_: ttdb_type_t) -> None:
    """Initialize the type with a C type value."""
    self._type = type_

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
    return Kind(ttdb_type__kind(self._type).decode("utf-8"))

  @property
  def rank(self) -> Rank:
    """Get the rank of the type."""
    return Rank(ttdb_type__rank(self._type))

  @property
  def dim(self) -> int:
    """Get the dimension of the type."""
    return ttdb_type__dim(self._type)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

ttdb_array_t = NewType("ttdb_array_t", c_void_p)
ttdb_array__close: Final = cast(
    Callable[[ttdb_array_t], None],
    lib.func("ttdb_array__close", (c_void_p,), None))
ttdb_array__name: Final = cast(
    Callable[[ttdb_array_t], bytes],
    lib.func("ttdb_array__name", (c_void_p,), c_char_p))
ttdb_array__size: Final = cast(
    Callable[[ttdb_array_t], int],
    lib.func("ttdb_array__size", (c_void_p,), c_uint64))
ttdb_array__type: Final = cast(
    Callable[[ttdb_array_t], ttdb_type_t],
    lib.func("ttdb_array__type", (c_void_p,), c_void_p))
ttdb_array__read: Final = cast(
    Callable[[ttdb_array_t, c_void_p], None],
    lib.func("ttdb_array__read", (c_void_p, c_void_p), None))

@final
class Array:
  """Array of data."""

  _array: ttdb_array_t

  def __init__(self, array: ttdb_array_t) -> None:
    """Initialize the array with a C pointer."""
    self._array = array

  def __del__(self) -> None:
    """Close the array."""
    ttdb_array__close(self._array)

  @property
  def name(self) -> str:
    """Get the name of the array."""
    return ttdb_array__name(self._array).decode("utf-8")

  @property
  def size(self) -> int:
    """Get the size of the array."""
    return ttdb_array__size(self._array)

  @property
  def type(self) -> Type:
    """Get the type of the array."""
    return Type(ttdb_array__type(self._array))

  def read(self, data: c_void_p) -> None:
    """Read the array data into the provided buffer."""
    ttdb_array__read(self._array, data)

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
    lib.func("ttdb_array_iter__close", (c_void_p,), None))
ttdb_array_iter__next: Final = cast(
    Callable[[ttdb_array_iter_t], ttdb_array_t | None],
    lib.func("ttdb_array_iter__next", (c_void_p,), c_void_p))

@final
class ArrayIter(Iterator[Array]):
  """Iterator over arrays in a dataset."""

  _iter: ttdb_array_iter_t

  def __init__(self, iterator: ttdb_array_iter_t) -> None:
    """Initialize the iterator with a C pointer."""
    self._iter = iterator

  def __del__(self) -> None:
    """Close the iterator."""
    ttdb_array_iter__close(self._iter)

  def __iter__(self) -> ArrayIter:
    """Return the iterator itself."""
    return self

  def __next__(self) -> Array:
    """Get the next array from the iterator."""
    if (array := ttdb_array_iter__next(self._iter)) is None:
      raise StopIteration
    return Array(array)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

ttdb_dataset_t = NewType("ttdb_dataset_t", c_void_p)
ttdb_dataset__close: Final = cast(
    Callable[[ttdb_dataset_t], None],
    lib.func("ttdb_dataset__close", (c_void_p,), None))
ttdb_dataset__num_arrays: Final = cast(
    Callable[[ttdb_dataset_t], int],
    lib.func("ttdb_dataset__num_arrays", (c_void_p,), c_uint64))
ttdb_dataset__find_array: Final = cast(
    Callable[[ttdb_dataset_t, bytes], ttdb_array_t | None],
    lib.func("ttdb_dataset__find_array", (c_void_p, c_char_p), c_void_p))
ttdb_dataset__arrays: Final = cast(
    Callable[[ttdb_dataset_t], ttdb_array_iter_t],
    lib.func("ttdb_dataset__arrays", (c_void_p,), c_void_p))

@final
class Dataset:
  """Dataset containing arrays of data."""

  _dataset: ttdb_dataset_t

  def __init__(self, dataset: ttdb_dataset_t) -> None:
    """Initialize the dataset with a C pointer."""
    self._dataset = dataset

  def __del__(self) -> None:
    """Close the dataset."""
    ttdb_dataset__close(self._dataset)

  @property
  def num_arrays(self) -> int:
    """Number of arrays in the dataset."""
    return ttdb_dataset__num_arrays(self._dataset)

  def find_array(self, name: str) -> Array | None:
    """Find an array by name in the dataset."""
    array = ttdb_dataset__find_array(self._dataset, name.encode("utf-8"))
    return Array(array) if array is not None else None

  def arrays(self) -> ArrayIter:
    """Iterate over all arrays in the dataset."""
    return ArrayIter(ttdb_dataset__arrays(self._dataset))

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

ttdb_time_step_t = NewType("ttdb_time_step_t", c_void_p)
ttdb_time_step__close: Final = cast(
    Callable[[ttdb_time_step_t], None],
    lib.func("ttdb_time_step__close", (c_void_p,), None))
ttdb_time_step__time: Final = cast(
    Callable[[ttdb_time_step_t], float],
    lib.func("ttdb_time_step__time", (c_void_p,), c_double))
ttdb_time_step__uniforms: Final = cast(
    Callable[[ttdb_time_step_t], ttdb_dataset_t],
    lib.func("ttdb_time_step__uniforms", (c_void_p,), c_void_p))
ttdb_time_step__varyings: Final = cast(
    Callable[[ttdb_time_step_t], ttdb_dataset_t],
    lib.func("ttdb_time_step__varyings", (c_void_p,), c_void_p))

@final
class TimeStep:
  """Time step in a series."""

  _time_step: ttdb_time_step_t

  def __init__(self, time_step: ttdb_time_step_t) -> None:
    """Initialize the time step with a C pointer."""
    self._time_step = time_step

  def __del__(self) -> None:
    """Close the time step."""
    ttdb_time_step__close(self._time_step)

  @property
  def time(self) -> float:
    """Get the time of the time step."""
    return ttdb_time_step__time(self._time_step)

  @property
  def uniforms(self) -> Dataset:
    """Get the uniform dataset of the time step."""
    return Dataset(ttdb_time_step__uniforms(self._time_step))

  @property
  def varyings(self) -> Dataset:
    """Get the varying dataset of the time step."""
    return Dataset(ttdb_time_step__varyings(self._time_step))

ttdb_time_step_iter_t = NewType("ttdb_time_step_iter_t", c_void_p)
ttdb_time_step_iter__close = cast(
    Callable[[ttdb_time_step_iter_t], None],
    lib.func("ttdb_time_step_iter__close", (c_void_p,), None))
ttdb_time_step_iter__next = cast(
    Callable[[ttdb_time_step_iter_t], ttdb_time_step_t | None],
    lib.func("ttdb_time_step_iter__next", (c_void_p,), c_void_p))

@final
class TimeStepIter(Iterator[TimeStep]):
  """Iterator over time steps in a series."""

  _iter: ttdb_time_step_iter_t

  def __init__(self, iterator: ttdb_time_step_iter_t) -> None:
    """Initialize the iterator with a C pointer."""
    self._iter = iterator

  def __del__(self) -> None:
    """Close the iterator."""
    ttdb_time_step_iter__close(self._iter)

  def __iter__(self) -> TimeStepIter:
    """Return the iterator itself."""
    return self

  def __next__(self) -> TimeStep:
    """Get the next time step from the iterator."""
    if (time_step := ttdb_time_step_iter__next(self._iter)) is None:
      raise StopIteration
    return TimeStep(time_step)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

ttdb_series_t = NewType("ttdb_series_t", c_void_p)
ttdb_series__close: Final = cast(
    Callable[[ttdb_series_t], None],
    lib.func("ttdb_series__close", (c_void_p,), None))
ttdb_series__num_time_steps: Final = cast(
    Callable[[ttdb_series_t], int],
    lib.func("ttdb_series__num_time_steps", (c_void_p,), c_uint64))
ttdb_series__last_time_step: Final = cast(
    Callable[[ttdb_series_t], ttdb_time_step_t],
    lib.func("ttdb_series__last_time_step", (c_void_p,), c_void_p))
ttdb_series__time_steps: Final = cast(
    Callable[[ttdb_series_t], ttdb_time_step_iter_t],
    lib.func("ttdb_series__time_steps", (c_void_p,), c_void_p))

@final
class Series:
  """Time series in a storage."""

  _series: ttdb_series_t

  def __init__(self, series: ttdb_series_t) -> None:
    """Initialize the series with a C pointer."""
    self._series = series

  def __del__(self) -> None:
    """Close the series."""
    ttdb_series__close(self._series)

  @property
  def num_time_steps(self) -> int:
    """Number of time steps in the series."""
    return ttdb_series__num_time_steps(self._series)

  @property
  def last_time_step(self) -> TimeStep:
    """Last time step in the series."""
    return TimeStep(ttdb_series__last_time_step(self._series))

  def time_steps(self) -> TimeStepIter:
    """Iterate over all time steps in the series."""
    return TimeStepIter(ttdb_series__time_steps(self._series))

ttdb_series_iter_t = NewType("ttdb_series_iter_t", c_void_p)
ttdb_series_iter__next: Final = cast(
    Callable[[ttdb_series_iter_t], ttdb_series_t | None],
    lib.func("ttdb_series_iter__next", (c_void_p,), c_void_p))
ttdb_series_iter__close: Final = cast(
    Callable[[ttdb_series_iter_t], None],
    lib.func("ttdb_series_iter__close", (c_void_p,), None))

@final
class SeriesIter(Iterator[Series]):
  """Iterator over series in a storage."""

  _iter: ttdb_series_iter_t

  def __init__(self, iterator: ttdb_series_iter_t) -> None:
    """Initialize the iterator with a C pointer."""
    self._iter = iterator

  def __del__(self) -> None:
    """Close the iterator."""
    ttdb_series_iter__close(self._iter)

  def __iter__(self) -> SeriesIter:
    """Return the iterator itself."""
    return self

  def __next__(self) -> Series:
    """Get the next series from the iterator."""
    if (series := ttdb_series_iter__next(self._iter)) is None:
      raise StopIteration
    return Series(series)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

ttdb_t = NewType("ttdb_t", c_void_p)
ttdb__close: Final = cast(  #
    Callable[[ttdb_t], None],  #
    lib.func("ttdb__close", (c_void_p,), None))
ttdb__open: Final = cast(  #
    Callable[[bytes], ttdb_t],  #
    lib.func("ttdb__open", (c_char_p,), c_void_p))
ttdb__num_series: Final = cast(
    Callable[[ttdb_t], int],  #
    lib.func("ttdb__num_series", (c_void_p,), c_uint64))
ttdb__last_series: Final = cast(
    Callable[[ttdb_t], ttdb_series_t],
    lib.func("ttdb__last_series", (c_void_p,), c_void_p))
ttdb__series: Final = cast(  #
    Callable[[ttdb_t], ttdb_series_iter_t],
    lib.func("ttdb__series", (c_void_p,), c_void_p))

@final
class Storage:
  """BlueTit particle storage."""

  _ttdb: ttdb_t

  def __init__(self, ttdb: ttdb_t) -> None:
    """Initialize the storage with a C pointer."""
    self._ttdb = ttdb

  def __del__(self) -> None:
    """Close the storage."""
    ttdb__close(self._ttdb)

  @property
  def num_series(self) -> int:
    """Number of series in the storage."""
    return ttdb__num_series(self._ttdb)

  @property
  def last_series(self) -> Series:
    """Last series in the storage."""
    return Series(ttdb__last_series(self._ttdb))

  def series(self) -> SeriesIter:
    """Iterate over all series in the storage."""
    return SeriesIter(ttdb__series(self._ttdb))

def open_storage(path: str) -> Storage:
  """Open the storage at the given path."""
  return Storage(ttdb__open(path.encode("utf-8")))

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
