# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, under the MIT License.
# See /LICENSE.md for license information. SPDX-License-Identifier: MIT
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# type: ignore -- ParaView and VTK are not typed for the most part.
# pylint: disable=invalid-name,missing-docstring,no-name-in-module,unused-argument,wrong-import-position
import os
from collections.abc import Iterator  # noqa: E402
from operator import attrgetter
import sys
from paraview.util.vtkAlgorithm import smdomain, smhint, smproperty, smproxy
from vtkmodules.vtkCommonCore import vtkDataArraySelection, vtkPoints
from vtkmodules.vtkCommonDataModel import vtkCellArray, vtkPolyData, vtkVertex
from vtkmodules.util import numpy_support
from vtkmodules.util.vtkAlgorithm import VTKPythonAlgorithmBase

sys.path.append(os.path.join(os.path.dirname(__file__)))
from ttdb import Array, Frame, open_storage, Rank, Storage  # noqa: E402


# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


@smproxy.reader(
    name="TTDBReader",
    label="BlueTit Database Reader",
    extensions="ttdb",
    file_description="BlueTit Database file.",
)
class TTDBReader(VTKPythonAlgorithmBase):

    __file_path: str | None
    __storage: Storage | None
    __array_selection: vtkDataArraySelection

    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    def __init__(self) -> None:
        super().__init__(nInputPorts=0, nOutputPorts=1)
        self.__file_path = None
        self.__storage = None
        self.__array_selection = vtkDataArraySelection()
        self.__array_selection.AddObserver(
            "ModifiedEvent",
            lambda *_: self.Modified(),
        )

    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    def __iter_frames(self) -> Iterator[Frame]:
        if self.__storage is None:
            assert self.__file_path is not None
            self.__storage = open_storage(self.__file_path)

        return self.__storage.last_series.frames()

    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    def __find_frame(self, time: float) -> Frame | None:
        return next((f for f in self.__iter_frames() if f.time == time), None)

    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    def __is_array_enabled(self, array: Array) -> bool:
        name = array.name
        return name == "r" or self.__array_selection.ArrayIsEnabled(name)

    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    @smproperty.stringvector(name="FileName")
    @smdomain.filelist()
    @smhint.filechooser(
        extensions="ttdb",
        file_description="BlueTit Database file.",
    )
    def SetFileName(self, file_path: str) -> None:
        if self.__file_path != file_path:
            self.__file_path = file_path
            self.__storage = None
            self.Modified()

    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    @smproperty.doublevector(
        name="TimestepValues",
        information_only="1",
        si_class="vtkSITimeStepsProperty",
    )
    def GetTimestepValues(self) -> list[float]:
        return list(map(attrgetter("time"), self.__iter_frames()))

    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    @smproperty.dataarrayselection(name="Arrays")
    def GetDataArraySelection(self) -> vtkDataArraySelection:
        return self.__array_selection

    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    def RequestInformation(self, request, inInfo, outInfo) -> bool:
        executive = self.GetExecutive()
        info = outInfo.GetInformationObject(0)

        # Report the available time steps.
        num_frames = 0
        info.Remove(executive.TIME_STEPS())
        for frame in self.__iter_frames():
            num_frames += 1
            info.Append(executive.TIME_STEPS(), frame.time)

        # Report the available time range.
        info.Remove(executive.TIME_RANGE())
        info.Append(executive.TIME_RANGE(), 0)
        info.Append(executive.TIME_RANGE(), num_frames)

        # Report the available arrays.
        # ParaView does not support matrix arrays, so we do not report them.
        if (probe_frame := next(self.__iter_frames(), None)) is not None:
            for array in probe_frame.arrays():
                if array.type.rank in (Rank.scalar, Rank.vector):
                    self.__array_selection.AddArray(array.name)

        return True

    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    def RequestData(self, request, inInfo, outInfo) -> bool:
        executive = self.GetExecutive()
        info = outInfo.GetInformationObject(0)

        # Find the time step corresponding to the current time.
        frame = self.__find_frame(info.Get(executive.UPDATE_TIME_STEP()))
        if frame is None:
            return False

        # Extract the data arrays.
        arrays = {
            array.name: array.data
            for array in frame.arrays()
            if self.__is_array_enabled(array)
        }

        # Create VTK points array and a VTK "vertex" cell.
        vtk_vertex = vtkVertex()
        vtk_points = vtkPoints()
        for i, point in enumerate(arrays["r"]):
            vtk_vertex.GetPointIds().InsertNextId(i)
            vtk_points.InsertNextPoint(*(*point, 0.0, 0.0)[:3])
        vtk_vertices = vtkCellArray()
        vtk_vertices.InsertNextCell(vtk_vertex)

        # Create VTK arrays for the selected arrays.
        vtk_poly_data = vtkPolyData()
        vtk_poly_data.SetPoints(vtk_points)
        vtk_poly_data.SetVerts(vtk_vertices)
        for name, array in arrays.items():
            vtk_array = numpy_support.numpy_to_vtk(
                array,
                numpy_support.get_vtk_array_type(array.dtype),
            )
            vtk_array.SetName(name)
            vtk_poly_data.GetPointData().AddArray(vtk_array)

        # Copy the VTK data to the output.
        vtkPolyData.GetData(outInfo, 0).ShallowCopy(vtk_poly_data)

        return True

    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
