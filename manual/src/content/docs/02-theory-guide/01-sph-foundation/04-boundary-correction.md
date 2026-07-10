---
title: Boundary Correction
---

The SPH operators on the following pages depend on two geometric quantities
that encode the truncation of the kernel support by the domain boundary:
$\gamma_a$ and $\nabla\gamma_a$ (and its per-face counterpart
$\nabla\gamma_{as}$). These depend only on the kernel and the boundary
geometry, not on the field being interpolated.

<!----------------------------------------------------------------------------->

## Kernel support fraction

The fraction of the kernel support that lies inside the domain at a point
$\vec{r}_a$ is defined by the continuous integral

$$
\gamma_a =
  \gamma(\vec{r}_a) =
  \int_{\Omega} W(\vec{r}_a - \vec{r}', h) d\vec{r}'.
$$

For an interior particle whose kernel neighborhood lies entirely within the
domain, $\gamma = 1$. Near a boundary the support is truncated and
$\gamma < 1$.

In BlueTit Solver the value of $\gamma_a$ is obtained via direct evaluation
using the kernel antigradient, described below.

The antigradient $\vec{A}(\vec{r}, h)$ is a compact-support vector radial
function that shares the same support as the kernel $W(\vec{r}, h)$ and
satisfies

$$
\nabla \cdot \vec{A}(\vec{r}, h) = W(\vec{r}, h) - \delta(\vec{r}).
$$

It is defined as

$$
\vec{A}(\vec{r}, h) = -
  \frac{\omega_{d}}{h^{d}} m_d\left( \frac{|\vec{r}|}{h} \right),
$$

$\omega_{d}$ is a normalization constant,

$$
m_d(q) = \int_q^R \xi^{d-1} w(\xi) \, d\xi
$$

is the tail moment of the unit kernel $w(\xi)$.

The antigradient is related to the kernel by the divergence theorem: applying
$\nabla \cdot \vec{A} = W - \delta$ and integrating over the domain gives

$$
\gamma_a =
  \mathrm{I}_{\vec{r}_a \in \Omega} -
  \oint_{\partial\Omega}
    \vec{A}(\vec{r}_a - \vec{r}', h) \cdot \vec{n} d\Gamma' =
  \mathrm{I}_{\vec{r}_a \in \Omega} -
  \sum_{s \in \mathcal{S}} \gamma_{as},
$$

where the per-face contribution is the surface integral restricted to face
$s$:

$$
\gamma_{as} =
  \gamma_{s}(\vec{r}_a) =
  \vec{n}_s \cdot
  \int_{\partial\Omega_s} \vec{A}(\vec{r}_a - \vec{r}', h) d\Gamma'.
$$

This identity allows $\gamma$ to be evaluated from boundary surface integrals
alone, avoiding the costly volumetric integral over $\Omega$.

Directly evaluating $\gamma_a$ is problematic when $\vec{r}$ lies
on the surface $\partial\Omega_s$, where the antigradient is singular. This is
resolved using a Taylor expansion that shifts $\vec{r}_{a}$ away from the
boundary by a small amount $\vec{\Delta}_a$:

$$
\tilde{\gamma}_a =
  \mathrm{I}_{\vec{r}_a \in \Omega} -
  \sum_{s \in \mathcal{S}} \gamma_{s}(\vec{r}_a + \vec{\Delta}_a),
$$

where

$$
\vec{\Delta}_a =
  (2 \mathrm{I}_{\vec{r}_a \in \Omega} - 1) h^2
  \frac{\nabla\gamma_a}{|\nabla\gamma_a|}.
$$

<!----------------------------------------------------------------------------->

## Boundary correction vector

The gradient of $\gamma$ follows from the surface integral representation.
Applying the divergence theorem with the inner normal gives

$$
\nabla\gamma_a =
  \int_{\partial\Omega} W(\vec{r}_a - \vec{r}', h) \vec{n}(\vec{r}') d\Gamma' =
  \sum_{s \in \mathcal{S}} \nabla\gamma_{as},
$$

where the per-face contribution is the surface integral restricted to face
$s$:

$$
\nabla\gamma_{as} =
  \vec{n}_s \int_{\partial\Omega_s} W(\vec{r}_a - \vec{r}', h) d\Gamma'.
$$

For a straight segment (2D) or planar triangle (3D), the kernel integration
over the face can be carried out analytically. In 2D, closed-form primitives
for each kernel type are available. In 3D, high-order numerical quadrature is
used.

BlueTit Solver evaluates $\gamma_{as}$ and $\nabla\gamma_{as}$ analytically for
all available kernels, which is both fast and accurate for the polygonal boundary
representation.

<!----------------------------------------------------------------------------->

## References

1. M. Ferrand, D. R. Laurence, B. D. Rogers, D. Violeau, and C. Kassiotis,
   "Unified semi-analytical wall boundary conditions for inviscid, laminar or
   turbulent flows in the meshless SPH method",
   _International Journal for Numerical Methods in Fluids_,
   vol. 71, no. 4, pp. 446–472, 2013.

2. A. Mayrhofer, M. Ferrand, C. Kassiotis, D. Violeau, and F.-X. Morel,
   "Unified semi-analytical wall boundary conditions in SPH: analytical
   extension to 3-D", _Numerical Algorithms_, vol. 68, pp. 15–34, 2015.
