---
layout: "~/layouts/page.astro"
---

# Fluid Equations

This page describes the weakly compressible SPH fluid model as a theory of
particle-based mass and momentum transport. The emphasis is on the structure of
the equations rather than on coding details: what evolves, how pressure
enters, why dissipation is needed, and how this viewpoint differs from the
pressure-correction and flux-balance ideas familiar from mesh-based CFD.

## Overview

The fluid formulation treats the medium as a set of moving particles carrying
mass, momentum, and, when needed, internal energy. Once the SPH interpolation
machinery from the previous page is in place, the remaining task is to define
how those particle quantities evolve in time.

For a finite-volume reader, one useful mental model is the following. In a
classical cell-centered method, density, momentum, and energy live in control
volumes and change because fluxes cross cell faces. In SPH, the unknowns are
attached to moving particles, and the analog of a numerical flux is a weighted
pair interaction with nearby particles. The conservation laws are the same, but
the discrete topology is no longer a mesh.

For weakly compressible SPH, the governing fields are:

- particle position $\vec{r}$
- velocity $\vec{v}$
- density $\rho$
- pressure $p$
- optionally internal energy $u$

The central modeling idea is simple:

- density is evolved from the local velocity field
- pressure is reconstructed from density by an equation of state
- acceleration is driven by pressure gradients, body forces, and dissipation
- thermal effects, when included, are represented through an internal-energy
  equation

## Continuum equations

In continuum form the model can be written as

$$
\frac{D \rho}{D t} = - \rho \nabla \cdot \vec{v} + S_{\rho},
\label{fluid-continuity}
$$

$$
\frac{D \vec{v}}{D t} = - \frac{1}{\rho} \nabla p + \vec{g} + \vec{f}_{\mu} +
\vec{f}_{\Pi},
\label{fluid-momentum}
$$

and, when thermal evolution is retained,

$$
\frac{D u}{D t} = - \frac{p}{\rho} \nabla \cdot \vec{v} + q + S_{u}.
\label{fluid-energy}
$$

Here $S_{\rho}$ denotes any mass-source contribution, $\vec{g}$ is a body
force such as gravity, $\vec{f}_{\mu}$ is the physical viscosity term,
$\vec{f}_{\Pi}$ is a numerical or artificial dissipation contribution, and $q$
is a conductive energy flux.

The weakly compressible setting closes the system by prescribing pressure as a
function of density, rather than solving an elliptic pressure equation.

This is another major difference from many traditional incompressible CFD
workflows. Instead of enforcing $\nabla \cdot \vec{v} = 0$ through a global
Poisson solve, weakly compressible SPH allows small density variations and
recovers pressure algebraically from an equation of state. The reward is a
fully local method; the cost is acoustic stiffness and a smaller stable time
step.

## Density evolution

In particle form the continuity equation is written as a pairwise interaction
between neighboring particles:

$$
\frac{d \rho_{a}}{d t}
=
\sum_{k} S_{\rho, k}(a)
- \sum_{b} m_{b}
\left(
\vec{v}_{ba} - \frac{\vec{\Psi}_{ab}}{\rho_{b}}
\right)
\cdot \nabla W_{ab}.
\label{implemented-density}
$$

The first term represents any source contribution. The second term is the SPH
approximation of $-\rho \nabla \cdot \vec{v}$, augmented by a diffusive
correction $\vec{\Psi}_{ab}$.

This structure is important because it shows how density plays two roles at
once:

- it is a transported variable, evolved in time
- it also enters the pressure law that drives the momentum equation

The formulation therefore couples density and momentum very tightly.

From a CFD perspective, this means that density is not merely an auxiliary
variable reconstructed from a pressure solve. It is an actively evolved field,
and pressure is a consequence of that evolution.

### Density diffusion

In weakly compressible free-surface flows, density oscillations can be one of
the dominant numerical difficulties. A common cure is to add an SPH-consistent
diffusive term to the continuity equation.

One such form is

$$
\vec{\Psi}_{ab}
=
2 \, \delta c_{0} h_{ab}
\left(
\rho_{ba} - \overline{\nabla \rho}_{ab} \cdot \vec{r}_{ab}
\right)
\frac{\vec{r}_{ab}}{\lVert \vec{r}_{ab} \rVert^{2}},
\label{delta-sph-density-diffusion}
$$

which corresponds to the $\delta$-SPH correction used in the standard fluid
configuration. The correction is proportional to a reference sound speed
$c_{0}$, a smoothing length $h_{ab}$, and a local density difference corrected
by the average density gradient.

The purpose of this term is not to change the physical model, but to regularize
high-frequency density noise while keeping the SPH pairwise structure intact.

## Momentum equation

The particle form of the momentum equation can be written as

$$
\frac{d \vec{v}_{a}}{d t}
=
\sum_{k} \vec{g}_{k}(a)
+
\sum_{b} m_{b}
\left(
- \frac{p_{a}}{\rho_{a}^{2}}
- \frac{p_{b}}{\rho_{b}^{2}}
+ \Pi_{ab}
\right)
\nabla W_{ab},
\label{implemented-momentum}
$$

where $\Pi_{ab}$ collects dissipative contributions.

This expression should be read term by term:

- the two pressure contributions form the symmetric SPH pressure force
- $\Pi_{ab}$ modifies that pairwise interaction through physical or artificial
  dissipation
- body forces are applied separately and then added to the total acceleration

