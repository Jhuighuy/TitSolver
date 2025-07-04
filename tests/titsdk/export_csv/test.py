import os
from titsdk import open_storage

# Open the storage and access the varying dataset of the last time step.
storage = open_storage(
    os.path.join(os.environ["TEST_DATA_DIR"], "particles.ttdb"))
series = storage.last_series
time_step = series.last_time_step
varying = time_step.varyings

# Locate the positions, velocities, and densities arrays.
r_array = varying.find_array("r")
assert r_array is not None

v_array = varying.find_array("v")
assert v_array is not None

rho_array = varying.find_array("rho")
assert rho_array is not None

# Write the data to a CSV file.
r = r_array.data
v = v_array.data
rho = rho_array.data
with open("output.csv", "w", encoding="utf-8") as csv_file:
  csv_file.write("r_x,r_y,v_x,v_y,rho\n")
  for i in range(len(r)):
    csv_file.write(f"{r[i, 0]},{r[i, 1]},{v[i, 0]},{v[i, 1]},{rho[i]}\n")
