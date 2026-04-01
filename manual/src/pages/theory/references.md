---
layout: "~/layouts/page.astro"
---

# References

This page collects article links for the concepts used in the fluid theory
guide. The list is intentionally selective: it is not a full SPH bibliography,
but a map from the documented methods to the papers where those ideas were
introduced or canonically described.

## SPH foundations and kernels

- L. B. Lucy, [Numerical approach to the testing of the fission hypothesis](https://www.osti.gov/biblio/7077277) (1977).
  One of the two original papers introducing what later became SPH.
- R. A. Gingold and J. J. Monaghan, [Smoothed particle hydrodynamics: theory and application to non-spherical stars](https://academic.oup.com/mnras/article-abstract/181/3/375/988212) (1977).
  The companion foundational SPH paper.
- J. J. Monaghan, [Smoothed Particle Hydrodynamics](https://doi.org/10.1146/annurev.aa.30.090192.002551) (1992).
  Canonical review for kernel interpolation, pairwise SPH operators, and artificial viscosity.
- H. Wendland, [Piecewise Polynomial, Positive Definite and Compactly Supported Radial Functions of Minimal Degree](https://doi.org/10.1007/BF02123482) (1995).
  Canonical source for the Wendland kernel family used in the solver.

## Weakly compressible free-surface SPH

- J. J. Monaghan, [Simulating Free Surface Flows with SPH](https://www.sciencedirect.com/science/article/pii/S0021999184710345) (1994).
  Classic free-surface WCSPH formulation and the standard dam-break-style context.
- J. P. Hughes and D. I. Graham, [Comparison of incompressible and weakly-compressible SPH models for free-surface water flows](https://doi.org/10.1080/00221686.2010.9641251) (2010).
  Source for the Hughes-Graham boundary-density correction used with Tait-type free-surface models.

## Density diffusion and artificial viscosity

- J. J. Monaghan, [Smoothed Particle Hydrodynamics](https://doi.org/10.1146/annurev.aa.30.090192.002551) (1992).
  Canonical reference for the $\alpha$-$\beta$ artificial viscosity idea.
- A. Colagrossi and M. Landrini, [Free-surface flows solved by means of SPH schemes with numerical diffusive terms](https://www.sciencedirect.com/science/article/pii/S0010465509003506) (2010).
  Canonical reference for density-diffusion ideas in WCSPH and the historical background of the Molteni-Colagrossi style diffusive term.
- S. Marrone, A. Colagrossi, D. Le Touzé, and G. Graziani, [δ-SPH model for simulating violent impact flows](https://www.researchgate.net/publication/241077909_-SPH_model_for_simulating_violent_impact_flows) (2011).
  Primary reference for the $\delta$-SPH density-diffusion formulation used by the standard executable branch.

## Riemann SPH and reconstruction

- C. Zhang, X. Hu, and N. A. Adams, [A weakly compressible SPH method based on a low-dissipation Riemann solver](https://www.sciencedirect.com/science/article/pii/S0021999117300438) (2017).
  Direct reference for the low-dissipation particle-pair Riemann formulation.
- G.-S. Jiang and C.-W. Shu, [Efficient Implementation of Weighted ENO Schemes](https://doi.org/10.1006/jcph.1996.0130) (1996).
  Canonical reference for the WENO reconstruction framework.

## Time integration and solver numerics

- S. Gottlieb, C.-W. Shu, and E. Tadmor, [Strong stability-preserving high-order time discretization methods](https://doi.org/10.1137/S003614450036757X) (2001).
  Canonical SSP reference for the SSPRK(3,3) time integrator discussed in the numerics page.
- J. J. Monaghan, [Smoothed Particle Hydrodynamics](https://doi.org/10.1146/annurev.aa.30.090192.002551) (1992).
  Also the standard reference for explicit SPH time marching and artificial viscosity context.

## How these references map to the guide

- [SPH Framework](./sph.html):
  Lucy (1977), Gingold & Monaghan (1977), Monaghan (1992), Wendland (1995)
- [Fluid Equations](./fluid-equations.html):
  Monaghan (1994), Hughes & Graham (2010), Colagrossi & Landrini (2010), Marrone et al. (2011)
- [Riemann Formulation](./riemann-formulation.html):
  Zhang, Hu & Adams (2017), Jiang & Shu (1996)
- [Numerics](./numerics.html):
  Gottlieb, Shu & Tadmor (2001), together with standard SPH numerics context
