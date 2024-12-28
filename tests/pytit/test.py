from pytit.data import Storage

ds = Storage("input.ttdb")

series = ds.last_series
time_step = series.last_time_step
varying = time_step.varyings
r = varying.find_array("r").data
v = varying.find_array("v").data
rho = varying.find_array("rho").data

with open("./output.csv", "w") as csv_file:
    csv_file.write("r_x,r_y,v_x,v_y,rho\n")
    for i in range(len(r)):
        csv_file.write(f"{r[i,0]},{r[i,1]},{v[i,0]},{v[i,1]},{rho[i]}\n")
