---
title: Kernel Functions
---

The kernel $W$ is the heart of the SPH method. It determines the smoothing
characteristics and has a direct impact on accuracy, stability, and
computational cost.

Any kernel used in SPH must satisfy the following properties:

- **Positivity**: $W(\vec{r}, h) \geq 0$.
- **Normalization**: $\displaystyle\int_{\mathbb{R}^d} W(\vec{r}, h) d\vec{r} = 1$.
- **Delta property**: $\displaystyle\lim_{h \to 0} W(\vec{r}, h) = \delta(\vec{r})$.
- **Compact support**: $W(\vec{r}, h) = 0$ for $|\vec{r}| \geq R h$, where $R$
  is the kernel radius.

<!----------------------------------------------------------------------------->

## General form

Kernels are typically written in the radially symmetric form:

$$
W(\vec{r}, h) =
  \frac{\omega_{d}}{h^{d}} w\left( \frac{|\vec{r}|}{h} \right),
$$

where $w(q)$ is the dimensionless kernel function and $\omega_{d}$ is a
normalization constant that depends on the spatial dimension $d$:

$$
\omega_{d} =
  \left[
    S_d \int_{0}^{\infty} q^{d-1} w(q) dq
  \right]^{-1},
\qquad
S_d = \frac{2\pi^{d/2}}{\Gamma(d/2)}.
$$

Here $S_d$ is the surface area of the unit sphere in $\mathbb{R}^d$
($S_1 = 2$, $S_2 = 2\pi$, and $S_3 = 4\pi$).

The kernel gradient follows from the chain rule:

$$
\nabla W(\vec{r}, h) =
  \frac{\omega_{d}}{h^{d+1}}
    \frac{\vec{r}}{|\vec{r}|}
    \frac{d w}{d q}\left( \frac{|\vec{r}|}{h} \right).
$$

<!----------------------------------------------------------------------------->

## Kernel variants

The following kernels are available in BlueTit Solver.

### M4 Spline

A widely used, computationally cheap kernel:

$$
w(q) =
  \begin{cases}
    \frac{1}{4} (2 - q)^3 + (1 - q)^3, & 0 \leq q < 1, \\
    \frac{1}{4} (2 - q)^3,             & 1 \leq q < 2, \\
    0,                                 & q \geq 2.
  \end{cases}
$$

### M5 Spline

Smoother and more stable, at a slightly higher cost:

$$
w(q) =
  \begin{cases}
    (\frac{5}{2} - q)^4
      - 5 (\frac{3}{2} - q)^4
      + 10 (\frac{1}{2} - q)^4, & 0 \leq q < \frac{1}{2}, \\
    (\frac{5}{2} - q)^4
      - 5 (\frac{3}{2} - q)^4,  & \frac{1}{2} \leq q < \frac{3}{2}, \\
    (\frac{5}{2} - q)^4,        & \frac{3}{2} \leq q < \frac{5}{2}, \\
    0,                          & q \geq \frac{5}{2}.
  \end{cases}
$$

### M6 Spline

Higher-order continuity at an increased cost:

$$
w(q) =
  \begin{cases}
    (3 - q)^5 - 6 (2 - q)^5 + 15 (1 - q)^5, & 0 \leq q < 1, \\
    (3 - q)^5 - 6 (2 - q)^5,                & 1 \leq q < 2, \\
    (3 - q)^5,                              & 2 \leq q < 3, \\
    0,                                      & q \geq 3.
  \end{cases}
$$

### C2 Wendland

Wendland kernels are guaranteed positive-definite and exhibit good stability:

$$
w(q) =
  \begin{cases}
    (1 + 2 q) \left(1 - \frac{1}{2} q\right)^4, & q < 2, \\
    0,                                          & q \geq 2.
  \end{cases}
$$

### C4 Wendland

A smoother variant with higher-order continuity:

$$
w(q) =
  \begin{cases}
    \left(1 + 3 q + \frac{35}{12} q^2\right) \left(1 - \frac{1}{2} q\right)^6, & q < 2, \\
    0,                                                                         & q \geq 2.
  \end{cases}
