.. ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
.. Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
.. Commercial use, including SaaS, requires a separate license, see /LICENSE.md
.. ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

SPH framework
=============

In this section we will present the basics of the
*Smooth Particle Hydrodynamics* (SPH). SPH is a numerical method for solving
various problems in fluid dynamics and solid mechanics.

Kernel function
---------------

Dirac's :math:`\delta`-function is defined with the integral identity:

.. math::
  :label: dirac-delta

  f(\vec{r}) =
  \int_{\mathbb{R}^{d}} f(\vec{r}') \delta(\vec{r} - \vec{r}') dV'.

Identity :eq:`dirac-delta` supposes to hold for all functions :math:`f`
in the space :math:`\mathbb{R}^{d}`. :math:`\delta` may be interpreted as
following:

.. math::

  \delta(\vec{r} - \vec{r}') = \begin{cases}
    \dfrac{1}{dV'}, & \text{if } \vec{r} = \vec{r}' \\
    0,              & \text{otherwise.}
  \end{cases}

Therefore, :math:`\delta` is a function that is zero everywhere except the
origin, where it is limits to reciprocal of the volume differential :math:`dV'`.
Replacing :math:`\delta` with its finite-valued approximation :math:`W` in
:eq:`dirac-delta` we obtain the smoothed function :math:`\langle f \rangle`:

.. math::
  :label: smoothed-function

  \langle f \rangle(\vec{r}) =
  \int_{\mathbb{R}^{d}} f(\vec{r}') W(\vec{r} - \vec{r}', h) dV'.

We will refer to :math:`W = W(\vec{r}, h)` in :eq:`smoothed-function` as smooth
*kernel* function with *radius* :math:`h`. Kernel is often defined in terms of
a radial function :math:`w = w(q)`:

.. math::

  W(\vec{r}, h) = \frac{\omega_{d}}{h^{d}} \, w(q),

where :math:`\omega_{d}` is a normalization constant and
:math:`q = |\vec{r}| / h`. |product| supports the following kernels:

.. list-table:: Smooth kernel functions
  :header-rows: 1

  * - Name
    - Radial function
    - Normalization constant

  * - Gaussian
    - .. math::

        \exp(-q^2)

    - .. math::

        \sqrt{\pi}^{-d}

  * - Cubic B-spline (M4)
    - .. math::


        \frac{1}{4} & (q - 2)^{3} \cdot \mathbb{I}[q \leq 2] \\
                  - & (q - 1)^{3} \cdot \mathbb{I}[q \leq 1]

    - .. math::

        \omega_{d} = \frac{2}{3}, \frac{10}{7\pi}, \frac{1}{\pi},

      in 1D, 2D, 3D respectively

  * - Quartic B-spline (M5)
    - .. math::

           & (q - 2.5)^{4} \cdot \mathbb{I}[q \leq 2.5] \\
        -5 & (q - 1.5)^{4} \cdot \mathbb{I}[q \leq 1.5] \\
        10 & (q - 0.5)^{4} \cdot \mathbb{I}[q \leq 0.5]

    - .. math::

        \frac{1}{24}, \frac{96}{1199\pi}, \frac{1}{20\pi},

      in 1D, 2D, 3D respectively

  * - Quintic B-spline (M6)
    - .. math::

           & (q - 3)^{5} \cdot \mathbb{I}[q \leq 3] \\
        -6 & (q - 2)^{5} \cdot \mathbb{I}[q \leq 2] \\
        15 & (q - 1)^{5} \cdot \mathbb{I}[q \leq 1]

    - .. math::

        \frac{1}{120}, \frac{7}{478\pi}, \frac{1}{120\pi},

      in 1D, 2D, 3D respectively

  * - Quartic Wendland (C2)
    - .. math::

        (1 + 2 q) \cdot
        \left( 1 - \frac{q}{2} \right)^{4} \cdot
        \mathbb{I}[q \leq 2]

    - .. math::

        \frac{3}{4}, \frac{7}{4\pi}, \frac{21}{16\pi},

      in 1D, 2D, 3D respectively

  * - Quintic Wendland (C4)
    - .. math::

        \left( 1 + 3 q + \frac{35}{12} q^{2} \right) \cdot
        \left( 1 - \frac{q}{2} \right)^{6} \cdot
        \mathbb{I}[q \leq 2]

    - .. math::

        \frac{27}{16}, \frac{9}{4\pi}, \frac{495}{256\pi},

      in 1D, 2D, 3D respectively

  * - 8-th order Wendland (C8)
    - .. math::

        \left( 1 + 4 q + \frac{25}{4} q^{2} + 4 q^{3} \right) \cdot
        \left( 1 - \frac{q}{2} \right)^{8} \cdot
        \mathbb{I}[q \leq 2]

    - .. math::

        \frac{15}{8}, \frac{39}{14\pi}, \frac{339}{128\pi},

      in 1D, 2D, 3D respectively


Kernel interpolation
--------------------

Given a set of points :math:`\{ \vec{r}_{a} \}` and a set of values
:math:`\{ f_{a} \}` associated with points, one can define smooth interpolation
operator based on approximation of the smoothed function
:eq:`smoothed-function`:

.. math::
  :label: sph-interpolation

  f(\vec{r}) = \sum_{b} f_{b} W(\vec{r} - \vec{r}_{b}, h) V_{b}.

Point volumes :math:`\{V_{a}\}` are defined such that the resulting
interpolation preserves given values in the interpolation points:

.. math::
  :label: sph-identity

  f_{a} = \sum_{b} f_{a} W(\vec{r}_{a} - \vec{r}_{b}, h) V_{b},

In SPH, the volumes :math:`\{V_{a}\}` are defined as:

.. math::
  :label: sph-volume

  V_{a} = \frac{m_{a}}{\rho_{a}},

where :math:`m_{a}` is an arbitrarily chosen *mass* of the point, and
:math:`\rho_{a}` is the *density* of the point, that is computed by
substituting it into the identity :eq:`sph-identity`:

.. math::
  :label: sph-density

  \rho_{a} = \sum_{b} m_{b} W(\vec{r}_{a} - \vec{r}_{b}, h).

.. note::

  In SPH literature, it is common to abbreviate difference like
  :math:`\vec{r}_{a} - \vec{r}_{b}` as :math:`\vec{r}_{ab}`,
  and the kernel function value :math:`W(\vec{r}_{ab}, h)` as :math:`W_{ab}`.
  With this notation, the equation :eq:`sph-density` can be written as:

  .. math::

    \rho_{a} = \sum_{b} m_{b} W_{ab}.

  :math:`\bar{\vec{r}}_{ab}` is the average of :math:`\vec{r}_{a}` and
  :math:`\vec{r}_{b}`. We will follow this notation in the rest of the manual.

Therefore, any set pairs of points and their associated function values
:math:`\{ ( \vec{r}_{a}, f_{a} ) \}` can be continuously interpolated by first
computing the density values :math:`\{ \rho_{a} \}` and volumes
:math:`\{ V_{a} \}` using :eq:`sph-density` and :eq:`sph-volume` for each
point, and then computing the interpolated function using
:eq:`sph-interpolation`.

First order SPH operators
-------------------------

Gradient of a interpolated function can be calculated by the following
equation:

.. math::
  :label: sph-interpolation-gradient

  \nabla f(\vec{r}) = \sum_{b} f_{b} \nabla W(\vec{r} - \vec{r}_{b}, h) V_{b}.

Discrete gradient operator can be defined by projecting the equation
:eq:`sph-interpolation-gradient` onto the set of points
:math:`\{ \vec{r}_{a} \}`:

.. math::
  :label: sph-gradient-0

  \nabla_{0} f_{a} = \sum_{b} f_{b} \nabla W_{ab} V_{b}.

It is easy to show that the gradient operator :eq:`sph-gradient-0`, computed
over the constant function does not vanish (subscript :math:`_{0}` stands for
zeroth order of polynomial approximation). To fix this, a simple correction
can be applied by subtracting the :math:`f_{a} \nabla 1_{a}` from
the gradient operator :eq:`sph-gradient-0`:

.. math::
  :label: sph-gradient-1

  \nabla_{1} f_{a} = \sum_{b} f_{ba} \nabla W_{ab} V_{b}.

Although the gradient operator :eq:`sph-gradient-1` vanishes for the constant
function, it fails to compute the gradient of the linear function exactly.
To fix this, the following correction is applied:

.. math::
  :label: sph-gradient-2

  \nabla_{2} f_{a} = L_{a} \sum_{b} f_{ba} \nabla W_{ab} V_{b},

where :math:`L_{a}` is the renormalization matrix, defined as:

.. math::
  :label: sph-renormalization-matrix

  L_{a} =
  \left( \sum_{b} \vec{r}_{ab} \otimes \nabla W_{ab} V_{b} \right)^{-1}.

Divergence and curl operators can be derived in a similar way. For example,
the divergence operators can be defined as:

.. math::
  :label: sph-divergence-0

  \nabla_{0} \cdot \vec{F}_{a} =
  \sum_{b} \vec{F}_{b} \cdot \nabla W_{ab} V_{b}.

.. math::
  :label: sph-divergence-1

  \nabla_{1} \cdot \vec{F}_{a} =
  \sum_{b} \vec{F}_{ba} \cdot \nabla W_{ab} V_{b}.

.. math::
  :label: sph-divergence-2

  \nabla_{2} \cdot \vec{F}_{a} =
  \sum_{b} \vec{F}_{ba} \cdot L_{a} \nabla W_{ab} V_{b}.
