import os
from titsdk import open_storage

# Open the storage and access the last frame.
storage = open_storage(
    os.path.join(os.environ["TEST_DATA_DIR"], "particles.ttdb"))

# Export the last series to an HDF5 file.
series = storage.last_series
series.export_hdf5("output.h5")
