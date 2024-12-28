# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of the Tit Solver project, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

import numpy
from .data import DataArray


def to_numpy(data: DataArray):
    """Convert the data array to a NumPy array."""
    memory, capsule = data.data
    match data.type.rank:
        case 0:  # DataRank.SCALAR:
            shape = len(memory) // 8
        case 1:  # DataRank.VECTOR:
            shape = (len(memory) // 8 // data.type.dim, data.type.dim)
        case 2:  # DataRank.MATRIX:
            shape = (
                len(memory) // 8 // (data.type.dim**2),
                data.type.dim,
                data.type.dim,
            )
    array = numpy.frombuffer(memory, dtype=numpy.float64).reshape(shape)
    # setattr(array, "__capsule__", capsule)
    return array
