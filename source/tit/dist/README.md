# `tit/dist`

This library contains the distributed-particle infrastructure: domain
decomposition, particle ownership migration, and ghost (halo) particle
exchange, built on top of `tit/mpi` and the geometric partitioning
algorithms of `tit/geom`.

The library is SPH-aware — it operates on `tit::sph::ParticleArray` — but
equation-agnostic: the solver interacts with it exclusively through the
`tit::sph::exchange_for` interface.

Everything in this library should be placed into `tit::dist` namespace.
