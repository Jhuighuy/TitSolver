# MPI Migration Plan

This document describes the architecture, design changes, and roadmap for
migrating BlueTit Solver from single-process thread-based parallelism (TBB) to
hybrid distributed parallelism (MPI across ranks + TBB within each rank).

The goal is *hybrid* parallelism, not a replacement: TBB remains the
intra-rank engine, MPI adds inter-node scaling. One MPI rank per NUMA domain
(or per node), TBB threads inside — this is the standard and most efficient
configuration for particle codes.

## 1. Where we are today

### 1.1 Parallelism

All parallelism lives in `tit::par` (`source/tit/par/`):

- `par::for_each` / `par::fold` / `par::unstable_copy_if` — TBB-backed data
  parallelism over ranges.
- `par::block_for_each` — race-free symmetric pair updates (`a += …`,
  `b -= …`) over pair blocks produced by `ParticleMesh`.
- `par::TaskGroup` — recursive task parallelism (used by
  `geom::RecursiveBisection`).
- `par::init()` / `num_threads()` — global thread-count control.

### 1.2 Data model

`sph::ParticleArray` (`source/tit/sph/particle_array.hpp`) is a
structure-of-arrays container with compile-time field sets (uniform +
varying), where particles are grouped by `ParticleType` (`fluid`, then
`fixed`). Fields are plain `std::vector<field_value_t<...>>` columns — each
field of each particle subset is a contiguous span. This is *excellent* news
for MPI: packing a subset of particles for communication is a generic,
type-driven gather over columns, and the field-set machinery
(`TypeSet::for_each`, `field_name`) lets us write one halo-exchange routine
that works for any field subset.

### 1.3 Neighbor search and thread partitioning

`sph::ParticleMesh` (`source/tit/sph/particle_mesh.hpp`) couples three
concerns:

1. Neighbor search over *all* particles (`geom::GridSearch` /
   `geom::KDTreeSearch`) and boundary-face search (`geom::GridFaceSearch`).
2. Multi-level *thread* partitioning of the particle set
   (`geom::RecursiveInertialBisection` for the bulk,
   `geom::PixelatedPartition` + `geom::KMeansClustering` for interfaces) used
   to build `block_pairs` so symmetric pair updates are race-free.
3. Storage of adjacency in global particle indices.

The key insight: **the multi-level partitioning scheme already implemented for
threads is exactly the scheme needed for ranks.** Adding a rank level "above"
the thread levels is a natural extension, and the `geom::partition` module
(RIB, SFC-sort partition, k-means) is directly reusable for domain
decomposition.

### 1.4 Solver loop

`FluidEquations` + integrators (`SymplecticEuler`, `VelocityVerlet`, `SSPRK`)
follow a phase structure per (sub)step:

1. `prepare()` — mesh update (search + partition), `compute_gamma` (local,
   face-based), `setup_boundary` (wall particles read fluid neighbors).
2. `compute_time_step()` — global min-reduction.
3. `compute_continuity` / `compute_momentum` — per-particle loops + symmetric
   pair loops + boundary-face loops.
4. Integration updates (`r`, `v`, `rho` on fluid).
5. `post_integrate()` — `apply_shifts` (multi-pass free-surface
   classification with neighbor reads of `N` and `phi`),
   `apply_free_surface_correction` (neighbor reads of `rho_raw`).

### 1.5 Everything else

- I/O: `data::Storage` — SQLite (`.ttdb`), whole-array frame writes. HDF5
  support exists in `tit/data` but is not the primary path.
- Entry point: `TIT_IMPLEMENT_MAIN` → `run_main` → `par::init()`.
- Serialization: `tit/core/serialization.hpp` — byte-level, stream-based.
- Tests: doctest units + `add_tit_test` behavioral tests with output/checksum
  matching, driven by `./build/build.sh --run`.
- Dependencies: vcpkg manifest; no MPI today.

### 1.6 MPI-relevant pain points

