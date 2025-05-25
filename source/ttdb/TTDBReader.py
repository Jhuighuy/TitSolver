# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
# Commercial use, including SaaS, requires a separate license, see /LICENSE.md
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# pylint: disable=import-error,no-name-in-module,invalid-name,unused-argument
# pylint: disable=wrong-import-position
from collections.abc import Iterator
from operator import attrgetter
import os
import sys
from paraview.util.vtkAlgorithm import (  # type: ignore
    smdomain, smhint, smproperty, smproxy, VTKPythonAlgorithmBase,
)
from vtk import (  # type: ignore
    vtkCellArray, vtkDataArraySelection, vtkPoints, vtkPolyData, vtkVertex,
)
from vtkmodules.util import numpy_support

# ParaView does not know how to import `ttdb.py`.
sys.path.append(os.path.dirname(__file__))
from ttdb import Rank, Storage, TimeStep  # noqa: E402

__all__ = ("TTDBReader",)

READER_LABEL = "BlueTit Database Reader"
FILE_DESCRIPTION = "BlueTit Database"
FILE_EXTENSIONS = "ttdb"

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

@smproxy.reader(
    name="TTDBReader",
    label=READER_LABEL,
    extensions=FILE_EXTENSIONS,
    file_description=FILE_DESCRIPTION,
)
class TTDBReader(VTKPythonAlgorithmBase):
  _file_path: str | None
  _storage: Storage | None
  _array_selection: vtkDataArraySelection

  def __init__(self) -> None:
    super().__init__(nInputPorts=0, nOutputPorts=1)
    self._file_path = None
    self._storage = None
    self._series = None
    self._array_selection = vtkDataArraySelection()
    self._array_selection.AddObserver("ModifiedEvent",
                                      lambda *_: self.Modified())

  @property
  def storage(self) -> Storage:
    if self._storage is None:
      if not self._file_path or not os.path.exists(self._file_path):
        raise RuntimeError(f"File not found: {self._file_path}.")
      self._storage = Storage(self._file_path)
    return self._storage

  def time_steps(self) -> Iterator[TimeStep]:
    # Do not cache the last series and time steps, as they may change.
    return self.storage.last_series.time_steps()

  @smproperty.stringvector(name="FileName")
  @smdomain.filelist()
  @smhint.filechooser(extensions=FILE_EXTENSIONS,
                      file_description=FILE_DESCRIPTION)
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
    return list(map(attrgetter("time"), self.time_steps()))

  @smproperty.dataarrayselection(name="Arrays")
  def GetDataArraySelection(self) -> vtkDataArraySelection:
    return self._array_selection

  def RequestInformation(self, request, inInfo, outInfo) -> bool:
    executive = self.GetExecutive()

    # Report the available time steps.
    info = outInfo.GetInformationObject(0)
    info.Remove(executive.TIME_STEPS())
    num_time_steps = 0
    for ts in self.time_steps():
      num_time_steps += 1
      info.Append(executive.TIME_STEPS(), ts.time)
    info.Remove(executive.TIME_RANGE())
    info.Append(executive.TIME_RANGE(), 0)
    info.Append(executive.TIME_RANGE(), num_time_steps)

    # Report the available arrays.
    # ParaView does not support matrix arrays, so we do not report them.
    if (probe_time_step := next(self.time_steps(), None)) is not None:
      for array in probe_time_step.varyings.arrays():
        if array.type.rank in (Rank.scalar, Rank.vector):
          self._array_selection.AddArray(array.name)

    return True

  def RequestData(self, request, inInfo, outInfo) -> bool:
    # Find the time step corresponding to the current time.
    time: float = outInfo.GetInformationObject(0).Get(
        self.GetExecutive().UPDATE_TIME_STEP())
    time_step = next((ts for ts in self.time_steps() if ts.time == time), None)
    if time_step is None:
      return False

    # Extract the data arrays.
    arrays = {
        array.name: array.data
        for array in time_step.varyings.arrays()
        if array.name == "r" or self._array_selection.ArrayIsEnabled(array.name)
    }
    points = arrays["r"]

    # Create VTK points array and a VTK "vertex" cell.
    vtk_vertex = vtkVertex()
    vtk_points = vtkPoints()
    for i, point in enumerate(points):
      xyz = (*point, 0.0, 0.0)[:3]  # must always be 3D.
      vtk_vertex.GetPointIds().InsertNextId(i)
      vtk_points.InsertNextPoint(*xyz)

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

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
