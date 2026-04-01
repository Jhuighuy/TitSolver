---
layout: "~/layouts/page.astro"
---

# Numerics

This page describes the numerical layer that surrounds the fluid equations:
explicit time integration, particle shifting, and the current wall treatment.
These topics are not separate from the theory. They determine how the particle
equations are actually advanced and stabilized in time.

For users coming from traditional CFD, these topics may seem unusually visible.
On a mesh, geometry regularity and boundary representation are partly handled by
the grid itself. In SPH, particle arrangement is part of the numerical state,
so stabilization and boundary modeling must be discussed much more explicitly.

## Overview

Once a particle method has been written down, three additional questions remain:

1. how should the equations be integrated in time?
2. how should particle disorder be controlled?
3. how should fixed boundaries be represented?

The current fluid solver answers those questions with explicit Runge-Kutta or
kick-drift style schemes, a free-surface-aware particle shifting method, and a
ghost-particle wall treatment.

This is a useful place to pause on the contrast with conventional methods. A
finite-volume code typically separates spatial discretization, mesh quality, and
boundary closure into different conceptual boxes. In SPH those boxes overlap:
particle motion, particle regularity, and wall reconstruction all influence the
quality of the discrete operators.

## Explicit time integration

The particle equations are advanced explicitly, meaning that the right-hand side
is evaluated from the current particle state and then used immediately to
produce the next one.

That choice matches the local nature of SPH. Since the weakly compressible
model avoids a global pressure solve, the method naturally favors explicit
integrators, with the time step controlled by acoustic, viscous, and body-force
restrictions.

### Kick-drift Euler

The simplest update is the first-order kick-drift step

$$
\rho_{a}^{n+1} = \rho_{a}^{n} + \Delta t \, \dot{\rho}_{a}^{n},
$$

$$
\vec{v}_{a}^{n+1} = \vec{v}_{a}^{n} + \Delta t \, \dot{\vec{v}}_{a}^{n},
$$

$$
\vec{r}_{a}^{n+1} = \vec{r}_{a}^{n} + \Delta t \, \vec{v}_{a}^{n+1}.
$$

Velocity is updated first, then position is drifted with the kicked velocity.
The method is straightforward and inexpensive, but only first-order accurate in
time.

### Kick-drift-kick leapfrog

The leapfrog idea splits the kick into two half-steps:

$$
\vec{v}_{a}^{n+1/2}
=
\vec{v}_{a}^{n} + \frac{\Delta t}{2} \dot{\vec{v}}_{a}^{n},
$$

$$
\vec{r}_{a}^{n+1}
=
\vec{r}_{a}^{n} + \Delta t \, \vec{v}_{a}^{n+1/2},
$$

$$
\vec{v}_{a}^{n+1}
=
\vec{v}_{a}^{n+1/2} + \frac{\Delta t}{2} \dot{\vec{v}}_{a}^{n+1}.
$$

This improves temporal behavior while preserving the explicit structure.

### SSPRK(3,3)

The strongest explicit integrator currently used for fluid runs is the
three-stage strong-stability-preserving Runge-Kutta method:

$$
U^{(1)} = U^{n} + \Delta t \, \mathcal{L}(U^{n}),
$$

$$
U^{(2)} =
\frac{3}{4} U^{n}
+
\frac{1}{4}
\left(
U^{(1)} + \Delta t \, \mathcal{L}(U^{(1)})
\right),
$$

$$
U^{n+1} =
\frac{1}{3} U^{n}
+
\frac{2}{3}
\left(
U^{(2)} + \Delta t \, \mathcal{L}(U^{(2)})
\right).
$$

Its attraction is that it preserves the explicit particle structure while
providing better temporal accuracy and nonlinear stability behavior than a
single Euler step.

## Particle shifting

Particle methods naturally suffer from disorder: particles may clump, stretch,
or form irregular local patterns even when the underlying flow is smooth.
Particle shifting is a numerical correction that redistributes particles
slightly without changing the physical model one aims to approximate.

The shifting procedure used here is deliberately tied to free-surface flow.

