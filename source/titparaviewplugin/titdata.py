# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

"""Python API for the Tit Solver data library."""

from __future__ import annotations
from ctypes import (
    _Pointer,
    c_bool,
    c_char_p,
    c_double,
    c_float,
    c_int16,
    c_int32,
    c_int64,
    c_int8,
    c_uint16,
    c_uint32,
    c_uint64,
    c_uint8,
    c_void_p,
    cast,
    cdll,
    POINTER,
    sizeof,
    Structure,
)
from enum import Enum
import os
from typing import Any, Callable, NoReturn


# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


class Dkind(Enum):
    """Data kind specification."""

    INT8 = "int8"
    UINT8 = "uint8"
    INT16 = "int16"
    UINT16 = "uint16"
    INT32 = "int32"
    UINT32 = "uint32"
    INT64 = "int64"
    UINT64 = "uint64"
    FLOAT32 = "float32"
    FLOAT64 = "float64"

    def __str__(self) -> str:
        """Get the string representation of the data kind."""
        return self.value

    @property
    def pytype(self) -> type:
        """Python type."""
        match self:
            case (
                Dkind.INT8
                | Dkind.UINT8
                | Dkind.INT16
                | Dkind.UINT16
                | Dkind.INT32
                | Dkind.UINT32
                | Dkind.INT64
                | Dkind.UINT64
            ):
                return int
            case Dkind.FLOAT32 | Dkind.FLOAT64:
                return float
            case _:
                assert False

    @property
    def ctype(self) -> type:
        """Native type."""
        match self:
            case Dkind.INT8:
                return c_int8
            case Dkind.UINT8:
                return c_uint8
            case Dkind.INT16:
                return c_int16
            case Dkind.UINT16:
                return c_uint16
            case Dkind.INT32:
                return c_int32
            case Dkind.UINT32:
                return c_uint32
            case Dkind.INT64:
                return c_int64
            case Dkind.UINT64:
                return c_uint64
            case Dkind.FLOAT32:
                return c_float
            case Dkind.FLOAT64:
                return c_double
            case _:
                assert False


# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


class Dtype:
    """Data type specification."""

    _kind: Dkind
    _rank: int
    _dim: int

    def __init__(self, dtype_id: int):
        if not _Lib.check_dtype(dtype_id):
            raise RuntimeError(f"Data type with ID {dtype_id} does not exist!")
        self._kind = Dkind(_Lib.dtype_kind(dtype_id))
        self._rank = _Lib.dtype_rank(dtype_id)
        self._dim = _Lib.dtype_dim(dtype_id)

    def __str__(self) -> str:
        """
        Get the string representation of the data type.
        :returns: String representation of the data type.
        """
        match self._rank:
            case 0:
                assert self.dim == 1, "Invalid dimensionality!"
                string = self.kind.value
            case 1:
                string = f"Vec<{self.kind.value}, {self.dim}>"
            case 2:
                string = f"Mat<{self.kind.value}, {self.dim}>"
            case _:
                assert False, f"Invalid rank {self.rank}!"
        return string

    @property
    def kind(self) -> Dkind:
        """Data kind."""
        return self._kind

    @property
    def rank(self) -> int:
        """Rank (scalar, vector, or matrix)."""
        return self._rank

    @property
    def dim(self) -> int:
        """Dimensionality."""
        return self._dim


# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


