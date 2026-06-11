---
title: Boundary Conditions
---

Boundary conditions close the differential operators defined in the previous
sections. They enter the SPH discretization through the face terms in the
operators — specifically through the field values $f_s$ on boundary faces and
the face flux
$\widetilde{\left[ k \frac{\partial f}{\partial n} \right]}_{as}$.

<!----------------------------------------------------------------------------->

## Vertex and face values

The boundary is represented by a mesh of vertices (indexed by $e$) and faces
(indexed by $s$). Each face is defined by a set of vertices
$\mathcal{E}(s)$. The discrete differential operators use values on faces, but
the wall data are first constructed at the boundary vertices and then averaged
onto faces.

### Dirichlet boundary

For a Dirichlet condition,

$$
f \Bigr|_{\partial\Omega} = \phi,
$$

the field value at each boundary vertex is taken directly from the prescribed
boundary data:

$$
f_e = \phi(\vec{r}_e).
$$

### Robin boundary

For a Robin or Neumann condition, the boundary value itself is not prescribed.
Instead it is extrapolated from nearby fluid particles so that the boundary
condition is satisfied at the wall. Following Mayrhofer et al. [2], this is
written as a local one-dimensional interpolation problem along the inward normal
$\vec{n}_e$ at boundary vertex $e$:

$$
\left(
  k \frac{\partial f}{\partial n} + \lambda f
\right) \Bigr|_{\partial\Omega} = \psi
$$

with $k \neq 0$ for the Robin extrapolation. At vertex $e$, the boundary
coefficients are evaluated at $\vec{r}_e$ and treated as constants for the local
fit. For fluid particle $b$, define the normal coordinate
$x_b = \vec{r}_{be} \cdot \vec{n}_e$. A polynomial $\tilde{f}(x)$ is fitted to
the nearby fluid values while enforcing the boundary condition at $x = 0$:

$$
\tilde{f}(x) =
  \sum_{i=2}^{m} \beta_i x^i +
  \frac{\psi}{k} x +
  \beta_1 \left( 1 - \frac{\lambda}{k} x \right).
$$

The wall value is the first coefficient, $f_e = \tilde{f}(0) = \beta_1$. The
coefficients are obtained from a kernel-weighted least-squares fit:

$$
\mathbf{X}^T \mathbf{\Lambda} \mathbf{X} \, [\beta] =
  \mathbf{X}^T \mathbf{\Lambda} [y],
$$

where

$$
X_{b j} =
  \begin{cases}
    1 - \frac{\lambda}{k} x_b, & j = 1, \\
    x_b^j,                     & j > 1,
  \end{cases}
\qquad
y_b = f_b - \frac{\psi}{k} x_b,
\qquad
\Lambda_{b c} = \delta_{b c} \frac{m_b}{\rho_b} W_{be}.
$$

For $m = 1$ and a homogeneous Neumann condition ($\lambda = 0$, $\psi = 0$),
this reduces to the normalized semi-analytical wall interpolation used by
Ferrand et al. [1]:

$$
\alpha_e =
  \sum_{b \in \mathcal{F}} \frac{m_b}{\rho_b} W_{be},
\qquad
f_e =
  \frac{1}{\alpha_e}
  \sum_{b \in \mathcal{F}} \frac{m_b}{\rho_b} f_b W_{be}.
$$

For pressure boundary conditions with a body force $\vec{g}$,

$$
\frac{\partial p}{\partial n} = \rho \vec{g} \cdot \vec{n},
$$

Mayrhofer et al. modify the fit so that only the normal variation is represented
by $\tilde{p}(x)$. The tangential hydrostatic contribution is subtracted from
the right-hand side:

$$
\vec{r}_{be}^{\,t} = \vec{r}_{be} - x_b \vec{n}_e,
\qquad
y_b =
  p_b -
  \rho_b \vec{r}_{be}^{\,t} \cdot \vec{g}.
$$

This is important near a wall/free-surface intersection. Without the tangential
correction, the particles used in the fit may all have positive hydrostatic
pressure even when the vertex itself lies on the free surface, which causes the
extrapolated wall pressure to be too large and produces a spurious repulsive
force.

### Face value

The field value at a face $s$ is obtained by averaging the vertex values:

$$
f_s =
  \frac{1}{|\mathcal{E}(s)|}
  \sum_{e \in \mathcal{E}(s)} f_e.
$$

<!----------------------------------------------------------------------------->

## Face flux

The face flux in the Laplacian operator is set according to the boundary
condition:

### Dirichlet boundary <!-- markdownlint-disable-line MD-0024 -->

Flux is approximated using a finite difference across the particle-face
distance:

$$
\widetilde{\left[ k \frac{\partial f}{\partial n} \right]}_{as} \approx
  \frac{\bar{k}_{as} f_{as} \vec{r}_{as}}{|\vec{r}_{as}|^2} \cdot
  \vec{n}_{s}.
$$

### Robin boundary <!-- markdownlint-disable-line MD-0024 -->

Flux is provided directly by the boundary condition:

$$
\widetilde{\left[ k \frac{\partial f}{\partial n} \right]}_{as} =
  \psi(\vec{r}_s) - \lambda(\vec{r}_s) f_s.
$$

<!----------------------------------------------------------------------------->

## References

1. M. Ferrand, D. R. Laurence, B. D. Rogers, D. Violeau, and C. Kassiotis,
   "Unified semi-analytical wall boundary conditions for inviscid, laminar or
   turbulent flows in the meshless SPH method",
   _International Journal for Numerical Methods in Fluids_,
   vol. 71, no. 4, pp. 446–472, 2013.

2. A. Mayrhofer, B. D. Rogers, D. Violeau, and M. Ferrand,
   "Investigation of wall bounded flows using SPH and the unified
   semi-analytical wall boundary conditions", _Computer Physics Communications_,
   vol. 184, no. 11, pp. 2515–2527, 2013.
