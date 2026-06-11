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
\gamma(\vec{r}_a) =
  \int_{\Omega} W(\vec{r}_a - \vec{r}', h) d\vec{r}'.
$$

For an interior particle whose kernel neighborhood lies entirely within the
domain, $\gamma = 1$. Near a boundary the support is truncated and
$\gamma < 1$.

Computing $\gamma_a$ accurately is difficult. Analytical strategies that
integrate the kernel over the polygonal domain exist, but they are either
computationally expensive or lack robustness for general mesh configurations.

In BlueTit Solver the value of $\gamma_a$ is obtained via the evolution equation
described below.

<!----------------------------------------------------------------------------->

## Boundary correction vector

The gradient of $\gamma$ follows from the surface integral representation.
Applying the divergence theorem with the inner normal gives

$$
\nabla\gamma(\vec{r}_a) =
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
over the face can be carried out analytically. Ferrand et al. [1] provide
closed-form formulas for the C2 kernel in 2D; Mayrhofer et al. [2] give the
3D extension.

BlueTit Solver evaluates $\nabla\gamma_{as}$ analytically for all available
kernels, which is both fast and exact for the polygonal boundary representation.

<!----------------------------------------------------------------------------->

## Evolution equation

Ferrand et al. [1] observed that $\gamma$ satisfies a simple advection
equation. Differentiating the definition of $\gamma$ with respect to time
gives

$$
\frac{d\gamma_a}{dt} =
  \sum_{s \in \mathcal{S}} \vec{v}_{as} \cdot \nabla\gamma_{as},
$$

where $\vec{v}_{as}$ is the velocity of particle $a$ relative to the face $s$.
This ODE can be integrated alongside the fluid equations to evolve $\gamma$ as
particles move, avoiding a fresh direct evaluation of the costly domain integral
at every step.

A CFL-like condition is required when the ODE is integrated in time:

$$
\Delta t \leq
  C_\gamma \,
  \min_{a \in \mathcal{P}, s \in \mathcal{S}}
  \frac{1}{\left| \vec{v}_{as} \cdot \nabla\gamma_{as} \right|},
  \qquad
  C_\gamma \approx 5 \times 10^{-3}.
$$

<!----------------------------------------------------------------------------->

## Computing support fraction

The evolution equation can be used to obtain $\gamma_a$ before the simulation
starts. Deep inside the domain the kernel is fully supported, so $\gamma = 1$
is known exactly. The idea is to start at a point where $\gamma$ is known and
integrate along a straight line toward the boundary, using the path integral
$\gamma_a = 1 + \int \nabla\gamma \cdot d\vec{r}$.

The procedure for each particle $a$ is:

1. **Evaluate $\nabla\gamma_a$** using the analytical per-face formulas.

2. **Far from the boundary:** if $|\nabla\gamma_a| = 0$ then the kernel is fully
   supported and $\gamma_a = 1$.

3. **Near the boundary:** if $|\nabla\gamma_a| > 0$, define a starting point
   that lies at least a kernel diameter inside the domain, in the direction
   of $\nabla\gamma_a$:

   $$
   \vec{r}_a^* =
     \vec{r}_a + 2 R h \,
     \frac{\nabla\gamma_a}{|\nabla\gamma_a|},
   $$

   where $R$ is the kernel support radius. At $\vec{r}_a^*$ the kernel
   neighborhood is fully inside the domain, so $\gamma(\vec{r}_a^*) = 1$.

4. **Integrate** $\gamma$ along the straight line from $\vec{r}_a^*$ back to
   $\vec{r}_a$. Along the path $d\gamma = \nabla\gamma \cdot d\vec{r}$, so

   $$
   \gamma_a =
     1 +
     \int_{\vec{r}_a^*}^{\vec{r}_a}
       \nabla\gamma(\vec{r}) \cdot d\vec{r}.
   $$

   Discretizing with the trapezoidal rule:

   $$
   \gamma^{(n+1)} = \gamma^{(n)} +
     \frac{1}{2}
     \Bigl[
       \nabla\gamma
         \bigl( \vec{r}^{(n+1)} \bigr) +
       \nabla\gamma
         \bigl( \vec{r}^{(n)} \bigr)
     \Bigr] \cdot
     \Bigl[
       \vec{r}^{(n+1)} - \vec{r}^{(n)}
     \Bigr],
   $$

   starting from $\vec{r}^{(0)} = \vec{r}_a^*$, $\gamma^{(0)} = 1$ and
   stopping when $\vec{r}$ reaches $\vec{r}_a$.

This scheme requires only the analytically computed $\nabla\gamma$ and never
evaluates the domain integral directly. It was first proposed by Ferrand et
al. [1] and is the method used in BlueTit Solver.

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