class Darray:
    """Data array."""

    _dtype: Dtype
    _handle: _Handle
    _data: _Pointer
    _num_items: int
    _item_size: int

    def __init__(self, dtype: Dtype, handle: _Handle):
        """
        Initialize the data array.

        :param dtype: Data type specification.
        :param handle: C handle to the data array.
        """
        if not handle:
            raise RuntimeError("Failed to create the data array!")
        self._handle = handle
        self._dtype = dtype
        self._data = cast(_Lib.darray_data(handle), POINTER(dtype.kind.ctype))
        self._item_size = dtype.dim**dtype.rank
        self._num_items = _Lib.darray_size(handle) // (
            self._item_size * sizeof(dtype.kind.ctype)
        )

    def __del__(self) -> None:
        """Close the data array."""
        _Lib.darray_close(self._handle)

    def __str__(self) -> str:
        """Get the string representation of the data array."""
        return f"Darray<{self.dtype}>({len(self)})"

    @property
    def dtype(self) -> Dtype:
        """Data type specification."""
        return self._dtype

    def __len__(self) -> int:
        """Get the number of items in the data array."""
        return self._num_items

    def __getitem__(self, index: int) -> _Ditem:
        """
        Get a data array item.

        :param index: Index of the item.
        :returns: Array item: scalar, vector, or matrix.
        """
        if index < 0 or index >= len(self):
            self._raise_out_of_range(index)
        pytype = self._dtype.kind.pytype
        if self.dtype.rank == 0:
            return pytype(self._data[index])
        dim = self.dtype.dim
        start = index * self._item_size
        elems = self._data[start : start + self._item_size]
        if self.dtype.rank == 1:
            return tuple(map(pytype, elems[:dim]))
        if self.dtype.rank == 2:
            return tuple(
                tuple(map(pytype, elems[i * dim : (i + 1) * dim])) for i in range(dim)
            )
        assert False

    def __setitem__(self, index: int, value: _Ditem):
        """
        Set a data array item.

        :param index: Index of the item.
        :param value: Array item: scalar, vector, or matrix.
        """
        if index < 0 or index >= len(self):
            self._raise_out_of_range(index)
        pytype = self._dtype.kind.pytype
        if self.dtype.rank == 0:
            if not isinstance(value, pytype):
                self._raise_type_mismatch(value)
            self._data[index] = value
            return
        dim = self.dtype.dim
        start = index * self._item_size
        elems = self._data[start : start + self._item_size]
        if self.dtype.rank == 1:
            if not (
                isinstance(value, (tuple, list))
                and len(value) == dim
                and all(isinstance(elem, pytype) for elem in value)
            ):
                self._raise_type_mismatch(value)
            elems[:dim] = value
            return
        if self.dtype.rank == 2:
            if not (
                isinstance(value, (tuple, list))
                and len(value) == dim
                and all(
                    isinstance(elem, (tuple, list))
                    and len(elem) == dim
                    and all(isinstance(elem, pytype) for elem in value)
                    for elem in value
                )
            ):
                self._raise_type_mismatch(value)
            for i, row in enumerate(value):
                start = i * dim
                elems[start : start + dim] = row  # type: ignore
            return
        assert False

    def _raise_out_of_range(self, index: int) -> NoReturn:
        raise IndexError(f"Index out of bounds: {index}, size is {len(self)}!")

    def _raise_type_mismatch(self, value: _Ditem) -> NoReturn:
        pytype: object = self._dtype.kind.pytype
        for _ in range(self.dtype.rank):
            pytype = (pytype,) * self.dtype.dim
        raise TypeError(
            f"Value type mismatch, expected '{pytype}', got '{type(value)}'!"
        )


_Dvalue = int | float
_Dvector = tuple[_Dvalue, ...]
_Dmatrix = tuple[_Dvector, ...]
_Ditem = _Dvalue | _Dvector | _Dmatrix


# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


