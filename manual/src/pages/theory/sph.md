---
layout: "~/layouts/page.astro"
---

# SPH Framework

This page introduces the basic approximation ideas behind _Smoothed Particle
Hydrodynamics_ (SPH). For a reader coming from finite-volume or finite-element
methods, the main conceptual shift is that there is no fixed mesh. The field is
represented by moving particles, and differentiation is replaced by weighted
interactions over a compact neighborhood.

The governing conservation laws are still the usual ones from continuum
mechanics. What changes is the discrete language used to approximate them.
Instead of control volumes, face fluxes, and reconstruction on a mesh, SPH uses
kernel averages, particle volumes, and pairwise interactions inside the support
of a smoothing kernel.

## Kernel function

Dirac's $\delta$-function is defined with the integral identity:

$$
f(\vec{r}) =
\int_{\mathbb{R}^{d}} f(\vec{r}') \delta(\vec{r} - \vec{r}') dV'.
\label{dirac-delta}
$$

Identity $\eqref{dirac-delta}$ is assumed to hold for all functions $f$ in the
space $\mathbb{R}^{d}$. The $\delta$-function may be interpreted as follows:

$$
\delta(\vec{r} - \vec{r}') = \begin{cases}
  \dfrac{1}{dV'}, & \text{if } \vec{r} = \vec{r}' \\
  0,              & \text{otherwise.}
\end{cases}
$$

Therefore, $\delta$ is a function that is zero everywhere except the origin,
where it tends to the reciprocal of the volume differential $dV'$. Replacing
$\delta$ with its finite-valued approximation $W$ in $\eqref{dirac-delta}$ we
obtain the smoothed function $\langle f \rangle$:

$$
\langle f \rangle(\vec{r}) =
\int_{\mathbb{R}^{d}} f(\vec{r}') W(\vec{r} - \vec{r}', h) dV'.
\label{smoothed-function}
$$

We refer to $W = W(\vec{r}, h)$ in $\eqref{smoothed-function}$ as the smooth
_kernel_ function with radius $h$. The kernel is often defined in terms of a
radial function $w = w(q)$:

$$
W(\vec{r}, h) = \frac{\omega_{d}}{h^{d}} \, w(q),
$$

where $\omega_{d}$ is a normalization constant and $q = |\vec{r}| / h$.
The solver supports the following kernels:

| Name                     | Radial function                                                                                                                       | Normalization constant                                                               |
| ------------------------ | ------------------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------ |
| Gaussian                 | $\exp(-q^2)$                                                                                                                          | $\sqrt{\pi}^{-d}$                                                                    |
| Cubic B-spline (M4)      | $\frac{1}{4} (q - 2)^3 \cdot \mathbb{I}[q \leq 2] - (q - 1)^3 \cdot \mathbb{I}[q \leq 1]$                                             | $\omega_d = \frac{2}{3}, \frac{10}{7\pi}, \frac{1}{\pi}$ in 1D, 2D, 3D respectively. |
| Quartic B-spline (M5)    | $(q - 2.5)^4 \cdot \mathbb{I}[q \leq 2.5] - 5 (q - 1.5)^4 \cdot \mathbb{I}[q \leq 1.5] + 10 (q - 0.5)^4 \cdot \mathbb{I}[q \leq 0.5]$ | $\frac{1}{24}, \frac{96}{1199\pi}, \frac{1}{20\pi}$ in 1D, 2D, 3D respectively.      |
| Quintic B-spline (M6)    | $(q - 3)^5 \cdot \mathbb{I}[q \leq 3] - 6 (q - 2)^5 \cdot \mathbb{I}[q \leq 2] + 15 (q - 1)^5 \cdot \mathbb{I}[q \leq 1]$             | $\frac{1}{120}, \frac{7}{478\pi}, \frac{1}{120\pi}$ in 1D, 2D, 3D respectively.      |
| Quartic Wendland (C2)    | $(1 + 2 q) \cdot \left( 1 - \frac{q}{2} \right)^4 \cdot \mathbb{I}[q \leq 2]$                                                         | $\frac{3}{4}, \frac{7}{4\pi}, \frac{21}{16\pi}$ in 1D, 2D, 3D respectively.          |
| Quintic Wendland (C4)    | $\left( 1 + 3 q + \frac{35}{12} q^2 \right) \cdot \left( 1 - \frac{q}{2} \right)^6 \cdot \mathbb{I}[q \leq 2]$                        | $\frac{27}{16}, \frac{9}{4\pi}, \frac{495}{256\pi}$ in 1D, 2D, 3D respectively.      |
| 8-th order Wendland (C8) | $\left( 1 + 4 q + \frac{25}{4} q^2 + 4 q^3 \right) \cdot \left( 1 - \frac{q}{2} \right)^8 \cdot \mathbb{I}[q \leq 2]$                 | $\frac{15}{8}, \frac{39}{14\pi}, \frac{339}{128\pi}$ in 1D, 2D, 3D respectively.     |

## Kernel interpolation

Given a set of points $\{ \vec{r}_{a} \}$ and a set of values $\{ f_{a} \}$
associated with those points, one can define a smooth interpolation operator
based on the approximation of the smoothed function
$\eqref{smoothed-function}$:

$$
f(\vec{r}) = \sum_{b} f_{b} W(\vec{r} - \vec{r}_{b}, h) V_{b}.
\label{sph-interpolation}
$$

Point volumes $\{V_{a}\}$ are defined such that the resulting interpolation
preserves the given values in the interpolation points:

$$
f_{a} = \sum_{b} f_{b} W(\vec{r}_{a} - \vec{r}_{b}, h) V_{b}.
\label{sph-identity}
$$

In SPH, the volumes $\{V_{a}\}$ are defined as:

$$
V_{a} = \frac{m_{a}}{\rho_{a}}.
\label{sph-volume}
$$

Here $m_{a}$ is an arbitrarily chosen _mass_ of the point, and $\rho_{a}$ is
the _density_ of the point, computed by substituting the point into the
identity $\eqref{sph-identity}$:

$$
\rho_{a} = \sum_{b} m_{b} W(\vec{r}_{a} - \vec{r}_{b}, h).
\label{sph-density}
$$

> **Note**
>
> In SPH literature, it is common to abbreviate $\vec{r}_{a} - \vec{r}_{b}$ as
> $\vec{r}_{ab}$, and the kernel value $W(\vec{r}_{ab}, h)$ as $W_{ab}$. With
> this notation, equation $\eqref{sph-density}$ becomes
> $\rho_{a} = \sum_{b} m_{b} W_{ab}$.
>
> The symbol $\bar{\vec{r}}_{ab}$ denotes the average of $\vec{r}_{a}$ and
> $\vec{r}_{b}$. We use this notation throughout the rest of the manual.

Therefore, any set of point-value pairs
$\{ ( \vec{r}_{a}, f_{a} ) \}$ can be continuously interpolated by first
computing the density values $\{ \rho_{a} \}$ and volumes $\{ V_{a} \}$ using
$\eqref{sph-density}$ and $\eqref{sph-volume}$ for each point, and then
computing the interpolated function with $\eqref{sph-interpolation}$.

## First order SPH operators

The gradient of an interpolated function can be calculated by:

$$
\nabla f(\vec{r}) = \sum_{b} f_{b} \nabla W(\vec{r} - \vec{r}_{b}, h) V_{b}.
\label{sph-interpolation-gradient}
$$

The discrete gradient operator is obtained by projecting
$\eqref{sph-interpolation-gradient}$ onto the set of points
$\{ \vec{r}_{a} \}$:

$$
\nabla_{0} f_{a} = \sum_{b} f_{b} \nabla W_{ab} V_{b}.
\label{sph-gradient-0}
$$

It is easy to show that the gradient operator $\eqref{sph-gradient-0}$,
computed over a constant function, does not vanish. The subscript $0$ denotes
zeroth order polynomial approximation. A simple correction subtracts the term
$f_{a} \nabla 1_{a}$ from $\eqref{sph-gradient-0}$:

$$
\nabla_{1} f_{a} = \sum_{b} f_{ba} \nabla W_{ab} V_{b}.
\label{sph-gradient-1}
$$

Although $\eqref{sph-gradient-1}$ vanishes for a constant function, it still
fails to compute the gradient of a linear function exactly. A further
correction gives:

$$
\nabla_{2} f_{a} = L_{a} \sum_{b} f_{ba} \nabla W_{ab} V_{b}.
\label{sph-gradient-2}
$$

Here $L_{a}$ is the renormalization matrix:

$$
L_{a} =
\left( \sum_{b} \vec{r}_{ab} \otimes \nabla W_{ab} V_{b} \right)^{-1}.
\label{sph-renormalization-matrix}
$$

Divergence and curl operators can be derived in a similar way. For example, the
divergence operators can be written as:

$$
\nabla_{0} \cdot \vec{F}_{a} =
\sum_{b} \vec{F}_{b} \cdot \nabla W_{ab} V_{b}.
\label{sph-divergence-0}
$$

$$
\nabla_{1} \cdot \vec{F}_{a} =
\sum_{b} \vec{F}_{ba} \cdot \nabla W_{ab} V_{b}.
\label{sph-divergence-1}
$$

$$
\nabla_{2} \cdot \vec{F}_{a} =
\sum_{b} \vec{F}_{ba} \cdot L_{a} \nabla W_{ab} V_{b}.
\label{sph-divergence-2}
$$

## Second order SPH operators

Second-order SPH operators can be naively written as:

$$
\nabla \cdot \nabla f_{a} =
\sum_{b}
\underbrace{
  \left(
    \sum_{c} f_{c} \nabla W_{bc} V_{c}
  \right)
}_{\nabla f_{b}} \cdot \nabla W_{ab} V_{b}.
$$

Second-order operators are commonly introduced when one wishes to model
diffusion, viscosity, or heat conduction. In continuum form these effects are
typically governed by the Laplacian or by the divergence of a gradient:

$$
\Delta f = \nabla \cdot (\nabla f).
$$

In a particle setting, a naive second differentiation of the kernel is usually
too sensitive to particle disorder. For that reason, practical SPH
discretizations often rewrite second-order terms into pairwise forms that
depend on first differences between neighboring particles.

A representative scalar Laplacian operator can be written as

$$
\Delta f_a =
2 (d + 2)
\sum_b
f_{ba}
\frac{\vec{r}_{ab}}{\lVert \vec{r}_{ab} \rVert^2} \cdot \nabla W_{ab}
V_b.
\label{sph-laplacian}
$$

This formula should be interpreted as the particle analogue of a diffusive
flux balance. The factor $\vec{r}_{ab} \cdot \nabla W_{ab}$ extracts the
directional contribution of the neighbor pair, while the quotient
$f_{ba} / \lVert \vec{r}_{ab} \rVert^2$ plays a role similar to a discrete
gradient along the line joining the particles.

In actual fluid models, this operator rarely appears in isolation. More often,
it enters multiplied by a diffusivity and embedded inside momentum or energy
equations. Nevertheless, $\eqref{sph-laplacian}$ is useful as a prototype,
because it shows the main structural principle: second-order effects are still
assembled from local pairwise interactions rather than from derivatives taken
on a fixed grid.

From the standpoint of numerical analysis, this construction is closely related
to edge-based diffusion on unstructured meshes. In both settings one seeks a
stable approximation of a second derivative by summing directional exchanges
between neighboring degrees of freedom.

## Relation to finite volume method

For a reader used to finite-volume methods, it is helpful to translate the SPH
objects into more familiar numerical language.

The analogy is not exact, but it is often very productive:

- a particle plays a role similar to a moving control volume
- the particle mass $m_a$ is the conserved amount carried by that control
  volume
- the particle volume $V_a = m_a / \rho_a$ plays the role of a cell volume
- a neighboring pair $(a,b)$ acts like a face-based interaction channel between
  two adjacent cells
- the kernel gradient $\nabla W_{ab}$ provides the directional weighting that a
  face normal and face area would provide in a mesh method

From that point of view, the SPH summations written earlier should not be read
as mysterious particle magic. They are discrete balance laws written on a
moving cloud rather than on a fixed mesh.

One may make this analogy explicit at the level of representative formulas.

### Cell averages versus particle averages

For example, the density sum

$$
\rho_a = \sum_b m_b W_{ab}
$$

can be viewed as a kernel-weighted local volume average. In a mesh code one
would recover cell density from mass and control-volume geometry; in SPH one
instead reconstructs the local particle volume through the kernel support.

The finite-volume counterpart is the cell-average relation

$$
\rho_i = \frac{m_i}{V_i},
$$

or, more generally, the volume average

$$
\rho_i \approx \frac{1}{V_i} \int_{V_i} \rho(\vec{x}) \, dV.
$$

The two formulas are built differently, but their role is analogous: both
associate a local average density with a discrete carrier of conserved mass.

### First-order gradients and flux balances

The same analogy becomes even stronger for differential operators. In a
finite-volume method, gradients and divergences are usually assembled from
fluxes across cell faces. In SPH, the pair $(a,b)$ replaces the geometric face,
and the interaction weight $\nabla W_{ab}$ replaces the geometric face measure.
The operator

$$
\nabla_1 f_a = \sum_b f_{ba} \nabla W_{ab} V_b
$$

therefore plays a role very similar to a face-based discrete gradient built
from differences between neighboring cell values.

An analogous finite-volume expression is the Gauss-type gradient

$$
\nabla f_i \approx \frac{1}{V_i} \sum_{f \in \partial V_i} f_f \vec{n}_f A_f,
$$

while the divergence of a vector field is written as

$$
\nabla \cdot \vec{F}_i \approx \frac{1}{V_i}
\sum_{f \in \partial V_i} \vec{F}_f \cdot \vec{n}_f A_f.
$$

These formulas show the same structural pattern as SPH: a discrete derivative
is formed by summing neighbor exchanges weighted by local geometry.

### Renormalization and least-squares reconstruction

The renormalized operator

$$
\nabla_2 f_a = L_a \sum_b f_{ba} \nabla W_{ab} V_b
$$

is even closer in spirit to techniques already familiar in unstructured-grid
CFD. The matrix

$$
L_a =
\left( \sum_b \vec{r}_{ab} \otimes \nabla W_{ab} V_b \right)^{-1}
$$

acts as a local correction for the anisotropy and irregularity of the particle
cloud. Its purpose is the same as in least-squares or gradient-reconstruction
procedures on unstructured meshes:

- compensate for uneven neighbor geometry
- recover first-order consistency for linear fields
- improve the quality of gradients used later in fluxes or source terms

So while the notation looks different, the underlying numerical instinct is
very familiar. A least-squares gradient on an irregular mesh and a
renormalized SPH gradient on an irregular particle cloud are both trying to
answer the same question: how can one recover a reliable local derivative from
neighboring samples when the geometry is not perfectly symmetric?

This analogy can be written explicitly through the standard least-squares
reconstruction problem

$$
\nabla f_i
=
\operatorname*{arg\,min}_{\vec{g}}
\sum_{j \in N(i)}
\omega_{ij}
\left[
f_j - f_i - \vec{g} \cdot (\vec{x}_j - \vec{x}_i)
\right]^2.
$$

The least-squares gradient and the renormalized SPH gradient are not
algebraically identical, but they serve the same purpose: they correct the raw
neighbor-difference operator by accounting for local geometric imbalance.

### Second-order diffusion terms

The same correspondence appears for diffusive operators. The SPH Laplacian
$\eqref{sph-laplacian}$ may be compared with the finite-volume diffusion form

$$
\nabla \cdot (\Gamma \nabla f)_i
\approx
\frac{1}{V_i}
\sum_{f \in \partial V_i}
\Gamma_f (\nabla f)_f \cdot \vec{n}_f A_f.
$$

In both cases, one approximates a second-order continuum operator by summing
local diffusive exchanges with neighboring degrees of freedom.

This viewpoint is useful for the rest of the guide. The later fluid equations
can be read as conservation laws written over moving particle volumes, with
pairwise exchanges standing in for face fluxes and with renormalization playing
a role analogous to geometric reconstruction on an unstructured mesh.
