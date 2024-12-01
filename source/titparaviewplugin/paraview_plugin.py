import sys
import os

sys.path.append(os.path.dirname(__file__))

from paraview.util.vtkAlgorithm import *
import vtk
from titdata import DataStorage, DataSeriesView


@smproxy.reader(
    name="TitDatabaseReader",
    label="Tit Database Reader",
    extensions="ttdb",
    file_description="Tit Database",
)
class TitDatabaseReader(VTKPythonAlgorithmBase):
    """
    ParaView reader for Tit Database files.
    """

    def __init__(self):
        super().__init__(nInputPorts=0, nOutputPorts=1)
        self._file_path = None
        self._ds = None
        self._series = None

    @property
    def ds(self) -> DataStorage:
        if self._ds is None:
            assert self._file_path is not None
            self._ds = DataStorage(self._file_path)
        return self._ds

    @property
    def series(self) -> DataSeriesView:
        if self._series is None:
            assert self._file_path is not None
            self._series = self.ds.series[-1]
        return self._series

    @property
    def time_steps(self) -> list[float]:
        return [ts for ts in self.series.time_steps]

    @smproperty.stringvector(name="FileName")
    @smdomain.filelist()
    @smhint.filechooser(extensions="titcsv", file_description="Tit Database")
    def SetFileName(self, file_path):
        print(f"Set file: {file_path}")
        if self._file_path != file_path:
            self._file_path = file_path
            self.Modified()

    @smproperty.doublevector(
        name="TimestepValues", information_only="1", si_class="vtkSITimeStepsProperty"
    )
    def GetTimestepValues(self):
        return self.time_steps

    def RequestInformation(self, request, inInfoVec, outInfoVec):
        executive = self.GetExecutive()
        outInfo = outInfoVec.GetInformationObject(0)
        outInfo.Remove(executive.TIME_STEPS())
        outInfo.Remove(executive.TIME_RANGE())

        timesteps = self.time_steps
        if timesteps is not None:
            for t in range(len(timesteps)):
                outInfo.Append(executive.TIME_STEPS(), t)
            outInfo.Append(executive.TIME_RANGE(), 0)
            outInfo.Append(executive.TIME_RANGE(), len(timesteps) - 1)
        return 1

    def RequestData(self, request, in_info, out_info):
        if not self._file_path or not os.path.exists(self._file_path):
            raise RuntimeError(f"File not found: {self._file_path}")

        executive = self.GetExecutive()
        utime = out_info.GetInformationObject(0).Get(executive.UPDATE_TIME_STEP())
        tsindex = int(utime) if utime is not None else -1

        # Load data using the provided function
        poly_data = load_data(self._file_path, tsindex)

        # Get output vtkDataObject and copy the loaded data
        output = vtk.vtkPolyData.GetData(out_info, 0)
        output.ShallowCopy(poly_data)

        return 1


def load_data(file_path, tsindex=-1):
    """
    Reads the Tit Database and converts it into a vtkPolyData object.
    Uses the provided time step index.

    :param file_path: Path to the Tit Database file.
    :param tsindex: Time step index.
    :return: vtkPolyData object.
    """

    ds = DataStorage(file_path)

    series = ds.series[-1]
    time_step = series.time_steps[tsindex]
    varying = time_step.varyings
    arrays = varying.arrays
    array_names = arrays.keys()

    # Create vtk arrays to store particles.
    vtk_points = vtk.vtkPoints()
    vtk_arrays = {name: vtk.vtkFloatArray() for name in array_names}
    for name, array in vtk_arrays.items():
        array.SetName(name)
        array.SetNumberOfComponents(arrays[name].dtype.dim)

    blobs = {name: array.read_data() for name, array in arrays.items()}
    r = blobs["r"]

    for i in range(len(r)):
        values = {name: blob[i] for name, blob in blobs.items()}
        ri = values["r"]
        vtk_points.InsertNextPoint(ri[0], ri[1], 0.0)
        for name, value in values.items():
            if isinstance(value, (int, float)):
                vtk_arrays[name].InsertNextValue(value)
            else:
                vtk_arrays[name].InsertNextTuple(value)

    poly_data = vtk.vtkPolyData()
    poly_data.SetPoints(vtk_points)
    for array in vtk_arrays.values():
        poly_data.GetPointData().AddArray(array)

    vtk_vertex = vtk.vtkVertex()
    for i in range(vtk_points.GetNumberOfPoints()):
        vtk_vertex.GetPointIds().InsertNextId(i)

    vtk_cells = vtk.vtkCellArray()
    vtk_cells.InsertNextCell(vtk_vertex)
    poly_data.SetVerts(vtk_cells)

    return poly_data