class DataArrayView:
    """Data array view."""

    _storage: DataStorage
    _array_id: int

    def __init__(self, storage: DataStorage, array_id: int):
        """
        Initialize the data array view.

        :param storage: Data storage.
        :param array_id: Array ID.
        """
        self._storage = storage
        self._array_id = array_id
        if not _Lib.check_array(self._storage.handle, self._array_id):
            raise RuntimeError(f"Data array with ID {self._array_id} does not exist!")

    def __repr__(self) -> str:
        """Get the string representation of the data array view."""
        return f"DataArrayView('{self._storage.path}', {self._array_id})"

    def __hash__(self) -> int:
        """Get the hash of the data array view."""
        return hash((self._storage, self._array_id))

    def __eq__(self, other: object) -> bool:
        """Check if the data array view is equal to another object."""
        return (
            isinstance(other, DataArrayView)
            and self._storage == other._storage
            and self._array_id == other._array_id
        )

    @property
    def storage(self) -> DataStorage:
        """Data storage that contains the array."""
        return self._storage

    @property
    def id(self) -> int:
        """Array ID."""
        return self._array_id

    @property
    def dtype(self) -> Dtype:
        """Data type specification."""
        return Dtype(_Lib.array_dtype(self._storage.handle, self._array_id))

    def read_data(self) -> Darray:
        """Read all the data from the storage into a `Darray`."""
        return Darray(self.dtype, _Lib.array_data(self._storage.handle, self._array_id))


# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


class DataSetView:
    """Data set view."""

    _storage: DataStorage
    _set_id: int

    def __init__(self, storage: DataStorage, set_id: int):
        """
        Initialize the data set view.

        :param storage: Data storage.
        :param set_id: Set ID.
        """
        self._storage = storage
        self._set_id = set_id
        if not _Lib.check_set(self._storage.handle, self._set_id):
            raise RuntimeError(f"Data set with ID {self._set_id} does not exist!")

    def __repr__(self) -> str:
        """Get the string representation of the data set view."""
        return f"DataSetView('{self._storage.path}', {self._set_id})"

    def __hash__(self) -> int:
        """Get the hash of the data set view."""
        return hash((self._storage, self._set_id))

    def __eq__(self, other: object) -> bool:
        """Check if the data set view is equal to another object."""
        return (
            isinstance(other, DataSetView)
            and self._storage == other._storage
            and self._set_id == other._set_id
        )

    @property
    def storage(self) -> DataStorage:
        """Data storage that contains the set."""
        return self._storage

    @property
    def id(self) -> int:
        """Set ID."""
        return self._set_id

    @property
    def num_arrays(self) -> int:
        """Number of arrays in the set."""
        return _Lib.set_num_arrays(self._storage.handle, self._set_id)

    @property
    def arrays(self) -> dict[str, DataArrayView]:
        """Array views in the set."""
        array_names = str(
            _Lib.data_set_array_names(self._storage.handle, self._set_id)
        ).split(":")
        return {
            name: DataArrayView(self._storage, array_id)
            for name, array_id in zip(
                array_names, _Lib.set_arrays(self._storage.handle, self._set_id)
            )
        }


# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


class DataTimeStepView:
    """Data time step view."""

    _storage: DataStorage
    _time_step_id: int

    def __init__(self, storage: DataStorage, time_step_id: int):
        """
        Initialize the data time step view.

        :param storage: Data storage.
        :param time_step_id: Time step ID.
        """
        self._storage = storage
        self._time_step_id = time_step_id
        if not _Lib.check_time_step(self._storage.handle, self._time_step_id):
            raise RuntimeError(
                f"Time step with ID {self._time_step_id} does not exist!"
            )

    def __repr__(self) -> str:
        """Get the string representation of the data time step view."""
        return f"DataTimeStepView('{self._storage.path}', {self._time_step_id})"

    def __hash__(self) -> int:
        """Get the hash of the data time step view."""
        return hash((self._storage, self._time_step_id))

    def __eq__(self, other: object) -> bool:
        """Check if the data time step view is equal to another object."""
        return (
            isinstance(other, DataTimeStepView)
            and self._storage == other._storage
            and self._time_step_id == other._time_step_id
        )

    @property
    def storage(self) -> DataStorage:
        """Data storage that contains the time step."""
        return self._storage

    @property
    def id(self) -> int:
        """Time step ID."""
        return self._time_step_id

    @property
    def time(self) -> float:
        """Simulation time."""
        return _Lib.time_step_time(self._storage.handle, self._time_step_id)

    @property
    def uniforms(self) -> DataSetView:
        """Uniform data set view."""
        return DataSetView(
            self._storage,
            _Lib.time_step_uniforms(self._storage.handle, self._time_step_id),
        )

    @property
    def varyings(self) -> DataSetView:
        """Varying data set view."""
        return DataSetView(
            self._storage,
            _Lib.time_step_varyings(self._storage.handle, self._time_step_id),
        )


# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


class DataSeriesView:
    """Data series view."""

    _storage: DataStorage
    _series_id: int

    def __init__(self, storage: DataStorage, series_id: int):
        """
        Initialize the data series view.

        :param storage: Data storage.
        :param series_id: Series ID.
        """
        self._storage = storage
        self._series_id = series_id
        if not _Lib.check_series(self._storage.handle, self._series_id):
            raise RuntimeError(f"Series with ID {self._series_id} does not exist!")

    def __repr__(self) -> str:
        """Get the string representation of the data series view."""
        return f"DataSeriesView('{self._storage.path}', {self._series_id})"

    def __hash__(self) -> int:
        """Get the hash of the data series view."""
        return hash((self._storage, self._series_id))

    def __eq__(self, other: object) -> bool:
        """Check if the data series view is equal to another object."""
        return (
            isinstance(other, DataSeriesView)
            and self._storage == other._storage
            and self._series_id == other._series_id
        )

    @property
    def storage(self) -> DataStorage:
        """Data storage that contains the series."""
        return self._storage

    @property
    def id(self) -> int:
        """Series ID."""
        return self._series_id

    @property
    def parameters(self) -> str:
        """Series parameters."""
        return str(_Lib.series_parameters(self._storage.handle, self._series_id))

    @property
    def num_time_steps(self) -> int:
        """Number of time steps in the series."""
        return _Lib.series_num_time_steps(self._storage.handle, self._series_id)

    @property
    def time_steps(self) -> list[DataTimeStepView]:
        """Time step views in the series."""
        return [
            DataTimeStepView(self._storage, time_step_id)
            for time_step_id in _Lib.series_time_steps(
                self._storage.handle, self._series_id
            )
        ]


# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


class DataStorage:
    """Data storage."""

    _handle: _Handle

    def __init__(self, path: str):
        """
        Open the data storage.

        :param path: Path to the data storage.
        """
        self._handle = _Lib.storage_open(path)
        if not self._handle:
            raise RuntimeError(f"Failed to open the data storage at '{path}'!")

    def __del__(self) -> None:
        """Close the data storage."""
        _Lib.storage_close(self._handle)

    def __repr__(self) -> str:
        """Get the string representation of the data storage."""
        return f"DataStorage('{self.path}')"

    def __hash__(self) -> int:
        """Get the hash of the data storage."""
        return hash(self.path)

    def __eq__(self, other: object) -> bool:
        """Check if the data storage is equal to another object."""
        return isinstance(other, DataStorage) and self.path == other.path

    @property
    def path(self) -> str:
        """Path to the data storage."""
        return _Lib.storage_path(self._handle)

    @property
    def handle(self) -> _Handle:
        """Native handle to the data storage."""
        return self._handle

    @property
    def num_series(self) -> int:
        """Number of series in the storage."""
        return _Lib.storage_num_series(self._handle)

    @property
    def series(self) -> list[DataSeriesView]:
        """Series views in the storage."""
        return [
            DataSeriesView(self, series_id)
            for series_id in _Lib.storage_series(self._handle)
        ]

    @property
    def last_series(self) -> DataSeriesView:
        """Last series view in the storage."""
        return DataSeriesView(self, _Lib.storage_last_series(self._handle))


# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# Internal handle type.
_Handle = c_void_p


