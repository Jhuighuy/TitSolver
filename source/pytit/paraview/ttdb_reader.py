# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
# Commercial use, including SaaS, requires a separate license, see /LICENSE.md
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

import os
import sys

# ParaView does not know where pytit is installed, let's tell it.
sys.path.append(os.path.abspath(os.path.dirname(__file__) + "/../.."))

# pylint: disable=wrong-import-position
from operator import attrgetter
from paraview.util.vtkAlgorithm import (
    smdomain,
    smhint,
    smproperty,
    smproxy,
    VTKPythonAlgorithmBase,
)
from vtk import (
    vtkCellArray,
    vtkDataArraySelection,
    vtkPoints,
    vtkPolyData,
    vtkVertex,
)
from vtkmodules.util import numpy_support

# from pytit.data import DataRank, Series, Storage, TimeStep
from pytit.data import Series, Storage, TimeStep


# pylint: disable=invalid-name,unused-argument
@smproxy.reader(
    name="TitDatabaseReader",
    label="Tit Database Reader",
    extensions="ttdb",
    file_description="Tit Database",
)
class TitDatabaseReader(VTKPythonAlgorithmBase):
    _file_path: str | None
    _storage: Storage | None
    _series: Series | None
    _time_steps: list[TimeStep] | None
    _array_selection: vtkDataArraySelection

    def __init__(self):
        super().__init__(nInputPorts=0, nOutputPorts=1)
        self._file_path = None
        self._storage = None
        self._series = None
        self._time_steps = None
        self._array_selection = vtkDataArraySelection()
        self._array_selection.AddObserver(
            "ModifiedEvent", lambda *_: self.Modified()
        )

    @property
    def storage(self) -> Storage:
        if self._storage is None:
            if not self._file_path or not os.path.exists(self._file_path):
                raise RuntimeError(f"File not found: {self._file_path}.")
            self._storage = Storage(self._file_path)
        return self._storage

    @property
    def series(self) -> Series:
        if self._series is None:
            self._series = self.storage.last_series
        return self._series

    @property
    def time_steps(self) -> list[TimeStep]:
        if self._time_steps is None:
            # self._time_steps = list(self.series.time_steps)
            self._time_steps = [
                self.series.last_time_step
            ] * self.series.num_time_steps
        return self._time_steps

    @smproperty.stringvector(name="FileName")
    @smdomain.filelist()
    @smhint.filechooser(extensions="ttdb", file_description="Tit Database")
    def SetFileName(self, file_path: str) -> None:
        if self._file_path == file_path:
            return
        self._file_path = file_path
        self.Modified()

    @smproperty.doublevector(
        name="TimestepValues",
        information_only="1",
        si_class="vtkSITimeStepsProperty",
    )
    def GetTimestepValues(self) -> list[float]:
        return list(map(attrgetter("time"), self.time_steps))

    @smproperty.dataarrayselection(name="Arrays")
    def GetDataArraySelection(self) -> vtkDataArraySelection:
        return self._array_selection

    def RequestInformation(self, request, inInfo, outInfo) -> bool:
        executive = self.GetExecutive()

        # Report the available time steps.
        info = outInfo.GetInformationObject(0)
        info.Remove(executive.TIME_RANGE())
        info.Append(executive.TIME_RANGE(), 0)
        info.Append(executive.TIME_RANGE(), len(self.time_steps) - 1)
        info.Remove(executive.TIME_STEPS())
        for ts in self.time_steps:
            info.Append(executive.TIME_STEPS(), ts.time)

        # Report the available arrays.
        # ParaView does not support matrix arrays, so we do not report them.
        # probe_time_step = self.time_steps[0]
        # for name, array in probe_time_step.varyings.arrays:
        #     if array.type.rank <= in (DataRank.SCALAR, DataRank.VECTOR):
        #         self._array_selection.AddArray(name)
        self._array_selection.AddArray("r")
        self._array_selection.AddArray("v")
        self._array_selection.AddArray("rho")

        return True

    def RequestData(self, request, inInfo, outInfo) -> bool:
        # Find the time step corresponding to the current time.
        time: float = outInfo.GetInformationObject(0).Get(
            self.GetExecutive().UPDATE_TIME_STEP()
        )
        time_step = next(
            (ts for ts in self.time_steps if ts.time == time), None
        )
        if time_step is None:
            return False

        # Extract the data arrays.
        arrays = {
            "r": time_step.varyings.find_array("r").data,
            "v": time_step.varyings.find_array("v").data,
            "rho": time_step.varyings.find_array("rho").data,
            # name: array.data
            # for name, array in time_step.varyings.arrays
            # if name == "r" or self._array_selection.ArrayIsEnabled(name)
        }
        points = arrays["r"]

        # Create VTK points array and a VTK "vertex" cell.
        # FIXME: Unhardcode the dimensionality of the points.
        vtk_vertex = vtkVertex()
        vtk_points = vtkPoints()
        for i, point in enumerate(points):
            vtk_vertex.GetPointIds().InsertNextId(i)
            vtk_points.InsertNextPoint(point[0], point[1], 0.0)

        vtk_vertices = vtkCellArray()
        vtk_vertices.InsertNextCell(vtk_vertex)
        vtk_poly_data = vtkPolyData()
        vtk_poly_data.SetPoints(vtk_points)
        vtk_poly_data.SetVerts(vtk_vertices)

        # Create VTK arrays for the selected arrays.
        for name, array in arrays.items():
            vtk_array = numpy_support.numpy_to_vtk(
                array,
                numpy_support.get_vtk_array_type(array.dtype),
            )
            vtk_array.SetName(name)
            vtk_poly_data.GetPointData().AddArray(vtk_array)

        vtkPolyData.GetData(outInfo, 0).ShallowCopy(vtk_poly_data)
        return True
