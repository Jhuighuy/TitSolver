---
title: Interpolation
---

## Continuous formulation

Let $f : \mathbb{R}^d \to \mathbb{R}$ be a scalar field defined over a domain
$\Omega \subseteq \mathbb{R}^d$. The kernel approximation of $f$ is defined as:

$$
\langle f(\vec{r}) \rangle =
  \int\limits_{\mathbb{R}^d} f(\vec{r}') W(\vec{r} - \vec{r}', h) d\vec{r}',
$$

where $W$ is a smoothing kernel with smoothing length $h$ and compact support,
and satisfies:

$$
\int\limits_{\mathbb{R}^d} W(\vec{r}, h) d\vec{r} = 1,
\qquad
\lim_{h \to 0} W(\vec{r}, h) = \delta(\vec{r}).
$$

For a field that is smooth on the scale of $h$, the kernel approximation is
second-order accurate:

$$
\langle f(\vec{r}) \rangle = f(\vec{r}) + O(h^2).
$$

<!----------------------------------------------------------------------------->

## Boundary truncation

When the kernel support is truncated by a boundary, the SPH integral
representation is no longer consistent. A systematic fix, introduced by
Kulasegaram [1] and later popularized by Ferrand et al. [2], is to normalize the
kernel integral by the volume of the support that actually lies inside the
domain:

$$
\gamma(\vec{r}) =
  \int\limits_{\Omega} W(\vec{r} - \vec{r}', h) d\vec{r}'.
$$

The corrected SPH interpolation is then

$$
\langle f(\vec{r}) \rangle =
  \frac{1}{\gamma(\vec{r})}
  \int\limits_{\Omega} f(\vec{r}') W(\vec{r} - \vec{r}', h) d\vec{r}'.
$$

For interior particles $\gamma = 1$ and the classical formula is recovered;
near boundaries $\gamma < 1$ and the prefactor restores the ability to
reproduce constant fields exactly.

<!----------------------------------------------------------------------------->

## Particle sets

Following the semi-analytical wall-boundary notation used by Ferrand et al. [2]
and Mayrhofer et al. [3], the continuous domain is represented by two kinds of
particles and one kind of boundary element:

| Symbol                                       | Description                            |
| -------------------------------------------- | -------------------------------------- |
| $\mathcal{F}$                                | Fluid particles inside the domain      |
| $\mathcal{V}$                                | Boundary vertices carrying wall values |
| $\mathcal{P} = \mathcal{F} \cup \mathcal{V}$ | All particles used in kernel sums      |

Fluid particles sample the interior of $\Omega$. On a regular initial
distribution their control volumes are

$$
V_a \approx \Delta r^d.
$$

If the initial density is $\rho_0$, the corresponding initial mass is usually
chosen as

$$
m_a = \rho_0 V_a \approx \rho_0 \Delta r^d.
$$

Boundary vertices are not full fluid cells. They carry wall values for the
boundary-condition reconstruction, but their masses still appear through the
same volume weight $V_e = m_e / \rho_e$ in particle sums. The volume $V_e$ is
therefore chosen as the fraction of the local particle control volume that lies
inside the fluid domain. On a smooth, uniformly spaced wall this is a fixed
fraction of the interior particle volume; near corners, endpoints, or strongly
curved boundaries it depends on the adjacent boundary geometry.

Boundary faces $\mathcal{S}$ are separate from boundary vertices. They are used
for semi-analytical surface integrals, such as the boundary correction vector
introduced later. A face $s$ has an inner normal $\vec{n}_s$, pointing into the
fluid domain.

<!----------------------------------------------------------------------------->

## Discretization

With the particle set defined above, volume integrals are approximated by
particle sums. Each particle contributes with its volume
$V_b = m_b / \rho_b$.

The density at a particle $a$ is obtained by summing the kernel-weighted masses
of neighboring particles and normalizing by a correction factor
$\gamma_a = \gamma(\vec{r}_a)$ that accounts for boundary truncation:

$$
\rho_a =
  \frac{1}{\gamma_a}
  \sum_{b \in \mathcal{P}} m_b W_{ab},
$$

where $W_{ab} = W(\vec{r}_a - \vec{r}_b, h)$. The smoothing length $h$ sets the
width of the kernel. For a quasi-uniform particle distribution it is commonly
prescribed as

$$
h = \kappa \Delta r,\qquad \kappa \approx 2.
$$

Given the density, any scalar field $f$ can be interpolated at
an arbitrary point $\vec{r}$ as:

$$
f(\vec{r}) =
  \frac{1}{\gamma(\vec{r})}
  \sum_{b \in \mathcal{P}} \frac{m_b}{\rho_b} f_b W(\vec{r} - \vec{r}_b, h).
$$

The same formula evaluated at a particle $a$ gives:

$$
f_a =
  \frac{1}{\gamma_a}
  \sum_{b \in \mathcal{P}} \frac{m_b}{\rho_b} f_b W_{ab},
$$

where $\gamma_a = \gamma(\vec{r}_a)$.

<!----------------------------------------------------------------------------->

## References

1. S. Kulasegaram,
   "A corrected SPH approach for fluid–structure interaction",
   PhD thesis, Cardiff University, 1999.

2. M. Ferrand, D. R. Laurence, B. D. Rogers, D. Violeau, and C. Kassiotis,
   "Unified semi-analytical wall boundary conditions for inviscid, laminar or
   turbulent flows in the meshless SPH method",
   _International Journal for Numerical Methods in Fluids_,
   vol. 71, no. 4, pp. 446–472, 2013.

3. A. Mayrhofer, M. Ferrand, C. Kassiotis, D. Violeau, and F.-X. Morel,
   "Unified semi-analytical wall boundary conditions in SPH: analytical
   extension to 3-D", _Numerical Algorithms_, vol. 68, pp. 15–34, 2015.

<!----------------------------------------------------------------------------->