# Internal C string.
# pylint: disable=missing-class-docstring,missing-function-docstring
class _Str:
    _c_str: c_char_p

    def __init__(self, c_str: c_char_p):
        self._c_str = c_str

    def __del__(self) -> None:
        # TODO: why are we segfaulting here?
        # _Lib.free(self._c_str)
        pass

    def __str__(self) -> str:
        return bytes(self._c_str).decode("utf-8")


# ID array structure.
class _IDsStruct(Structure):
    _fields_ = [("ids", POINTER(c_uint64)), ("size", c_uint64)]


# Internal ID array.
# pylint: disable=missing-class-docstring,missing-function-docstring
class _IDs:
    _ids: _Pointer[c_uint64]
    _size: int

    def __init__(self, ids: _IDsStruct):
        self._ids = ids.ids
        self._size = ids.size

    def __del__(self) -> None:
        _Lib.free(self._ids)

    def __len__(self) -> int:
        return self._size

    def __getitem__(self, index: int) -> int:
        if not 0 <= index < self._size:
            raise IndexError(f"Index out of bounds: {index}, size is {self._size}!")
        return int(self._ids[index])


# Internal C API.
# pylint: disable=missing-class-docstring,missing-function-docstring
class _Lib:

    _lib = cdll.LoadLibrary(
        os.path.join(os.path.dirname(__file__), "libtitdata.dylib"),
    )

    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    _get_error = _lib.tit_get_error
    _get_error.argtypes = []
    _get_error.restype = c_char_p

    @staticmethod
    def _checked_call(func: Callable[..., Any]) -> Callable[..., Any]:
        def wrapper(*args: Any, **kwargs: Any) -> Any:
            if error := _Lib._get_error():
                raise RuntimeError(error.decode())
            return func(*args, **kwargs)

        return wrapper

    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    _free = _lib.free
    _free.argtypes = [c_void_p]
    _free.restype = None

    @staticmethod
    @_checked_call
    def free(ptr: c_void_p) -> None:
        _Lib._free(ptr)

    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    _storage_open = _lib.tit_storage_open
    _storage_open.argtypes = [c_char_p]
    _storage_open.restype = c_void_p

    @staticmethod
    @_checked_call
    def storage_open(path: str) -> _Handle:
        return _Lib._storage_open(path.encode(), None)

    _storage_close = _lib.tit_storage_close
    _storage_close.argtypes = [c_void_p]
    _storage_close.restype = None

    @staticmethod
    @_checked_call
    def storage_close(storage: _Handle) -> None:
        _Lib._storage_close(storage)

    _storage_path = _lib.tit_storage_path
    _storage_path.argtypes = [c_void_p]
    _storage_path.restype = c_char_p

    @staticmethod
    @_checked_call
    def storage_path(storage: _Handle) -> _Str:
        return _Str(_Lib._storage_path(storage))

    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    _storage_num_series = _lib.tit_storage_num_series
    _storage_num_series.argtypes = [c_void_p]
    _storage_num_series.restype = c_uint64

    @staticmethod
    @_checked_call
    def storage_num_series(storage: _Handle) -> int:
        return int(_Lib._storage_num_series(storage))

    _storage_series = _lib.tit_storage_series
    _storage_series.argtypes = [c_void_p]
    _storage_series.restype = _IDsStruct

    @staticmethod
    @_checked_call
    def storage_series(storage: _Handle) -> _IDs:
        return _IDs(_Lib._storage_series(storage))

    _storage_last_series = _lib.tit_last_series
    _storage_last_series.argtypes = [c_void_p]
    _storage_last_series.restype = c_uint64

    @staticmethod
    @_checked_call
    def storage_last_series(storage: _Handle) -> int:
        return int(_Lib._storage_last_series(storage))

    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    _check_series = _lib.tit_check_series
    _check_series.argtypes = [c_void_p, c_uint64]
    _check_series.restype = c_bool

    @staticmethod
    @_checked_call
    def check_series(storage: _Handle, series_id: int) -> bool:
        return bool(_Lib._check_series(storage, series_id))

    _series_parameters = _lib.tit_series_parameters
    _series_parameters.argtypes = [c_void_p, c_uint64]
    _series_parameters.restype = c_char_p

    @staticmethod
    @_checked_call
    def series_parameters(storage: _Handle, series_id: int) -> _Str:
        return _Str(_Lib._series_parameters(storage, series_id))

    _series_num_time_steps = _lib.tit_series_num_time_steps
    _series_num_time_steps.argtypes = [c_void_p, c_uint64]
    _series_num_time_steps.restype = c_uint64

    @staticmethod
    @_checked_call
    def series_num_time_steps(storage: _Handle, series_id: int) -> int:
        return int(_Lib._series_num_time_steps(storage, series_id))

    _series_time_steps = _lib.tit_series_time_steps
    _series_time_steps.argtypes = [c_void_p, c_uint64]
    _series_time_steps.restype = _IDsStruct

    @staticmethod
    @_checked_call
    def series_time_steps(storage: _Handle, series_id: int) -> _IDs:
        return _IDs(_Lib._series_time_steps(storage, series_id))

    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    _check_time_step = _lib.tit_check_time_step
    _check_time_step.argtypes = [c_void_p, c_uint64]
    _check_time_step.restype = c_bool

    @staticmethod
    @_checked_call
    def check_time_step(storage: _Handle, series_id: int) -> bool:
        return bool(_Lib._check_time_step(storage, series_id))

    _time_step_time = _lib.tit_time_step_time
    _time_step_time.argtypes = [c_void_p, c_uint64]
    _time_step_time.restype = c_float

    @staticmethod
    @_checked_call
    def time_step_time(storage: _Handle, time_step_id: int) -> float:
        return float(_Lib._time_step_time(storage, time_step_id))

    _time_step_uniforms = _lib.tit_time_step_uniforms
    _time_step_uniforms.argtypes = [c_void_p, c_uint64]
    _time_step_uniforms.restype = c_uint64

    @staticmethod
    @_checked_call
    def time_step_uniforms(storage: _Handle, time_step_id: int) -> int:
        return int(_Lib._time_step_uniforms(storage, time_step_id))

    _time_step_varyings = _lib.tit_time_step_varyings
    _time_step_varyings.argtypes = [c_void_p, c_uint64]
    _time_step_varyings.restype = c_uint64

    @staticmethod
    @_checked_call
    def time_step_varyings(storage: _Handle, time_step_id: int) -> int:
        return int(_Lib._time_step_varyings(storage, time_step_id))

    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    _check_set = _lib.tit_check_set
    _check_set.argtypes = [c_void_p, c_uint64]
    _check_set.restype = c_bool

    @staticmethod
    @_checked_call
    def check_set(storage: _Handle, set_id: int) -> bool:
        return bool(_Lib._check_set(storage, set_id))

    _set_num_arrays = _lib.tit_set_num_arrays
    _set_num_arrays.argtypes = [c_void_p, c_uint64]
    _set_num_arrays.restype = c_uint64

    @staticmethod
    @_checked_call
    def set_num_arrays(storage: _Handle, set_id: int) -> int:
        return int(_Lib._set_num_arrays(storage, set_id))

    _set_arrays = _lib.tit_set_arrays
    _set_arrays.argtypes = [c_void_p, c_uint64]
    _set_arrays.restype = _IDsStruct

    @staticmethod
    @_checked_call
    def set_arrays(storage: _Handle, set_id: int) -> _IDs:
        return _IDs(_Lib._set_arrays(storage, set_id))

    _set_array_names = _lib.tit_set_array_names
    _set_array_names.argtypes = [c_void_p, c_uint64]
    _set_array_names.restype = c_char_p

    @staticmethod
    @_checked_call
    def data_set_array_names(storage: _Handle, set_id: int) -> _Str:
        return _Str(_Lib._set_array_names(storage, set_id))

    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    _check_array = _lib.tit_check_array
    _check_array.argtypes = [c_void_p, c_uint64]
    _check_array.restype = c_bool

    @staticmethod
    @_checked_call
    def check_array(storage: _Handle, array_id: int) -> bool:
        return bool(_Lib._check_array(storage, array_id))

    _array_dtype = _lib.tit_array_dtype
    _array_dtype.argtypes = [c_void_p, c_uint64]
    _array_dtype.restype = c_uint64

    @staticmethod
    @_checked_call
    def array_dtype(storage: _Handle, array_id: int) -> int:
        return int(_Lib._array_dtype(storage, array_id))

    _array_data = _lib.tit_array_data
    _array_data.argtypes = [c_void_p, c_uint64]
    _array_data.restype = c_void_p

    @staticmethod
    @_checked_call
    def array_data(storage: _Handle, array_id: int) -> _Handle:
        return _Lib._array_data(storage, array_id)

    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    _darray_data = _lib.tit_darray_data
    _darray_data.argtypes = [c_void_p]
    _darray_data.restype = c_void_p

    @staticmethod
    @_checked_call
    def darray_data(darray: _Handle) -> _Handle:
        return _Lib._darray_data(darray)

    _darray_size = _lib.tit_darray_size
    _darray_size.argtypes = [c_void_p]
    _darray_size.restype = c_uint64

    @staticmethod
    @_checked_call
    def darray_size(darray: _Handle) -> int:
        return int(_Lib._darray_size(darray))

    _darray_close = _lib.tit_darray_close
    _darray_close.argtypes = [c_void_p]
    _darray_close.restype = None

    @staticmethod
    @_checked_call
    def darray_close(darray: _Handle) -> None:
        _Lib._darray_close(darray)

    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    _check_dtype = _lib.tit_check_dtype
    _check_dtype.argtypes = [c_uint32]
    _check_dtype.restype = c_bool

    @staticmethod
    @_checked_call
    def check_dtype(dtype_id: int) -> bool:
        return bool(_Lib._check_dtype(dtype_id))

    _dtype_kind = _lib.tit_dtype_kind
    _dtype_kind.argtypes = [c_uint32]
    _dtype_kind.restype = c_char_p

    @staticmethod
    @_checked_call
    def dtype_kind(dtype_id: int) -> str:
        # Note: no need to free the string, it is owned by the C library.
        return bytes(_Lib._dtype_kind(dtype_id)).decode("utf-8")

    _dtype_dim = _lib.tit_dtype_dim
    _dtype_dim.argtypes = [c_uint32]
    _dtype_dim.restype = c_uint64

    @staticmethod
    @_checked_call
    def dtype_dim(dtype_id: int) -> int:
        return int(_Lib._dtype_dim(dtype_id))

    _dtype_rank = _lib.tit_dtype_rank
    _dtype_rank.argtypes = [c_uint32]
    _dtype_rank.restype = c_uint64

    @staticmethod
    @_checked_call
    def dtype_rank(dtype_id: int) -> int:
        return int(_Lib._dtype_rank(dtype_id))


# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


def _main():
    ds = DataStorage("./particles.ttdb")
    print(f"Path: {ds.path}", flush=True)
    print(f"Num. series: {ds.num_series}", flush=True)

    series = ds.series[-1]
    time_step = series.time_steps[0]
    varying = time_step.varyings
    arrays = varying.arrays
    r = arrays["r"].read_data()
    v = arrays["v"].read_data()
    rho = arrays["rho"].read_data()

    with open("./particles_tdb.csv", "w") as csv_file:
        csv_file.write("r_x,r_y,v_x,v_y,rho\n")
        for i in range(len(r)):
            csv_file.write(f"{r[i][0]},{r[i][1]},{v[i][0]},{v[i][1]},{rho[i]}\n")


if __name__ == "__main__":
    _main()
