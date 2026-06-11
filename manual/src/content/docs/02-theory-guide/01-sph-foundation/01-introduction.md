---
title: Introduction
---

SPH is a meshless Lagrangian method for discretizing the equations of fluid
dynamics. Instead of a fixed Eulerian grid, the fluid is represented by a set
of particles that carry field quantities (density, velocity, pressure) and move
with the flow. The method was originally developed for astrophysical
simulations by Lucy (1977) [1] and Gingold & Monaghan (1977) [2], and has since
been adapted to a wide range of problems, including free-surface flows,
multiphase flows, and fluid–structure interaction.

The defining idea of SPH is that any continuous field can be expressed as an
integral convolution with a smoothing kernel, and that this integral can be
approximated by a discrete sum over nearby particles. This avoids the need for
a connectivity mesh and makes the method naturally suited to problems with
large deformations, fragmentation, or moving boundaries.

<!----------------------------------------------------------------------------->

## Notation and conventions

The particle and boundary sets are introduced on the interpolation page, where
the discrete geometry is first needed. The table below lists the shorthand used
throughout the chapter.

| Symbol                                      | Description                         |
| ------------------------------------------- | ----------------------------------- |
| $a, b$                                      | Particle indices                    |
| $e$                                         | Boundary vertex index               |
| $s$                                         | Boundary face index                 |
| $\vec{r}_a$                                 | Position of particle $a$            |
| $m_a$                                       | Mass of particle $a$                |
| $\rho_a$                                    | Density of particle $a$             |
| $f_a$                                       | Field value at particle $a$         |
| $\vec{r}_{ab} = \vec{r}_a - \vec{r}_b$      | Relative position                   |
| $f_{ab} = f_a - f_b$                        | Difference notation                 |
| $h$                                         | Smoothing length                    |
| $W_{ab} = W(\vec{r}_{ab}, h)$               | Kernel function                     |
| $\nabla W_{ab} = \nabla W(\vec{r}_{ab}, h)$ | Kernel gradient                     |
| $\gamma_a$                                  | Boundary correction factor          |
| $\nabla\gamma_{as}$                         | Boundary correction from face $s$   |
| $\vec{n}_s$                                 | Inner normal of boundary face $s$   |
| $\vec{n}_e$                                 | Inner normal at boundary vertex $e$ |

<!----------------------------------------------------------------------------->

## References

1. L. B. Lucy, "A numerical approach to the testing of the fission hypothesis",
   _The Astronomical Journal_, vol. 82, pp. 1013–1024, 1977.

2. R. A. Gingold and J. J. Monaghan, "Smoothed particle hydrodynamics: theory
   and application to non-spherical stars",
   _Monthly Notices of the Royal Astronomical Society_,
   vol. 181, pp. 375–389, 1977.

<!----------------------------------------------------------------------------->