- `ParticleMesh` mixes search, thread partitioning, and adjacency — the rank
  dimension has no place to live.
- `ParticleArray::append` inserts into the middle of every column (O(n) per
  append) and particle identity is positional — there are no stable particle
  IDs, which migration (and debugging distributed runs) requires.
- Face adjacency resolves wall vertices as `particles.fixed()[vert_indices]`
  — global vertex indices index directly into the fixed-particle range. Any
  decomposition of fixed particles breaks this without an index map.
- `SSPRKIntegrator::step` snapshots the whole array and recombines by index
  (`lincomb_`) — particle count and order must not change *within* a step.
- Reductions (`compute_time_step`), logging, profiling, and storage writes
  all assume a single process.

## 2. Target architecture

```
             ┌────────────────────────────────────────────────┐
             │                  titwcsph (main)               │
             │        runtime::init → mpi + tbb + logging     │
             └────────────────────────────────────────────────┘
                                     │
      ┌──────────────┬───────────────┼────────────────┬─────────────┐
      │   tit::sph   │   tit::dist   │    tit::par    │  tit::mpi   │
      │  equations,  │ decomposition,│  TBB in-rank   │ RAII MPI    │
      │ integrators, │ halo exchange,│  parallelism   │ wrappers    │
      │  particles   │   migration   │  (unchanged)   │             │
      └──────────────┴───────────────┴────────────────┴─────────────┘
                            │                 reuses
                            └── tit::geom (partition, search, sort)
```

Two new libraries:

- **`tit/mpi`** — a thin, RAII, type-safe C++ wrapper over MPI (in the same
  spirit as `tit/data/sqlite.hpp` wraps SQLite). No SPH knowledge. Contents:
  `mpi::Comm`, scoped init/finalize, datatype mapping for `Vec`/`Mat`/scalars,
  `all_reduce`, `all_to_all_v`, non-blocking send/recv with request handles.
  MPI is a **mandatory core dependency**, on the same footing as TBB — there
  is no build without it and no stub layer. Desktop and GUI workflows are
  unaffected: MPI's *singleton initialization* means the solver binary can be
  launched directly (no `mpiexec`) and comes up as a 1-rank world, where halo
  plans are empty and collectives degenerate to no-ops at runtime.

- **`tit/dist`** — distributed-particle infrastructure, SPH-aware but
  equation-agnostic:
  - `dist::Decomposition` — assigns owned subdomains to ranks using the
    existing `geom::partition_func`s (Hilbert-sort partition recommended as
    the default: cheap, incremental, deterministic; RIB as an alternative).
  - `dist::HaloExchange` — builds and caches the ghost communication plan
    (which local particles are ghosts on which neighbor rank and vice versa)
    and refreshes arbitrary field subsets:
    `halo.refresh(particles, TypeSet{r, v, rho})`.
  - `dist::Migrator` — moves particle ownership between ranks after
    re-decomposition, using stable global IDs.
  - Global reductions: `dist::all_reduce_min(...)` etc., delegating to
    `tit::mpi`.

### 2.1 Ownership model: owned + ghost particles

Each rank owns the particles inside its subdomain and mirrors **ghost
copies** of remote particles within the *halo width* of its subdomain
boundary:

```
halo width = max over particles of search radius (kernel_.radius)
           + skin margin δ
```

The skin margin lets the ghost *membership plan* stay valid for several steps
even as particles move (CFL bounds displacement to ~0.4·h per step, so a skin
of ~1·h buys several steps). Between plan rebuilds, only field *values* are
refreshed — a cheap neighbor-to-neighbor `Isend/Irecv` with precomputed index
lists. The plan (and the decomposition) is rebuilt when accumulated maximum
displacement exceeds δ/2, LAMMPS-style.

`ParticleArray` layout gains a ghost dimension. Particle ranges become a 2×2
grid ordered as:

```
[ owned fluid | owned fixed | ghost fluid | ghost fixed ]
```