The symmetrized pressure term is one of the most recognizable formulas in SPH.
It ensures that interacting particle pairs contribute equal and opposite
momentum exchange, which is one of the reasons the method remains robust for
strongly deforming free surfaces.

Compared with a mesh method, one may think of this term as a face-flux analog
without an actual face. The pair $(a,b)$ plays the role of a local interaction
channel, and the kernel gradient provides its directional weighting.

## Dissipation mechanisms

The formulation allows several dissipative closures, all of them written in the
same pairwise style.

### Physical viscosity

When a Laplacian-style viscous model is used, the pair contribution takes the
form

$$
\Pi_{ab}^{\mu}
=
2 (d + 2)\mu_{ab}
\frac{\vec{r}_{ab} \cdot \vec{v}_{ab}}
{\rho_{a} \rho_{b} \lVert \vec{r}_{ab} \rVert^{2}}.
$$

This term approximates viscous momentum diffusion in a way that is compatible
with the particle discretization.

### Artificial viscosity

Artificial viscosity is used for a different purpose. It is not intended to
represent molecular transport directly, but to stabilize the method in regions
of strong compression or large gradients.

The classical Monaghan $\alpha$-$\beta$ form is

$$
\Pi_{ab}^{\mathrm{art}}
=
\frac{(\alpha c_{ab} - \beta \mu_{ab}) \mu_{ab}}{\rho_{ab}},
\qquad
\mu_{ab}
=
h_{ab}
\frac{\vec{v}_{ab} \cdot \vec{r}_{ab}}
\lVert \vec{r}_{ab} \rVert^{2}},
$$

applied only for approaching particles.

In the standard weakly compressible free-surface configuration, a simpler
velocity-diffusion term is used:

$$
\Pi_{ab}^{\mathrm{art}}
=
\alpha c_{0} \rho_{0} h_{ab}
\frac{\vec{r}_{ab} \cdot \vec{v}_{ab}}
{\rho_{a} \rho_{b} \lVert \vec{r}_{ab} \rVert^{2}}.
\label{delta-sph-velocity-diffusion}
$$

Its role is to damp oscillatory pairwise motion while remaining consistent with
the rest of the weakly compressible framework.

## Equation of state

Pressure is reconstructed from density through an equation of state. In the
weakly compressible setting the most important idea is not thermodynamic
realism, but the introduction of a finite numerical compressibility controlled
by a chosen reference sound speed.

The linear Tait form reads

$$
p = p_{0} + c_{0}^{2} (\rho - \rho_{0}),
\label{linear-tait}
$$

while the nonlinear Tait family takes the form

$$
p = p_{0} + B
\left[
\left( \frac{\rho}{\rho_{0}} \right)^{\gamma} - 1
\right].
$$

The same framework also admits ideal-gas-type closures,

$$
p = (\gamma - 1)\rho u,
\qquad
p = \kappa \rho^{\gamma},
$$

which are useful when the problem is more naturally written in terms of
compressible thermodynamics rather than free-surface water flow.

In practice, the choice of EOS sets the acoustic stiffness of the numerical
model and strongly affects the stable time step.

That tradeoff is central in WCSPH. A larger artificial sound speed makes the
flow behave more nearly incompressibly, but it also tightens the CFL
restriction. The method therefore replaces the global algebra of pressure
projection with a local but acoustically stiff evolution law.

## Internal-energy equation

When internal energy is evolved explicitly, the particle equation is

$$
\frac{d u_{a}}{d t}
=
\sum_{k} S_{u, k}(a)
- \sum_{b} m_{b}
\left[
\left(
\frac{p_{a}}{\rho_{a}^{2}} - \frac{\Pi_{ab}}{2}
\right) \vec{v}_{ba}
- \vec{Q}_{ab}
\right]
\cdot \nabla W_{ab}.
\label{implemented-energy}
$$

The first contribution inside the bracket is pressure work, corrected by the
dissipative term. The second is conductive heat transfer. A pairwise
conductivity model can be written as

$$
\vec{Q}_{ab}
=
2 \, \kappa_{ab}
\frac{u_{ba} \, \vec{r}_{ab}}
{c_{v} \rho_{a} \rho_{b} \lVert \vec{r}_{ab} \rVert^{2}}.
\label{implemented-heat-conductivity}
$$

This term transfers thermal energy along the line joining the particles, again
following the SPH philosophy of constructing the model from pairwise exchanges.

## Overview of common variants

The fluid framework supports several closures around the same core pairwise
equations:

- equations of state:
  linear Tait, nonlinear Tait, ideal gas, adiabatic ideal gas
- physical viscosity:
  none or Laplacian viscosity
- artificial viscosity / density diffusion:
  none, Monaghan $\alpha$-$\beta$, Molteni-Colagrossi, or Marrone
  $\delta$-SPH
- thermal evolution:
  omitted, or included with pairwise heat conduction

So the fluid model should be viewed as a family of closely related weakly
compressible SPH formulations rather than a single rigid equation set.

## Typical weakly compressible free-surface configuration

For free-surface water-flow problems, a particularly natural choice is:

- a Wendland kernel
- a Tait-type equation of state
- gravity as a body force
- density diffusion for weakly compressible stabilization
- no separate thermal evolution

That combination produces the standard weakly compressible free-surface model
used throughout the rest of this theory section.

For readers coming from conventional CFD, the main takeaway is that SPH does
not discard the continuum equations. Instead, it rewrites their discrete
approximation around moving particles, compact kernel supports, and pairwise
interactions. Once that shift in viewpoint becomes natural, the remaining
equations are much less mysterious than the notation first suggests.
