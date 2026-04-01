---
layout: "~/layouts/page.astro"
---

# Riemann Formulation

The weakly compressible fluid model can be written in two different styles. The
first is the classical symmetric pressure-force formulation presented on the
previous page. The second, discussed here, replaces the direct pressure
symmetrization with interface states and a particle-pair Riemann solver.

This reformulation is especially attractive when one wants lower dissipation
across moving interfaces while keeping the rest of the SPH machinery intact.

For a reader with a finite-volume background, this page is often the most
familiar one in spirit. Reconstruction, one-dimensional interface problems, and
star states are all standard ingredients in mesh-based Godunov methods. The
difference is that here they are attached to particle pairs rather than to
fixed cell faces.

## Motivation

In the classical formulation, neighboring particles exchange momentum through a
directly symmetrized pressure term. That works well in many free-surface
problems, but it also introduces a specific numerical viscosity tied to that
pairwise symmetrization.

The Riemann formulation follows a different idea:

1. construct left and right states on the particle interface
2. solve a one-dimensional Riemann problem along the pair direction
3. use the resulting star states to build density and momentum fluxes

The result remains fully particle based, but the pressure interaction is no
longer taken directly from the two particle values alone.

In that sense, the method moves SPH one step closer to the language of
interface fluxes. It does not become a finite-volume scheme, but it borrows the
idea that local left/right states should be reconstructed before one decides
how momentum and mass are exchanged.

## Interface reconstruction

For a particle pair $(a, b)$ one first introduces the unit direction

$$
\vec{e}_{ab} = \frac{\vec{r}_{ab}}{\lVert \vec{r}_{ab} \rVert}.
$$

If no reconstruction is used, the interface states are simply the particle
states themselves:

$$
q_{L} = q_{a},
\qquad
q_{R} = q_{b}.
$$

With reconstruction, one instead uses the local gradients to extrapolate each
field toward the interface. For a scalar field $q$ this gives the stencil

$$
\left\{
q_{a} - \nabla q_{a} \cdot \vec{r}_{ab},
\;
q_{a},
\;
q_{b},
\;
q_{b} + \nabla q_{b} \cdot \vec{r}_{ab}
\right\}.
\label{riemann-reconstruction-stencil}
$$

The left and right interface values are then reconstructed from these four
pointwise samples.

For vector fields the same idea is applied to the component along
$\vec{e}_{ab}$, since the Riemann problem is posed in the normal direction of
the pair.

## WENO viewpoint

The reconstruction used here belongs to the WENO family. The basic principle is
to combine several candidate one-sided reconstructions with weights that depend
on local smoothness. Smooth regions receive high-order interpolation, while
oscillatory regions automatically shift the weights toward the more regular
stencil.

In the present setting this means:

- smooth particle neighborhoods obtain sharper interface states
- steep transitions are reconstructed more cautiously
- the Riemann solver receives better left/right data than raw particle values

This is the main reason reconstruction and Riemann fluxes naturally fit
together.

For CFD users, the analogy is direct: the smoother and more informative the
interface reconstruction, the less one must rely on blunt numerical damping in
the flux itself.

## Pressure from reconstructed density

In the weakly compressible setting, pressure is reconstructed indirectly. One
first reconstructs density, then evaluates the equation of state on the left
and right:

$$
p_{L} = p(\rho_{L}),
\qquad
p_{R} = p(\rho_{R}).
$$

For a linear Tait law this becomes

$$
p_{L} = p_{0} + c_{0}^{2}(\rho_{L} - \rho_{0}),
\qquad
p_{R} = p_{0} + c_{0}^{2}(\rho_{R} - \rho_{0}).
$$

So the interface pressure is not an independent unknown: it is derived from the
reconstructed density field.

## Star states

Once the left and right states are available, the pairwise Riemann problem is
solved along $\vec{e}_{ab}$. Introducing

$$
\rho_{ab} = \frac{\rho_{L} + \rho_{R}}{2},
\qquad
\Delta p_{ab} = p_{R} - p_{L},
\qquad
\Delta v_{ab} = (\vec{v}_{R} - \vec{v}_{L}) \cdot \vec{e}_{ab},
$$

one defines the limiter

$$
\beta = \operatorname{clamp}(3 \Delta v_{ab}, 0, c_{0}).
\label{riemann-beta}
$$

The resulting star states are

$$
p^{\ast}
=
\frac{p_{L} + p_{R}}{2}
+
\frac{1}{2}\beta \rho_{ab} \Delta v_{ab},
\label{riemann-p-star}
$$

$$
\vec{v}^{\ast}
=
\frac{\vec{v}_{L} + \vec{v}_{R}}{2}
+
\frac{\Delta p_{ab}}{2 \rho_{ab} c_{0}} \vec{e}_{ab}.
\label{riemann-v-star}
$$

These two quantities summarize the resolved interface interaction:

- $p^{\ast}$ replaces the directly symmetrized pressure term
- $\vec{v}^{\ast}$ replaces the raw pairwise interface velocity

This is the conceptual heart of the method. Instead of saying "the two
particles push on one another according to their raw pressures," the method
says "the pair defines a small interface problem, and the resolved interface
state determines the exchange."

## Density update

With the star velocity available, the density equation becomes

$$
\frac{d \rho_{a}}{d t}
=
2 \rho_{a} V_{b}
\left(
\vec{v}_{a} - \vec{v}^{\ast}
\right)
\cdot \nabla W_{ab},
\label{riemann-density-a}
$$

$$
\frac{d \rho_{b}}{d t}
=
-2 \rho_{b} V_{a}
\left(
\vec{v}_{b} - \vec{v}^{\ast}
\right)
\cdot \nabla W_{ab}.
\label{riemann-density-b}
$$

This is one of the conceptual differences from the classical WCSPH density
update: the interface velocity entering the flux is now the star velocity
returned by the Riemann problem.

## Momentum update

The momentum equation takes the form

$$
\frac{d \vec{v}_{a}}{d t}
=
\sum_{k} \vec{g}_{k}(a)
+
\sum_{b} m_{b}
\left(
\Pi_{ab}^{\mu} - \frac{2 p^{\ast}}{\rho_{a} \rho_{b}}
\right)
\nabla W_{ab},
\label{riemann-momentum-a}
$$

with the opposite pair contribution applied to particle $b$.

Compared with the classical SPH pressure force, the central pressure term is now
driven by $p^{\ast}$ rather than by the two particle pressures separately.
This is precisely the mechanism through which the method reduces dissipation and
sharpens the interface response.

## Relation to the standard formulation

The classical and Riemann formulations should be thought of as two ways of
closing the same SPH backbone. They share:

- the same kernels
- the same particle neighborhoods
- the same density, velocity, and smoothing-length notation
- the same explicit time-stepping framework

What changes is the interpretation of the particle pair:

- in the classical formulation, the pair contributes through a directly
  symmetrized pressure force and optional density diffusion
- in the Riemann formulation, the pair defines a local interface problem whose
  solution produces star-state fluxes

So the Riemann formulation is not a different particle method in spirit. It is
best understood as a sharper inter-particle flux model built on the same SPH
approximation framework.

For many CFD readers, this is the point where SPH begins to feel less alien.
The mesh is still gone, but the language of reconstruction, interface states,
and dissipation control returns in a recognizable form.

## Practical reading of the method

A useful way to read this formulation is:

1. reconstruct density and velocity near the interface
2. compute interface pressure from the reconstructed density
3. solve for interface star states
4. use those star states in the density and momentum fluxes

That perspective highlights the method as a sequence of increasingly refined
approximations to the local particle interaction, rather than as a disconnected
collection of formulas.