- `particles.fluid()` / `.fixed()` keep their meaning for *owned* particles —
  the equations' write loops (`par::for_each(particles.fluid(), …)`) keep
  working unmodified and never write to ghosts.
- `particles.all()` becomes "owned + ghost" — read loops and neighbor
  searches see everything, which is what they need.
- A new `varying` field `gid` (global particle ID, `std::uint64_t`) provides
  stable identity for migration, deterministic output ordering, and
  debugging.

### 2.2 Cross-rank pair interactions: symmetric-ghost ("Newton off across ranks")

The pair loops update both `a` and `b`. Across ranks, the simplest correct
scheme — and the recommended starting point — is:

- Ghost exchange is symmetric: if rank P holds a ghost of Q's particle *b*
  near owned particle *a*, then Q holds a ghost of *a* near *b*.
- Each rank builds pair blocks only for pairs with **at least one owned
  particle**; for owned–ghost pairs, updates are applied to the owned side
  only (writes to ghost fields are simply discarded — the mirrored pair on
  the neighbor rank produces them there).
- Owned–owned pairs behave exactly as today.

Cross-boundary pair kernels are evaluated twice (once per rank), but only for
the thin halo layer — a small fraction of total pairs for sensible subdomain
sizes. The alternative (compute once + reverse-scatter accumulation, "Newton
on") halves that cost at the price of an extra communication phase per pair
loop; it is listed as a Phase-5 optimization, not a starting requirement.

Implementation-wise this drops out of the existing multi-level block
machinery almost for free: the block builder already assigns each pair to the
finest level where both endpoints share a part. Pairs whose endpoints share
no part on any level with an *owned* endpoint go into the owned side's block;
ghost–ghost pairs are skipped.

### 2.3 Communication schedule per SSPRK substep

Per-particle EOS-derived fields (`p`, `cs`) are *recomputed locally on ghosts*
rather than communicated — cheaper than a message. The state that must
actually move over the wire per substep:

| Phase | Halo refresh before it | Notes |
| --- | --- | --- |
| `prepare()` (search + gamma + boundary) | `r`, `v`, `rho` | one combined message; positions changed in previous substep |
| `compute_time_step()` | — | local `par::fold` + `all_reduce_min` |
| `compute_continuity` / `compute_momentum` | — | `p`, `cs` recomputed on ghosts locally |
| integration update | — | owned-only writes |
| `apply_shifts`: N/L/gradients pair loop | — | ghost contributions mirrored remotely |
| `apply_shifts`: free-surface visibility pass | `N` | reads neighbors' `N` |
| `apply_shifts`: near/far classification | `phi` | reads neighbors' `phi` after FS pass |
| `apply_free_surface_correction` | `rho`, `phi` | `rho_raw` set locally on ghosts from refreshed `rho` |

That is ~4 small neighbor-to-neighbor exchanges per substep plus one global
scalar reduction per step — a typical SPH communication profile that scales
well.

`apply_shifts` moves particles (`r[a] += dr[a]`) *after* the mesh update; the
skin margin absorbs this, same as it absorbs integration motion.

### 2.4 Boundary geometry and fixed particles

The boundary `geom::Surface` (and the containment/winding function) is
**replicated on every rank** — it is static, compact relative to the particle
data, and replication keeps `compute_gamma` and face search purely local.

Fixed (wall) particles are *decomposed* like fluid particles (they carry
per-step state: `rho` via Neumann extrapolation). To keep
`mesh[domain_, a]`'s global-vertex-index resolution working, each rank
maintains a `global vertex index → local particle index` map, built once per
migration. Faces whose support vertices are not all present locally cannot be
adjacent to local particles if the halo width is respected — an assertion
will guard this invariant.

### 2.5 Time stepping and migration cadence

Strict separation of three frequencies:

1. **Every substep** — halo *value* refreshes + local neighbor-list rebuild
   (what `mesh.update` already does, now over owned+ghost particles).
2. **Every ~5–20 steps (displacement-triggered)** — ghost *plan* rebuild:
   re-run decomposition, migrate ownership, rebuild vertex maps, reset skin
   accumulator. Always at a *step* boundary, never inside `SSPRKIntegrator`
   (its snapshot/`lincomb_` requires stable local indexing within a step).
3. **Every frame** — output.

### 2.6 I/O, logging, profiling

- **Output (Phase 2):** gather varying fields to rank 0 (ordered by `gid`)
  and write through the existing SQLite `data::Storage` unchanged.
  Deterministic ordering by `gid` keeps `.ttdb` checksums meaningful.
- **Output (later):** parallel HDF5 via the already-vendored HighFive/HDF5 as
  a second `data` backend for large runs; SQLite stays the GUI-facing format.
- **Checkpoint/restart** falls out of the same field-pack machinery and
  becomes worthwhile the moment multi-node runs are long enough to fail.
- **Logging:** `log()` gains rank awareness — rank 0 prints by default,
  `log_all()` for per-rank diagnostics.
- **Profiler:** per-rank profiles written to per-rank files
  (`output/profile.<rank>`), plus a rank-0 aggregated summary (min/max/imbalance
  per section) — load imbalance is *the* number to watch in this migration.

## 3. Design changes by module

### `tit/core`

- `run_main` / new `runtime::init`: single init path that (in order)
  initializes MPI (`MPI_THREAD_FUNNELED` is sufficient — all MPI calls happen
  outside TBB parallel regions), then TBB, then logging/profiling with rank
  context. Exceptions/aborts must call `MPI_Abort` so a crashed rank kills
  the job instead of deadlocking it.
- `serialization.hpp` is stream-oriented; halo/migration packing will use the
  simpler `to_byte_array`-style trivially-copyable path over field columns
  (all field values are `Vec`/`Mat`/scalars — trivially copyable by
  construction).

### `tit/par`

Unchanged in role (intra-rank only). `par::init` folds into
`runtime::init`. Nothing in `par` learns about MPI.

### `tit/mpi` (new)

RAII wrappers as described in §2. Unit-tested standalone. ~1–2 KLOC.

### `tit/geom`

- `partition_func`s gain nothing MPI-specific; `dist::Decomposition` calls
  them. Phase 2 computes the rank-level partition redundantly on every rank
  from gathered positions (`all_gather` of `r` is acceptable up to ~10⁷
  particles); Phase 5 replaces this with a distributed Hilbert-key
  sort-partition (keys are computed locally; only the splitters are agreed
  globally — a small `all_reduce`-style exchange).
- `sort/hilbert_curve_sort` gains a "key extraction" entry point (currently
  sorting-only) so the distributed splitter logic can reuse it.

### `tit/sph`

- **`ParticleArray`**: owned/ghost ranges (§2.1), `gid` field, bulk
  `append_n`/`resize_ghosts` (fixing the O(n) middle-insert `append` along
  the way), generic `pack(indices, fields) → bytes` /
  `unpack(bytes, fields, offset)` built on the field-set machinery. This is
  the single most load-bearing refactor and is useful even without MPI
  (storage writes, future GPU transfers).
- **`ParticleMesh`**: splits into
  - neighbor/face search + adjacency (unchanged logic, now spanning
    owned+ghost particles), and
  - pair-block building, which learns the rule from §2.2 (skip ghost–ghost,
    owned side keeps owned–ghost pairs).
  The rank-level partition is *not* computed here — it is provided by
  `dist::Decomposition`; the existing thread-level multi-level partitioning
  runs unchanged within the rank's owned set.
- **`FluidEquations`**: phases stay intact; the integrators (or a thin
  `dist::Stepper` wrapper) insert the halo refreshes from §2.3 between
  phases. `compute_time_step` reduces via `dist::all_reduce_min`. The clean
  way to keep `sph` decoupled from `dist`: equations declare, per phase,
  which fields they *read from neighbors* (a `TypeSet` next to
  `required_fields`/`modified_fields`) and the stepper turns that into
  refreshes. That declaration is also self-documenting and assertable in
  serial builds.
- **Integrators**: `SSPRKIntegrator`'s snapshot copies ghosts too (correct
  and simple); `lincomb_` touches owned fluid only, as today.

