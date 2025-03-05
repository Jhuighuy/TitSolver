# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
# Commercial use, including SaaS, requires a separate license, see /LICENSE.md
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# pylint: disable=import-error,invalid-name,missing-docstring
from __future__ import annotations
import os
from collections.abc import Iterator
from ctypes import CDLL, c_char_p, c_double, c_uint32, c_uint64, c_void_p
from enum import Enum
from typing import Any
import numpy as np

__all__ = (
    "Type",
    "Kind",
    "Rank",
    "Array",
    "ArrayIter",
    "Dataset",
    "TimeStep",
    "TimeStepIter",
    "Series",
    "SeriesIter",
    "Storage",
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

ttdb = CDLL(os.path.dirname(__file__) + "/libttdb.so")


def ttdb_func(name: str, ret_type: type | None, args: tuple[type, ...]) -> Any:
    func = getattr(ttdb, name)
    func.argtypes = args
    func.restype = ret_type
    return func


# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


type_t = c_uint32
ttdb_type__kind = ttdb_func("ttdb_type__kind", c_char_p, (type_t,))
ttdb_type__rank = ttdb_func("ttdb_type__rank", c_uint32, (type_t,))
ttdb_type__dim = ttdb_func("ttdb_type__dim", c_uint32, (type_t,))


class Kind(Enum):
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

    def to_numpy(self) -> type:
        return {
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
        }[self]


class Rank(Enum):
    scalar = 0
    vector = 1
    matrix = 2


class Type:
    def __init__(self, type_: type_t) -> None:
        self._type = type_

    @property
    def kind(self) -> Kind:
        return Kind(ttdb_type__kind(self._type).decode("utf-8"))

    @property
    def rank(self) -> Rank:
        return Rank(ttdb_type__rank(self._type))

    @property
    def dim(self) -> int:
        return ttdb_type__dim(self._type)


# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

ttdb_array_t = c_void_p
ttdb_array__close = ttdb_func("ttdb_array__close", None, (ttdb_array_t,))
ttdb_array__name = ttdb_func("ttdb_array__name", c_char_p, (ttdb_array_t,))
ttdb_array__size = ttdb_func("ttdb_array__size", c_uint64, (ttdb_array_t,))
ttdb_array__type = ttdb_func("ttdb_array__type", type_t, (ttdb_array_t,))
ttdb_array__read = ttdb_func("ttdb_array__read", None, (ttdb_array_t, c_void_p))


class Array:
    def __init__(self, array: ttdb_array_t) -> None:
        self._array = array

    def __del__(self) -> None:
        ttdb_array__close(self._array)

    @property
    def name(self) -> str:
        return ttdb_array__name(self._array).decode("utf-8")

    @property
    def size(self) -> int:
        return ttdb_array__size(self._array)

    @property
    def type(self) -> Type:
        return Type(ttdb_array__type(self._array))

    def read(self, data: Any) -> None:
        ttdb_array__read(self._array, data)

    @property
    def data(self) -> np.ndarray:
        shape = (self.size,) + (self.type.dim,) * self.type.rank.value
        array: np.ndarray = np.empty(shape, dtype=self.type.kind.to_numpy())
        self.read(array.ctypes.data)
        return array


ttdb_array_iter_t = c_void_p
ttdb_array_iter__close = ttdb_func(
    "ttdb_array_iter__close", None, (ttdb_array_iter_t,)
)
ttdb_array_iter__next = ttdb_func(
    "ttdb_array_iter__next", ttdb_array_t, (ttdb_array_iter_t,)
)


class ArrayIter(Iterator[Array]):
    def __init__(self, iterator: ttdb_array_iter_t) -> None:
        self._iter = iterator

    def __del__(self) -> None:
        ttdb_array_iter__close(self._iter)

    def __iter__(self) -> ArrayIter:
        return self

    def __next__(self) -> Array:
        if (array := ttdb_array_iter__next(self._iter)) is None:
            raise StopIteration
        return Array(array)


# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

ttdb_dataset_t = c_void_p
ttdb_dataset__close = ttdb_func("ttdb_dataset__close", None, (ttdb_dataset_t,))
ttdb_dataset__num_arrays = ttdb_func(
    "ttdb_dataset__num_arrays", c_uint64, (ttdb_dataset_t,)
)
ttdb_dataset__find_array = ttdb_func(
    "ttdb_dataset__find_array", ttdb_array_t, (ttdb_dataset_t, c_char_p)
)
ttdb_dataset__arrays = ttdb_func(
    "ttdb_dataset__arrays", ttdb_array_iter_t, (ttdb_dataset_t,)
)


class Dataset:
    def __init__(self, dataset: ttdb_dataset_t) -> None:
        self._dataset = dataset

    def __del__(self) -> None:
        ttdb_dataset__close(self._dataset)

    @property
    def num_arrays(self) -> int:
        return ttdb_dataset__num_arrays(self._dataset)

    def find_array(self, name: str) -> Array | None:
        array = ttdb_dataset__find_array(self._dataset, name.encode("utf-8"))
        if array is None:
            return None
        return Array(array)

    def arrays(self) -> ArrayIter:
        return ArrayIter(ttdb_dataset__arrays(self._dataset))


# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

ttdb_time_step_t = c_void_p
ttdb_time_step__close = ttdb_func(
    "ttdb_time_step__close", None, (ttdb_time_step_t,)
)
ttdb_time_step__time = ttdb_func(
    "ttdb_time_step__time", c_double, (ttdb_time_step_t,)
)
ttdb_time_step__uniforms = ttdb_func(
    "ttdb_time_step__uniforms", ttdb_dataset_t, (ttdb_time_step_t,)
)
ttdb_time_step__varyings = ttdb_func(
    "ttdb_time_step__varyings", ttdb_dataset_t, (ttdb_time_step_t,)
)


class TimeStep:
    def __init__(self, time_step: ttdb_time_step_t) -> None:
        self._time_step = time_step

    def __del__(self) -> None:
        ttdb_time_step__close(self._time_step)

    @property
    def time(self) -> float:
        return ttdb_time_step__time(self._time_step)

    @property
    def uniforms(self) -> Dataset:
        return Dataset(ttdb_time_step__uniforms(self._time_step))

    @property
    def varyings(self) -> Dataset:
        return Dataset(ttdb_time_step__varyings(self._time_step))


ttdb_time_step_iter_t = c_void_p
ttdb_time_step_iter__close = ttdb_func(
    "ttdb_time_step_iter__close", None, (ttdb_time_step_iter_t,)
)
ttdb_time_step_iter__next = ttdb_func(
    "ttdb_time_step_iter__next", ttdb_time_step_t, (ttdb_time_step_iter_t,)
)


class TimeStepIter(Iterator[TimeStep]):
    def __init__(self, iterator: ttdb_time_step_iter_t) -> None:
        self._iter = iterator

    def __del__(self) -> None:
        ttdb_time_step_iter__close(self._iter)

    def __iter__(self) -> TimeStepIter:
        return self

    def __next__(self) -> TimeStep:
        if (time_step := ttdb_time_step_iter__next(self._iter)) is None:
            raise StopIteration
        return TimeStep(time_step)


# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

ttdb_series_t = c_void_p
ttdb_series__close = ttdb_func("ttdb_series__close", None, (ttdb_series_t,))
ttdb_series__num_time_steps = ttdb_func(
    "ttdb_series__num_time_steps", c_uint64, (ttdb_series_t,)
)
ttdb_series__last_time_step = ttdb_func(
    "ttdb_series__last_time_step", ttdb_time_step_t, (ttdb_series_t,)
)
ttdb_series__time_steps = ttdb_func(
    "ttdb_series__time_steps", ttdb_time_step_iter_t, (ttdb_series_t,)
)


class Series:
    def __init__(self, series: ttdb_series_t) -> None:
        self._series = series

    def __del__(self) -> None:
        ttdb_series__close(self._series)

    @property
    def num_time_steps(self) -> int:
        return ttdb_series__num_time_steps(self._series)

    @property
    def last_time_step(self) -> TimeStep:
        return TimeStep(ttdb_series__last_time_step(self._series))

    def time_steps(self) -> TimeStepIter:
        return TimeStepIter(ttdb_series__time_steps(self._series))


ttdb_series_iter_t = c_void_p
ttdb_series_iter__next = ttdb_func(
    "ttdb_series_iter__next", ttdb_series_t, (ttdb_series_iter_t,)
)
ttdb_series_iter__close = ttdb_func(
    "ttdb_series_iter__close", None, (ttdb_series_iter_t,)
)


class SeriesIter(Iterator[Series]):
    def __init__(self, iterator: ttdb_series_iter_t) -> None:
        self._iter = iterator

    def __del__(self) -> None:
        ttdb_series_iter__close(self._iter)

    def __iter__(self) -> SeriesIter:
        return self

    def __next__(self) -> Series:
        if (series := ttdb_series_iter__next(self._iter)) is None:
            raise StopIteration
        return Series(series)


# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

ttdb_t = c_void_p
ttdb__close = ttdb_func("ttdb__close", None, (ttdb_t,))
ttdb__open = ttdb_func("ttdb__open", ttdb_t, (c_char_p,))
ttdb__num_series = ttdb_func("ttdb__num_series", c_uint64, (ttdb_t,))
ttdb__last_series = ttdb_func("ttdb__last_series", ttdb_series_t, (ttdb_t,))
ttdb__series = ttdb_func("ttdb__series", ttdb_series_iter_t, (ttdb_t,))


class Storage:
    def __init__(self, path: str) -> None:
        self._ttdb = ttdb__open(path.encode("utf-8"))

    def __del__(self) -> None:
        ttdb__close(self._ttdb)

    @property
    def num_series(self) -> int:
        return ttdb__num_series(self._ttdb)

    @property
    def last_series(self) -> Series:
        return Series(ttdb__last_series(self._ttdb))

    def series(self) -> SeriesIter:
        return SeriesIter(ttdb__series(self._ttdb))


# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