$$

### C6 Wendland

A yet smoother variant:

$$
w(q) =
  \begin{cases}
    \left(1 + 4 q + \frac{25}{4} q^2 + 4 q^3\right) \left(1 - \frac{1}{2} q\right)^8, & q < 2, \\
    0,                                                                                   & q \geq 2.
  \end{cases}
$$

<!----------------------------------------------------------------------------->

## Normalization constants

The table below lists the normalization constants needed for the
one-, two-, and three-dimensional implementations:

| Kernel      | $R$   | $\omega_{1}$    | $\omega_{2}$         | $\omega_{3}$          |
| ----------- | ----- | --------------- | -------------------- | --------------------- |
| M4 Spline   | $2$   | $\frac{2}{3}$   | $\frac{10}{7\pi}$    | $\frac{1}{\pi}$       |
| M5 Spline   | $2.5$ | $\frac{1}{24}$  | $\frac{96}{1199\pi}$ | $\frac{1}{20\pi}$     |
| M6 Spline   | $3$   | $\frac{1}{120}$ | $\frac{7}{478\pi}$   | $\frac{1}{120\pi}$    |
| C2 Wendland | $2$   | $\frac{3}{4}$   | $\frac{7}{4\pi}$     | $\frac{21}{16\pi}$    |
| C4 Wendland | $2$   | $\frac{27}{32}$ | $\frac{9}{4\pi}$     | $\frac{495}{256\pi}$  |
| C6 Wendland | $2$   | $\frac{15}{16}$ | $\frac{39}{14\pi}$   | $\frac{1365}{512\pi}$ |

<!----------------------------------------------------------------------------->

## Properties of the spline and Wendland families

The kernels available in BlueTit Solver fall into two families, each with
distinct characteristics.

### B-spline family

M4, M5, and M6 splines are piecewise polynomials that coincide with uniform
B-splines. They are computationally cheap and have been the de facto standard
in SPH since the 1990s [1].

The main disadvantage of B-spline kernels is that they are not
positive-definite in all dimensions — their Fourier transform can take negative
values for certain wave numbers [2]. This sign change can excite a long-known
instability in which particles clump into pairs while the SPH equations report
a vanishingly small error [3]. The effect is most pronounced with the M4 spline.
Raising the polynomial order suppresses the negative tail but increases the
support radius, raising the computational cost per particle.

### Wendland family

C2, C4, and C6 Wendland kernels [4] are piecewise polynomials that are _compactly
supported and positive-definite by construction_ — their Fourier transform is
everywhere non-negative. This eliminates the pairing instability and makes them
the preferred choice for modern SPH codes [2,3]. They are the default kernel
family in BlueTit Solver.

The downside is a slightly higher operation count per evaluation compared with
a B-spline of the same support radius, and a marginally larger leading error
coefficient in the kernel approximation for regular particle distributions [5].
In practice the stability benefit far outweighs these costs, especially for
long-duration free-surface simulations.

<!----------------------------------------------------------------------------->

## References

1. J. J. Monaghan, "Smoothed particle hydrodynamics",
   _Annual Review of Astronomy and Astrophysics_, vol. 30, pp. 543–574, 1992.

2. W. Dehnen and H. Aly,
   "Improving convergence in smoothed particle hydrodynamics simulations without
   pairing instability",
   _Monthly Notices of the Royal Astronomical Society_,
   vol. 425, no. 2, pp. 1068–1082, 2012.

3. D. J. Price, "Smoothed particle hydrodynamics — a review",
   _Journal of Computational Physics_, vol. 231, no. 3, pp. 759–794, 2012.

4. H. Wendland,
   "Piecewise polynomial, positive definite and compactly supported radial
   functions of minimal degree",
   _Advances in Computational Mathematics_, vol. 4, pp. 389–396, 1995.

5. M. B. Liu and G. R. Liu,
   "Smoothed particle hydrodynamics (SPH): an overview and recent developments",
   _Archives of Computational Methods in Engineering_, vol. 17, pp. 25–76, 2010.