### `tit/data`

- Phase 2: no changes (rank-0 writes).
- Phase 4: `data::Storage` HDF5 parallel backend for large runs;
  checkpoint/restart record = decomposition-independent (`gid`-keyed) field
  dump, so restarts may change rank count.

### Build system, CI, tests

- vcpkg: add `mpi` (resolves to OpenMPI on macOS/Linux) as an unconditional
  dependency in the manifest; `find_package(MPI REQUIRED)` at the top level,
  next to TBB. On clusters, the vendor/system MPI takes precedence over the
  vcpkg port (standard `find_package(MPI)` resolution; the vcpkg port is the
  developer-machine fallback).
- Test driver: `add_tit_test(... MPI_RANKS 4)` prefixes the command with
  `mpiexec -n 4` and captures per-rank stdout. Unit tests of `tit/mpi` and
  `tit/dist` run at 1, 2, and 4 ranks.
- Behavioral acceptance: `titwcsph/dam_breaking` at `-n 1` must match the
  pre-migration checksum bit-for-bit — the 1-rank path takes no
  communication-dependent branches, so this is achievable and enforced.
  Multi-rank runs cannot be bitwise-identical
  (floating-point summation order changes with the pair-block structure), so
  multi-rank tests compare physical observables (wave-front position, total
  mass/momentum, pressure probes) against the serial run within tolerances —
  this needs a small tolerance-comparison mode in the test driver alongside
  the existing checksum matching.

