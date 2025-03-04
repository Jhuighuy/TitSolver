# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
# Commercial use, including SaaS, requires a separate license, see /LICENSE.md
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# pylint: disable=unused-argument
from __future__ import annotations
from collections.abc import Hashable, Sequence
from numpy.typing import NDArray
from pytit.utils.abc import RichComparable, TitEnum

class DataRank(TitEnum):
    """Data rank."""

    SCALAR = 1
    VECTOR = 2
    MATRIX = 3

class DataKind(TitEnum):
    """Data kind."""

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

class DataType(Hashable, RichComparable):
    """Data type."""

    kind: DataKind
    rank: DataRank
    dim: int

class DataArray(Hashable, RichComparable):
    """Array of data in a data set."""

    type: DataType
    data: NDArray

class DataSet(Hashable, RichComparable):
    """Data set of a time step."""

    num_arrays: int
    arrays: Sequence[tuple[str, DataArray]]

    def find_array(self, name: str) -> DataArray | None: ...
    def create_array(self, name: str) -> DataArray: ...

class TimeStep(Hashable, RichComparable):
    """Time step in a series."""

    time: float
    uniforms: DataSet
    varyings: DataSet

    def __hash__(self) -> int: ...

class Series(Hashable, RichComparable):
    """Data series."""

    parameters: str
    num_time_steps: int
    time_steps: Sequence[TimeStep]
    last_time_step: TimeStep

    def create_time_step(self, time: float) -> TimeStep: ...

class Storage(Hashable, RichComparable):
    """Data storage."""

    path: str
    max_series: int
    num_series: int
    series: Sequence[Series]
    last_series: Series

    def __init__(self, file_name: str) -> None: ...
    def create_series(self, parameters: str) -> Series: ...
    def delete_series(self, series: Series) -> None: ...
    def delete_time_step(self, time_step: TimeStep) -> None: ...
    def delete_array(self, array: DataArray) -> None: ...
