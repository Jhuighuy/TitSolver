from pytit.data import Storage

ds = Storage("input.ttdb")
print(ds)
print(ds.path)
print(ds.max_series, flush=True)

series = ds.last_series
print(series, flush=True)
del ds

time_step = series.last_time_step
print(time_step, flush=True)
del series

varying = time_step.varyings
print(varying)
print(varying.num_arrays, flush=True)
del time_step

r = varying.find_array("r")
v = varying.find_array("v")
rho = varying.find_array("rho")
print(r, v, rho, flush=True)
print(r.data, flush=True)
print(v.data, flush=True)
del varying

r = r.data
v = v.data
rho = rho.data
print(r, flush=True)
print(v, flush=True)
print(rho, flush=True)
with open("./output.csv", "w") as csv_file:
    csv_file.write("r_x,r_y,v_x,v_y,rho\n")
    for i in range(len(r)):
        csv_file.write(f"{r[i,0]},{r[i,1]},{v[i,0]},{v[i,1]},{rho[i]}\n")