## 4. Roadmap

Phases are ordered so that every phase lands green on `main` and the serial
path never regresses. Estimates assume one focused developer.

### Phase 0 — Preparatory refactors (no MPI yet) — ~2 weeks

1. `ParticleArray`: owned/ghost ranges (ghost ranges empty for now), `gid`
   field, bulk append, generic field pack/unpack. Fix `append` complexity.
2. Split `ParticleMesh` into search/adjacency vs. pair-block building;
   the rank partition slot exists but holds the trivial 1-rank value.
3. `runtime::init` consolidation (absorbs `par::init`).
4. Per-phase neighbor-read field declarations in `FluidEquations`
   (asserted, unused otherwise).

*Exit criteria:* all existing tests bitwise-green; no measurable serial
performance regression.

### Phase 1 — MPI foundation — ~2 weeks

1. `tit/mpi` wrapper + vcpkg/CMake/CI wiring — MPI becomes a required
   top-level dependency alongside TBB.
2. Test-driver `MPI_RANKS` support; `tit/mpi` unit tests at 1/2/4 ranks.
3. Rank-aware logging and per-rank profiler output.

*Exit criteria:* `titwcsph` runs both directly (singleton init) and under
`mpiexec -n 1` with output bitwise-identical to the pre-migration baseline;
CI exercises 2-rank `tit/mpi` unit tests.

### Phase 2 — Static decomposition + halo exchange — ~4–6 weeks (the core)

1. `dist::Decomposition` (Hilbert sort-partition, computed redundantly from
   gathered positions) + fixed-particle vertex maps.
2. `dist::HaloExchange`: plan build + field-subset refresh.
3. Pair-block rule from §2.2; integrator-level refresh schedule from §2.3.
4. Rank-0 gather output ordered by `gid`.
5. Tolerance-comparison mode in the test driver; multi-rank dam-break
   acceptance test.

Decomposition is *recomputed and ownership fully migrated every step* in this
phase (correct but slow) — cadence optimization is Phase 3. Skin margin
already in place so ghost plans survive the intra-step motion.

