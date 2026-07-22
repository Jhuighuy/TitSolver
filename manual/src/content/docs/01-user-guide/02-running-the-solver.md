---
title: Running the distributed solver
---

`titwcsph` is a hybrid MPI and TBB application. MPI ranks own spatial slabs of
the particle domain, while TBB evaluates rank-local work on CPU threads. The
same executable and numerical path are used for one-rank and multi-rank runs.

## Launching a run

Use the MPI launcher supplied with the MPI implementation that BlueTit was
built against:

```sh
mpiexec -n 4 titwcsph --output dam-break.tit-run
```

For a one-rank run, use `-n 1`; this also works with MPI implementations that
do not support singleton initialization without their launcher. Set
`TIT_NUM_THREADS` to limit TBB workers in each rank. For example, two MPI ranks
with four worker threads each can be launched with:

```sh
TIT_NUM_THREADS=4 mpiexec -n 2 titwcsph --output dam-break.tit-run
```

Do not assign more ranks times worker threads than the allocated CPU cores
unless oversubscription is intentional. All ranks must see the output directory
through the same filesystem path.

The solver accepts these options:

| Option                     | Default             | Meaning                                                                    |
| -------------------------- | ------------------- | -------------------------------------------------------------------------- |
| `--output PATH`            | `particles.tit-run` | New run directory to publish. An existing non-empty directory is rejected. |
| `--restart PATH`           | none                | Read the latest committed checkpoint from another run.                     |
| `--snapshot-every N`       | `100`               | Publish visualization fields every `N` completed steps.                    |
| `--checkpoint-every N`     | `1000`              | Publish restart state every `N` completed steps.                           |
| `--rebalance-every N`      | `100`               | Recompute distributed slab cuts every `N` completed steps.                 |
| `--particles-per-height N` | `80`                | Dam-break spatial resolution. This must match a restarted checkpoint.      |
| `--max-steps N`            | none                | Stop after `N` steps; useful for bounded runs and tests.                   |

The output and restart paths must differ. Normal progress is printed only by
rank zero. A fatal rank-local error terminates the complete communicator so
that peers cannot remain blocked in a collective operation.

## Run directory

Solver results are typed arrays, not database blobs:

```text
dam-break.tit-run/
├── manifest.json
├── index.json
├── run.xdmf
├── frames/
│   └── frame-00000100.h5
└── checkpoints/
    └── checkpoint-00001000.h5
```

Each HDF5 file is written collectively to a partial path, closed by every
rank, renamed, and only then added to `index.json`. Readers enumerate the
index, so an incomplete file is never a committed frame. Frame row order may
change with rank count; `/particles/id` is the stable identity. The XDMF file
is a regenerable compatibility view that can be opened in ParaView. The GUI
also reads only committed frames and can refresh while the solver continues.

Snapshots currently contain `id`, `kind`, position `r`, velocity `v`, and
density `rho`. Checkpoints additionally contain mass `m` and smoothing width
`h`. Checkpoint data is lossless persistent state; ghosts, adjacency, pressure,
and other derived fields are recomputed after restart.

## Restarting with a different rank count

Restart reads balanced contiguous HDF5 row ranges collectively and then
redistributes particles using the current decomposition. The new job does not
need the original number of ranks:

```sh
mpiexec -n 2 titwcsph \
  --restart dam-break.tit-run \
  --output resumed.tit-run
```

The latest committed checkpoint is selected. A new output directory is
required so the source run remains immutable and recoverable.

## Converting legacy `.ttdb` output

The solver no longer writes SQLite. During the compatibility window,
`titconvert` converts one legacy series to the run format:

```sh
titconvert particles.ttdb converted.tit-run
titconvert --series 0 particles.ttdb first-series.tit-run
```

Without `--series`, the latest series is selected. The converter streams each
decompressed field into its HDF5 dataset and preserves all legacy arrays.
Older files did not store stable IDs or particle kinds; when absent, IDs are
reconstructed from unchanged row order and kind is set to zero. Such converted
runs are suitable for visualization and analysis, but they do not contain the
complete persistent state required for restart.
