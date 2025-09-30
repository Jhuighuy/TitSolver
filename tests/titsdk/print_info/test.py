import os
from titsdk import Storage, Series, Frame, open_storage

#
# Print the information about the storage.
#
def print_storage_info(storage: Storage, spaces: str = '') -> None:
  print(f"{spaces}Num. series:", storage.num_series)
  print(f"{spaces}Series:")
  spaces += "  "
  for i, series in enumerate(storage.series()):
    print(f"{spaces}Series {i}:")
    print_series_info(series, spaces)

#
# Print the information about the series.
#
def print_series_info(series: Series, spaces: str = '') -> None:
  print(f"{spaces}Num. frames:", series.num_frames)
  print(f"{spaces}Frames:")
  spaces += "  "
  for i, frame in enumerate(series.frames()):
    print(f"{spaces}Frame {i}:")
    print_frame_info(frame, spaces + "  ")

#
# Print the information about the frame.
#
def print_frame_info(frame: Frame, spaces: str = '') -> None:
  print(f"{spaces}Time:", frame.time)
  print(f"{spaces}Num. arrays:", frame.num_arrays)
  print(f"{spaces}Arrays:")
  spaces += "  "
  for array in frame.arrays():
    print(f"{spaces}{array.name}: {array.type}[{array.size}]")

# Open the storage and print its information.
print_storage_info(
    open_storage(os.path.join(os.environ["TEST_DATA_DIR"], "particles.ttdb")))
