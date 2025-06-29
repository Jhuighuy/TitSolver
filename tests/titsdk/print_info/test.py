import os
from titsdk import Storage, Series, TimeStep, Dataset, open_storage

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
  print(f"{spaces}Num. time steps:", series.num_time_steps)
  print(f"{spaces}Time steps:")
  spaces += "  "
  for i, time_step in enumerate(series.time_steps()):
    print(f"{spaces}Time step {i}:")
    print_time_step_info(time_step, spaces + "  ")

#
# Print the information about the time step.
#
def print_time_step_info(time_step: TimeStep, spaces: str = '') -> None:
  print(f"{spaces}Time:", time_step.time)
  print(f"{spaces}Uniforms:")
  print_dataset_info(time_step.uniforms, spaces + "  ")
  print(f"{spaces}Varyings:")
  print_dataset_info(time_step.varyings, spaces + "  ")

#
# Print the information about the dataset.
#
def print_dataset_info(dataset: Dataset, spaces: str = '') -> None:
  print(f"{spaces}Num. arrays:", dataset.num_arrays)
  print(f"{spaces}Arrays:")
  spaces += "  "
  for array in dataset.arrays():
    print(f"{spaces}{array.name}: {array.type}[{array.size}]")

# Open the storage and print its information.
print_storage_info(
    open_storage(os.path.join(os.environ["TEST_DATA_DIR"], "particles.ttdb")))
