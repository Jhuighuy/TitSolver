---
children:
  - sph
  - fluid-equations
  - riemann-formulation
  - numerics
  - references
layout: "~/layouts/page.astro"
---

# Theory Guide

This guide collects the mathematical background behind the fluid methods used
in BlueTit Solver. It is written for readers who may already be comfortable
with computational fluid dynamics, but who have had limited exposure to
particle methods and to SPH in particular.

The section is organized as a short article sequence. It begins with the common
SPH approximation machinery, moves to the standard weakly compressible fluid
model, then to the low-dissipation Riemann variant, and finally to the
numerical devices needed to advance and stabilize those equations in practice.

Throughout the discussion it is useful to keep one broad difference from
traditional CFD in mind. In a finite-volume or finite-element method, the
discrete unknowns live on a mesh and fluxes are exchanged across control-volume
faces or element boundaries. In SPH, the unknowns are carried by moving
particles, and interaction is built from weighted particle pairs inside a local
kernel support. Many familiar conservation laws remain the same, but the
discrete viewpoint is different from the very beginning.

## Included topics

- [SPH Framework](./sph.html) introduces the shared SPH notation: kernels,
  particle interpolation, density estimation, and corrected first-order
  operators.
- [Fluid Equations](./fluid-equations.html) derives the weakly compressible
  SPH formulation as a density-pressure-velocity model for free-surface flow.
- [Riemann Formulation](./riemann-formulation.html) explains the low-dissipation
  interface treatment based on reconstruction and particle-pair star states.
- [Numerics](./numerics.html) covers explicit time integration, particle
  shifting, and wall treatment.
- [References](./references.html) collects article links for the methods named
  throughout the guide.

## Reading path

If you are new to the solver, read the pages in this order:

1. [SPH Framework](./sph.html)
2. [Fluid Equations](./fluid-equations.html)
3. [Riemann Formulation](./riemann-formulation.html)
4. [Numerics](./numerics.html)
5. [References](./references.html)

The first page establishes notation. The next two pages describe the two fluid
interaction models. The numerics page then explains how those equations are
advanced in time and stabilized. The references page can be used as a
bibliography while reading the rest of the section.
