# `tit/graph`

This library contains the everything related to graph theory.

Everything in this library should be placed into `tit::graph` namespace.

## Graph partitioning

Why? Because no actual graph partitioning library suits our needs.

- *Metis*. It can produce high-quality partitioning, but it's **very** slow.
- *Scotch*. It simply does not compile on ARM64.
- *KaHIP* and it's siblings. Not available in package managers.
- *Boost Graph Library*. Does not have a complete K-way partitioner.

So, we have to write our own partitioner. In SPH, we are not only dealing with
rather dense graphs (tens of edges per node), unlike the regular mesh-based
applications (4-6 edges per node), but also we have to partition the graph
in real-time. So, our partitioner should be very fast, even with a price of
not producing optimal edge cut.

To overcome the issue of too many connections, we are using a
simple geometric coarsening algorithm (see `tit::geom::GridGraphPartition`).

I've ended up with a classic V-cycle multilevel K-way partitioner, using
Greedy Edge Matching (GEM) algorithm for coarsening, connected-component-aware
Greedy Growing Partitioner (GGP) for partitioning the coarsest graph, and
Feduccia-Mattheyses-style refinement with rollback.

It's rather fast (order of magnitude faster than Metis), and it's even is
capable of edge cut better than Metis. Please see the details in the code and
good luck maintaining it.
