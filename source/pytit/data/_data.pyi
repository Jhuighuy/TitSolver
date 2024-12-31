# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# pylint: disable=unused-argument
from __future__ import annotations
from enum import IntEnum
from collections.abc import Hashable, Sequence
import numpy
from numpy.typing import NDArray

class DataRank(IntEnum):
    SCALAR = 1
    VECTOR = 2
    MATRIX = 3

class DataKind(IntEnum):
    UNKNOWN = 0
    INT_8 = 1
    UINT_8 = 2
    INT_16 = 3
    UINT_16 = 4
    INT_32 = 5
    UINT_32 = 6
    INT_64 = 7
    UINT_64 = 8
    FLOAT_32 = 9
    FLOAT_64 = 10

class DataType(Hashable):
    kind: DataKind
    rank: DataRank
    dim: int

    def __hash__(self) -> int: ...

class DataArray(Hashable):
    type: DataType
    data: NDArray[numpy.int_ | numpy.float_]

    def __hash__(self) -> int: ...

class DataSet(Hashable):
    num_arrays: int
    arrays: Sequence[tuple[str, DataArray]]

    def __hash__(self) -> int: ...
    def find_array(self, name: str) -> DataArray | None: ...
    def create_array(self, name: str) -> DataArray: ...

class TimeStep(Hashable):
    time: float
    uniforms: DataSet
    varyings: DataSet

    def __hash__(self) -> int: ...

class Series(Hashable):
    parameters: str
    num_time_steps: int
    time_steps: Sequence[TimeStep]
    last_time_step: TimeStep

    def __hash__(self) -> int: ...
    def create_time_step(self, time: float) -> TimeStep: ...

class Storage:
    path: str
    max_series: int
    num_series: int
    series: Sequence[Series]
    last_series: Series

    def __init__(self, file_name: str) -> None: ...
    def __hash__(self) -> int: ...
    def create_series(self, parameters: str) -> Series: ...
    def delete_series(self, series: Series) -> None: ...
    def delete_time_step(self, time_step: TimeStep) -> None: ...
    def delete_array(self, array: DataArray) -> None: ...
