---
title: Laplacian
---

Second-order differential operators are needed for viscous diffusion, thermal
conduction, and the pressure Poisson equation.

The formulation used in BlueTit Solver is based on a finite-difference-inspired
approximation of the second derivative, which was originally proposed by
Brookshaw [1] and later popularized for fluid SPH by Monaghan [2] and by Morris
et al. [3], who applied it to low-Reynolds-number incompressible flow. Its main
advantage is that it requires only the _first_ derivative of the kernel,
avoiding the noise amplification that comes from differentiating the kernel a
second time.

<!----------------------------------------------------------------------------->

## Laplacian operator

The Laplacian-like operator $\nabla \cdot (k \nabla f)$ at a particle $a$ is
discretized as:

$$
\mathrm{L}\{k, f\}_a =
  \frac{2}{\gamma_a}
  \left[
    \sum_{b \in \mathcal{P}}
      \frac{m_b}{\rho_b}
      \frac{\bar{k}_{ab} f_{ab} \vec{r}_{ab}}{|\vec{r}_{ab}|^2 + \eta^2} \cdot
      \nabla W_{ab} -
    \sum_{s \in \mathcal{S}}
      \widetilde{\left[ k \frac{\partial f}{\partial n} \right]}_{as}
      |\nabla\gamma_{as}|
  \right],
$$

where $\widetilde{\left[ k \frac{\partial f}{\partial n} \right]}_{as}$ is the
face flux determined by the boundary condition [4] and
$\eta \approx 0.01 h$ is a stabilization parameter.

The coefficient $\bar{k}_{ab}$ is the harmonic mean of $k_a$ and $k_b$:

$$
\bar{k}_{ab} = \frac{2 k_a k_b}{k_a + k_b}.
$$

This choice ensures that the flux is correctly represented when the material
property $k$ jumps across particles — for example, at a fluid-solid interface
or a free surface. The harmonic mean $\bar{k}_{ab}$ guarantees flux continuity
when $k$ is discontinuous.

<!----------------------------------------------------------------------------->

## References

1. L. Brookshaw, "A method of calculating radiative heat diffusion in particle
   simulations", _Proceedings of the Astronomical Society of Australia_,
   vol. 6, no. 2, pp. 207–210, 1985.

2. J. J. Monaghan, "Smoothed particle hydrodynamics",
   _Reports on Progress in Physics_, vol. 68, no. 8, pp. 1703–1759, 2005.

3. J. P. Morris, P. J. Fox, and Y. Zhu, "Modeling low Reynolds number
   incompressible flows using SPH",
   _Journal of Computational Physics_, vol. 136, no. 1, pp. 214–226, 1997.

4. M. Ferrand, D. R. Laurence, B. D. Rogers, D. Violeau, and C. Kassiotis,
   "Unified semi-analytical wall boundary conditions for inviscid, laminar or
   turbulent flows in the meshless SPH method",
   _International Journal for Numerical Methods in Fluids_,
   vol. 71, no. 4, pp. 446–472, 2013.
