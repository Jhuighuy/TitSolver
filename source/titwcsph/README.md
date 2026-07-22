# `titwcsph`

This executable contains the hybrid MPI + TBB weakly compressible SPH solver.

Launch a distributed run with, for example:

```sh
TIT_NUM_THREADS=4 mpiexec -n 2 titwcsph --output dam-break.tit-run
```

Results and portable checkpoints are published as immutable HDF5 files in a
versioned `.tit-run` directory. See the user manual's “Running the distributed
solver” page for launch, output, restart, and legacy conversion details.
