# `tit/mpi`

This library contains a thin RAII wrapper over the Message Passing Interface.

It intentionally knows nothing about particles or the solver: it only maps
MPI concepts (runtime control, communicators, datatypes, collective and
point-to-point operations) onto the C++ core of the codebase — errors become
exceptions, handles become value types, and buffers become spans.

Everything in this library should be placed into `tit::mpi` namespace.