*Exit criteria:* dam-break at 2/4/8 ranks matches serial observables within
tolerances; 1-rank bitwise identity holds.

### Phase 3 — Migration cadence + dynamic load balancing — ~3 weeks

1. Displacement-triggered plan/decomposition rebuild (skin accounting).
2. Incremental migration (only ownership deltas move).
3. Imbalance metrics in the profiler summary; rebalance trigger on measured
   imbalance, not just displacement.

*Exit criteria:* ≥70 % parallel efficiency on 8 ranks for a 3D dam break
sized ~10⁶ particles; communication time visible and bounded in profiles.

### Phase 4 — Distributed I/O and restart — ~2–3 weeks

1. Parallel HDF5 output backend; SQLite remains default for GUI-scale runs.
2. `gid`-keyed checkpoint/restart, rank-count independent.
3. GUI/titback: runs launched through `mpiexec` (launcher config surface);
   the GUI continues reading `.ttdb`/HDF5 artifacts — no live-socket changes.

### Phase 5 — Performance hardening — ongoing

1. Overlap halo refresh with interior computation (non-blocking exchange;
   compute interior pair blocks while boundary-layer data is in flight — the
   block structure already separates interface particles, which is precisely
   the split needed).
2. Distributed Hilbert splitter selection (drop the gather in
   `dist::Decomposition`).
3. Optional "Newton on" reverse-scatter across ranks if halo-pair
   double-compute shows up in profiles.
4. Message coalescing (single buffer per neighbor per refresh), datatype
   tuning, `MPI_THREAD_MULTIPLE` experiments only if profiles demand it.

## 5. Risks and open questions

- **Floating-point reproducibility.** Multi-rank results differ from serial
  in the last bits by construction. The tolerance-based acceptance testing in
  Phase 2 is the mitigation; if stronger guarantees are ever needed,
  deterministic (sorted-order) accumulation per particle is possible at a
  cost.
- **Free-surface classification across ranks.** The `phi` state machine in
  `apply_shifts` relies on bit-pattern sentinels and benign races within
  shared memory; across ranks the extra `phi`/`N` refreshes (§2.3) make the
  passes well-defined, but this is the most subtle correctness area and needs
  a dedicated multi-rank test with a fragmenting free surface (splash case).
- **Small-run overhead.** Below ~10⁵ particles per rank, halo overhead
  dominates. Single-rank runs stay on today's fast path — empty halo plans,
  no-op collectives, and negligible `MPI_Init` cost — so small/GUI runs pay
  essentially nothing for the distributed machinery.
- **Boundary geometry scale.** Replicating `geom::Surface` is fine for
  current cases; a future case with tens of millions of boundary faces would
  need face-search decomposition — out of scope, flagged for awareness.
- **macOS dev experience.** MPI is now a hard prerequisite everywhere; the
  vcpkg OpenMPI port covers macOS/Linux developer machines with no manual
  setup, and singleton init keeps direct (non-`mpiexec`) launches working for
  debugging and the GUI.
- **`prop`/GUI configuration.** Rank count, skin width, and rebalance cadence
  become solver properties once the `prop`-driven configuration reaches the
  solver executable; until then they are constructor parameters/env vars
  (`TIT_NUM_THREADS`-style).

## 6. Summary of the argument

The codebase is unusually well-positioned for this migration: SoA columns
with compile-time field sets make generic halo packing nearly free; the
multi-level partitioning in `ParticleMesh` is architecturally the same
mechanism a rank decomposition needs; and the `geom::partition` algorithms
(RIB, Hilbert sort) are the exact algorithms used for SPH domain
decomposition in production codes. The work is therefore mostly *relocation
and layering* — extracting the partition dimension out of `ParticleMesh`,
adding an ownership dimension to `ParticleArray`, and inserting a small,
well-scheduled set of halo refreshes between existing solver phases — rather
than a rewrite of the physics.
