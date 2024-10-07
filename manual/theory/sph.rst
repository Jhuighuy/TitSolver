.. ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
.. Part of the Tit Solver project, under the MIT License.
.. See /LICENSE.md for license information. SPDX-License-Identifier: MIT
.. ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. include:: ../shared.rst

.. ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

SPH framework
=============

Kernel function
---------------

Dirac's :math:`\delta`-function is defined with the following integral:

.. math::
  :label: dirac-delta

  f(\vec{r}) =
  \int_{\mathbb{R}^{d}} f(\vec{r}') \delta(\vec{r} - \vec{r}') dV'.

Equation :eq:`dirac-delta` supposes to hold for all functions :math:`f`
in the space :math:`\mathbb{R}^{d}`. :math:`\delta` may be interpreted as
following:

.. math::

  \delta(\vec{r} - \vec{r}') = \begin{cases}
    \dfrac{1}{dV'}, & \text{if } \vec{r} = \vec{r}' \\
    0,              & \text{otherwise.}
  \end{cases}

Therefore, :math:`\delta` is a function that is zero everywhere except origin,
where it is equal to reciprocal of the volume differential :math:`dV'`.
Replacing :math:`\delta` with a finite valued function :math:`W` in
:eq:`dirac-delta` we obtain the smoothed function :math:`\langle f \rangle`:

.. math::
  :label: smoothed-function

  \langle f \rangle(\vec{r}) =
  \int_{\mathbb{R}^{d}} f(\vec{r}') W(\vec{r} - \vec{r}', h) dV'.

We will refer to :math:`W` in :eq:`smoothed-function` as smooth *kernel*
function with *radius* :math:`h`. The kernel must:

* Approach Dirac's :math:`\delta`-function as :math:`h \to 0`.

* Be normalized:

  .. math::

    \int_{\mathbb{R}^{d}} W(\vec{r}, h) dV' = 1.

* Be symmetric:

  .. math::

    W(\vec{r}, h) = W_{0}(|\vec{r}|, h).

Kernel is often defined in terms of a radial function :math:`w`:

.. math::

  W(\vec{r}, h) = \frac{\omega_{d}}{h^{d}} \, w(q),

where :math:`\omega_{d}` is a normalization constant and
:math:`q = |\vec{r}| / h`. |product| uses the following kernels:

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

Given a set of points :math:`\{ \vec{r}_{a} \}` one can define the following
interpolation operator based on approximation of the smoothed function
:eq:`smoothed-function`:

.. math::
  :label: sph-interpolation

  f(\vec{r}) = \sum_{b} f_{b} W(\vec{r} - \vec{r}_{b}, h) V_{b}.

Here :math:`\{f_{a}\}` is a set of given function values, :math:`\{V_{a}\}` is
the volume, associated with point :math:`\vec{r}_{a}`.

Volumes :math:`\{V_{a}\}` are defined such that the resulting interpolation
preserves given values in the interpolation points:

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
  and the kernel function :math:`W(\vec{r}_{ab}, h)` as :math:`W_{ab}`.
  With this notation, the equation :eq:`sph-density` can be written as:

  .. math::

    \rho_{a} = \sum_{b} m_{b} W_{ab}.

  :math:`\bar{\vec{r}}_{ab}` is the average of :math:`\vec{r}_{a}` and
  :math:`\vec{r}_{b}`. We will follow this notation in the rest of the manual.

First order SPH operators
-------------------------

Gradient operator can be defined from the interpolation operator
:eq:`sph-interpolation` as:

.. math::
  :label: sph-gradient-0

  \mathbf{G}^{0} f_{a} = \sum_{b} f_{b} \nabla W_{ab} V_{b}.

It is easy to show that the gradient operator :eq:`sph-gradient-0`, computed
over the constant function does not vanish. To fix this, the following
correction is applied:

.. math::
  :label: sph-gradient-1

  \mathbf{G}^{1} f_{a} = \sum_{b} f_{ba} \nabla W_{ab} V_{b}.

Equation :eq:`sph-gradient-1` can be derived from the identity:
:math:`\nabla f_{a} = \nabla (f_{a} \cdot 1_{a}) - f_{a} \cdot \nabla 1_{a}`,
where :math:`\nabla (f_{a} \cdot 1_{a}) \equiv \nabla f_{a}` and
:math:`\nabla 1_{a}` are computed from :eq:`sph-gradient-0`.

Although the gradient operator :eq:`sph-gradient-1` vanishes for the constant,
it fails to compute the gradient of the linear function exactly. To fix this,
the following correction is applied:

.. math::
  :label: sph-gradient-2

  \mathbf{G}^{2} f_{a} = L_{a} \sum_{b} f_{ba} \nabla W_{ab} V_{b},

where :math:`L_{a}` is the renormalization matrix, defined as:

.. math::
  :label: sph-renormalization

  L_{a} =
  \left( \sum_{b} \vec{r}_{ab} \otimes \nabla W_{ab} V_{b} \right)^{-1}.

Divergence and curl operators can be defined in a similar way. For example,
constant-vanishing divergence operator can be defined as:

.. math::
  :label: sph-divergence-1

  \mathbf{D}^{1} \vec{F}_{a} =
  \sum_{b} \vec{F}_{ba} \cdot \nabla W_{ab} V_{b}.

Conservative SPH operators
--------------------------

Although the operators :eq:`sph-gradient-1` and :eq:`sph-gradient-2` provide
precise gradient computation, they are not conservative. For example, by
integrating the gradient operator :eq:`sph-gradient-1` over the domain, one
obtains:

.. math::

  \sum_{a} \mathbf{G}^{k} f_{a} V_{a} =
  2 \sum_{a < b} f_{ba} \nabla W_{ab} V_{a} V_{b} \not\equiv 0.

In order to regain the conservation property, the gradient operator
must be *symmetric* in the sense that:

.. math::

  \mathbf{G}^{S} f_{a} = \sum_{b} \mathit{g}(f_{a}, f_{b}) \nabla W_{ab} V_{b},

where :math:`\mathit{g}(f_{a}, f_{b}) = \mathit{g}(f_{b}, f_{a})`. Family of
symmetric gradient operators can be defined as:

.. math::
  :label: sph-gradient-s

  \mathbf{G}^{S}_{k} f_{a} =
  \sum_{b}
  \frac{\rho_{a}^{2k} f_{a} + \rho_{b}^{2k} f_{b}}{\rho_{a}^{k} \rho_{b}^{k}}
  \nabla W_{ab} V_{b}.


Second order SPH operators
--------------------------

Laplacian operator can be defined from the gradient operator
:eq:`sph-gradient-0`:

.. math::

  |\nabla W_{ab}| =

.. math::

  \mathbf{L} f_{a} = \sum_{b} \nabla f_{b} \cdot \nabla W_{ab} V_{b}.

Inner gradients are rarely computed directly. Instead, the whole inner dot
product is approximated as:


.. math::

  \nabla f_{b} \cdot \nabla W_{ab} =
  \frac{\partial f_{b}}{\partial \vec{e}_{ab}} \nabla W_{ab},

where :math:`\vec{e}_{ab} = \vec{r}_{ab} / |\vec{r}_{ab}|`. The directional
derivative can be approximated as:

.. math::

  \frac{\partial f_{b}}{\partial \vec{e}_{ab}} \approx
  \frac{f_{ab}}{|\vec{r}_{ab}|}.

.. ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