### Surface classification

Before shifts are computed, particles are classified as:

- on the free surface
- near the free surface
- far from the free surface

This classification uses a renormalized normal proxy together with local
visibility tests. The idea is that particles close to the free surface should
not be shifted in the same way as interior particles, because there the local
support is incomplete and aggressive shifting can damage the interface.

### Shift formula

Once the classification is known, the shift applied to a particle can be written
schematically as

$$
\Delta \vec{r}_{a}
=
- \sum_{b}
\left(
\Xi_{a} + \Chi_{ab}
\right)
FS_{a}
\frac{m_{b}}{\rho_{b}} \nabla W_{ab},
\label{implemented-shift}
$$

with a symmetric opposite contribution for the neighboring particle.

The kernel-weight factor is

$$
\Chi_{ab}
=
R
\left(
\frac{W_{ab}}{W_{0}}
\right)^{4},
\qquad
W_{0} = W\!\left(\frac{h}{2}\right),
$$

while $\Xi_{a}$ acts as an indicator that distinguishes far-surface particles
from interface particles.

The purpose of this construction is practical:

- shift interior particles more strongly
- weaken shifts near the free surface
- suppress problematic shifts near walls

So particle shifting here is best understood as a geometry-aware stabilization
device rather than as part of the continuum model itself.

This is one of the sharpest differences from traditional CFD intuition. In a
mesh method, one hopes the grid quality is already acceptable before time
stepping begins. In SPH, the particle set is the evolving geometry, so some
amount of regularization may be needed throughout the computation.

## Boundary treatment

The current wall treatment is based on mirrored ghost points and local
interpolation. It should be viewed as the present numerical boundary model, not
as the only possible SPH boundary theory.

Again, the comparison with mesh-based CFD is instructive. On a body-fitted
mesh, a wall boundary condition is usually enforced directly on a geometric
boundary face. In SPH, one must instead construct a boundary state that can
participate in kernel interactions with nearby fluid particles.

### Ghost construction

For a fixed boundary particle one introduces a mirrored ghost point

$$
\vec{r}_{g} = 2 \, \Pi(\vec{r}_{b}) - \vec{r}_{b},
$$

where $\Pi(\vec{r}_{b})$ denotes a clamped point on the domain boundary. The
surrounding fluid particles are then used to interpolate boundary density,
velocity, and, when relevant, internal energy.

Two levels of reconstruction are attempted:

- first, a local linear reconstruction
- otherwise, a constant kernel interpolation

If neither is reliable, a fallback state is used.

### Hydrostatic correction

After interpolation, the boundary density receives an additional hydrostatic
correction

$$
\rho_{b}
\leftarrow
\rho_{b}
+
S_{D} \frac{\rho_{0}}{c_{0}^{2}} \vec{G} \cdot \vec{S}_{N},
\label{boundary-hydrostatic-correction}
$$

where $S_{D}$ is the ghost distance and $\vec{S}_{N}$ is the local wall-normal
direction.

This term is meant to keep the boundary state compatible with the pressure law
under gravity, rather than leaving the boundary density as a purely geometric
interpolation.

### Slip reflection

The boundary velocity is finally reflected into a slip-wall state by separating
normal and tangential components,

$$
\vec{v}_{b} = \vec{V}_{t} - \vec{V}_{n}.
$$

So tangential motion is preserved, while the normal component is reflected.

## How to read the numerical layer

The numerical layer can be summarized as follows:

- time integration advances the SPH equations explicitly
- particle shifting improves particle regularity in the interior while
  respecting the free surface
- the wall treatment reconstructs ghost states that remain compatible with the
  weakly compressible pressure model

These ingredients are not merely auxiliary numerics. They are part of what
makes the particle equations usable for long free-surface simulations.

For an SPH newcomer with CFD experience, this page is the reminder that
particle methods carry some of their numerical geometry inside the solution
itself. Time stepping, shifting, and boundary treatment are therefore not side
issues around the equations. They are part of the mathematical story of how the
method remains stable and accurate in practice.
