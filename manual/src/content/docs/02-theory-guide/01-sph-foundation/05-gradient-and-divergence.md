---
title: Gradient and Divergence
---

## Continuous formulation

The gradient of $f$ can be approximated by differentiating the kernel:

$$
\langle \nabla f(\vec{r}) \rangle =
  \int\limits_{\mathbb{R}^d} f(\vec{r}')
  \nabla W(\vec{r} - \vec{r}', h) d\vec{r}',
$$

which follows from integration by parts.

Near boundaries, the kernel support is truncated and a boundary correction
is required. The corrected gradient operator is:

$$
\langle \nabla f(\vec{r}) \rangle =
  \frac{1}{\gamma(\vec{r})}
  \left[
    \int\limits_{\Omega}
      f(\vec{r}')
      \nabla W(\vec{r} - \vec{r}', h)
      d\vec{r}' -
    \int\limits_{\partial\Omega}
      f(\vec{r}')
      W(\vec{r} - \vec{r}', h)
      \vec{n}(\vec{r}')
      d\vec{r}'
  \right].
$$

The discrete form of this operator is developed below.

<!----------------------------------------------------------------------------->

## Zeroth-order consistent gradient

The simplest gradient of a scalar field $f$ at a particle $a$ is [1,2,3]:

$$
\mathrm{G}^0\{f\}_a =
  \frac{1}{\gamma_a}
  \left[
    \sum_{b \in \mathcal{P}} \frac{m_b}{\rho_b} f_b \nabla W_{ab} -
    \sum_{s \in \mathcal{S}} f_s \nabla\gamma_{as}
  \right],
$$

where $f_s$ is the field value on face $s$, determined by the boundary
condition (see Boundary Conditions). The operator is not exact for constant
fields:

$$
\mathrm{G}^0\{1\}_a \not\equiv 0,
$$

which motivates the first-order correction.

<!----------------------------------------------------------------------------->

## First-order consistent gradient

The operator $\mathrm{G}^1$ is obtained by subtracting the error of
$\mathrm{G}^0$ on a constant field from the result:

$$
\mathrm{G}^1\{f\}_a = \mathrm{G}^0\{f\}_a - f_a \mathrm{G}^0\{1\}_a.
$$

Using the difference notation $f_{ba} = f_b - f_a$, this can be written as:

$$
\mathrm{G}^1\{f\}_a =
  \frac{1}{\gamma_a}
  \left[
    \sum_{b \in \mathcal{P}} \frac{m_b}{\rho_b} f_{ba} \nabla W_{ab} -
    \sum_{s \in \mathcal{S}} f_{sa} \nabla\gamma_{as}
  \right].
$$

For a vector field $\vec{u}$, the gradient is a second-order tensor:

$$
\mathrm{G}^1\{\vec{u}\}_a =
  \frac{1}{\gamma_a}
  \left[
    \sum_{b \in \mathcal{P}}
      \frac{m_b}{\rho_b} \vec{u}_{ba} \otimes \nabla W_{ab} -
    \sum_{s \in \mathcal{S}}
      \vec{u}_{sa} \otimes \nabla\gamma_{as}
  \right]^T.
$$

<!----------------------------------------------------------------------------->

## Second-order consistent gradient

Operator $\mathrm{G}^1$ is first-order consistent: it reproduces constant
fields exactly but is not exact for linear fields. The second-order operator
is obtained by multiplying with the inverse of the $\mathrm{G}^1$ gradient of
the position field:

$$
\mathrm{G}^2\{f\}_a =
  \left[ \mathrm{G}^1\{\vec{r}\}_a \right]^{-1} \mathrm{G}^1\{f\}_a,
$$

$$
\mathrm{G}^2\{\vec{u}\}_a =
  \mathrm{G}^1\{\vec{u}\}_a \left[ \mathrm{G}^1\{\vec{r}\}_a \right]^{-T}.
$$

The operator $\mathrm{G}^2$ reproduces linear fields exactly.

<!----------------------------------------------------------------------------->

## Divergence operators

The divergence of a vector field $\vec{u}$ is obtained as the trace of the
gradient tensor. For the first-order consistent gradient this gives:

$$
\mathrm{D}^1\{\vec{u}\}_a =
  \frac{1}{\gamma_a}
  \left[
    \sum_{b \in \mathcal{P}}
      \frac{m_b}{\rho_b}
      \vec{u}_{ba} \cdot \nabla W_{ab} -
    \sum_{s \in \mathcal{S}}
      \vec{u}_{sa} \cdot \nabla\gamma_{as}
  \right].
$$

The second-order divergence follows from the definition of $\mathrm{G}^2$:

$$
\mathrm{D}^2\{\vec{u}\}_a =
  \operatorname{tr}
  \left(
    \mathrm{G}^2\{\vec{u}\}_a
  \right)
  =
  \operatorname{tr}
  \left(
    \mathrm{G}^1\{\vec{u}\}_a
    \left[ \mathrm{G}^1\{\vec{r}\}_a \right]^{-T}
  \right).
$$

These operators inherit the consistency properties of their gradient
counterparts: $\mathrm{D}^1$ is exact for constant vector fields and
$\mathrm{D}^2$ is exact for linear vector fields.

<!----------------------------------------------------------------------------->

## Conservative variants

Applying the product-rule identity

$$
\nabla f = \frac{1}{\rho^k} \nabla(\rho^k f) - \frac{k}{\rho} f \nabla\rho
$$

to the $\mathrm{G}^0$ operator gives a family of density-weighted gradients
and divergences:

$$
\begin{aligned}
\mathrm{G}_k^0\{f\}_a &=
  \frac{1}{\rho_a^k} \mathrm{G}^0\{\rho^k f\}_a -
  \frac{k}{\rho_a} f_a \mathrm{G}^0\{\rho\}_a, \\[6pt]
\mathrm{D}_k^0\{\vec{u}\}_a &=
  \frac{1}{\rho_a^k} \mathrm{D}^0\{\rho^k \vec{u}\}_a -
  \frac{k}{\rho_a} \vec{u}_a \cdot \mathrm{G}^0\{\rho\}_a.
\end{aligned}
$$

These operators use the zeroth-order base operators, so they are zeroth-order
consistent, but the density weighting changes their pairwise structure and
conservation properties. Here $\mathrm{D}^0$ is the trace of the
$\mathrm{G}^0$ operator applied to a vector field.

### Divergence for the continuity equation

$$
\mathrm{D}_1^0\{\vec{u}\}_a =
  \frac{1}{\gamma_a \rho_a}
  \left[
    \sum_{b \in \mathcal{P}} m_b \vec{u}_{ba} \cdot \nabla W_{ab} -
    \sum_{s \in \mathcal{S}} \rho_s \vec{u}_{sa} \cdot \nabla\gamma_{as}
  \right].
$$

This is the divergence that appears in the continuity equation
$d\rho_a/dt = -\rho_a \mathrm{D}_1^0\{\vec{v}\}_a$.

### Gradient for the momentum equation

$$
\mathrm{G}_{-1}^0\{f\}_a =
  \frac{\rho_a}{\gamma_a}
  \left[
    \sum_{b \in \mathcal{P}}
      m_b
      \left( \frac{f_b}{\rho_b^2} + \frac{f_a}{\rho_a^2} \right)
      \nabla W_{ab} -
    \sum_{s \in \mathcal{S}}
      \rho_s
      \left( \frac{f_s}{\rho_s^2} + \frac{f_a}{\rho_a^2} \right)
      \nabla\gamma_{as}
  \right].
$$

This is the gradient that appears in the momentum equation
$d\vec{v}_a/dt = -\mathrm{G}_{-1}^0\{p\}_a / \rho_a$.

### Adjoint relationship

With boundary terms neglected, the gradient operator $\mathrm{G}_{-1}^0$ and
the divergence operator $\mathrm{D}_1^0$ are related by

$$
\langle \mathrm{G}_{-1}^0\{f\}, \vec{u} \rangle =
  -\langle f, \mathrm{D}_1^0\{\vec{u}\} \rangle.
$$

Three properties make these operators fully conservative:

1. **Mass conservation** — the continuity equation
   $d\rho_a/dt = -\rho_a \mathrm{D}_1^0\{\vec{v}\}_a$ can be directly derived
   by differentiating the density sum $\rho_a = \sum m_b W_{ab}$ with respect
   to time.

2. **Momentum conservation** — the pairwise contribution between particles
   $a$ and $b$ involves the term $m_a m_b (p_a/\rho_a^2 + p_b/\rho_b^2)$,
   which is symmetric in $a$ and $b$. Since $\nabla W_{ab} = -\nabla W_{ba}$,
   the force on $a$ from $b$ is equal and opposite to the force on $b$
   from $a$.

3. **Energy conservation** — the adjoint relationship
   $\langle \mathrm{G}_{-1}^0\{p\}, \vec{v} \rangle =
   -\langle p, \mathrm{D}_1^0\{\vec{v}\} \rangle$ implies that the work
   done by the pressure forces equals the rate of change of internal
   energy. When $\mathrm{G}_{-1}^0$ drives the momentum equation and
   $\mathrm{D}_1^0$ drives the continuity equation, the discrete energy
   balance mirrors the continuous one exactly for interior particles.

<!----------------------------------------------------------------------------->

## References

1. J. J. Monaghan, "Smoothed particle hydrodynamics",
   _Reports on Progress in Physics_, vol. 68, no. 8, pp. 1703–1759, 2005.

2. M. Ferrand, D. R. Laurence, B. D. Rogers, D. Violeau, and C. Kassiotis,
   "Unified semi-analytical wall boundary conditions for inviscid, laminar or
   turbulent flows in the meshless SPH method",
   _International Journal for Numerical Methods in Fluids_,
   vol. 71, no. 4, pp. 446–472, 2013.

3. D. Violeau, _Fluid Mechanics and the SPH Method: Theory and Applications_,
   Oxford University Press, 2012.
