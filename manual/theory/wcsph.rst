.. ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
.. Part of the Tit Solver project, under the MIT License.
.. See /LICENSE.md for license information. SPDX-License-Identifier: MIT
.. ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. include:: ../shared.rst

.. ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Weakly compressible SPH
=======================

Governing equations
-------------------

Weakly-compressible fluid equations in Lagrange coordinates are given by:

.. math::
  :label: weakly-compressible-equations

  \frac{d\vec{r}}{dt} &= \vec{v}, \\
  \frac{d\rho}{dt} &= -\rho \nabla \cdot \vec{v}, \\
  \frac{d\vec{v}}{dt} &= -\frac{1}{\rho} \nabla p + \vec{g}.

Pressure and density are linked by a weakly-compressible equation of state
:math:`p = p(\rho)`. |product| uses the following equation of state:

.. list-table:: Weakly-compressible equations of state
  :header-rows: 1

  * - Name
    - Pressure-density relation

  * - **Tait equation**
    - .. math::

        p =
        p_{0} +
        \frac{\rho_{0} c_{0}^{2}}{\gamma}
        \left[ \left( \frac{\rho}{\rho_{0}} \right)^{\gamma} - 1 \right],

  * - **Linear Tait equation**
    - .. math::

        p = p_{0} + c_{0}^{2} (\rho - \rho_{0}),

In the above equations :math:`p_{0}` is the gackground pressure,
:math:`c_{0}` is the reference speed of sound, :math:`\rho_{0}` is the
reference density and :math:`\gamma` is the specific heat ratio.
Typically reference sound speed :math:`c_{0}` is set such that is at least ten
times larger than maximal flow velocity, so variations in density are less
than 1%.

SPH discretization
------------------

In |product|, the following SPH discretization for the weakly-compressible fluid
equations :eq:`weakly-compressible-equations` is used:

.. math::
  :label: wcsph-equations

  \frac{d\vec{r}_{a}}{dt} &= \vec{v}_{a}, \\
  \frac{d\rho_{a}}{dt} &=
  \sum_{b}
  m_{b} \left(
    -\vec{v}_{ba} + \frac{\vec{\Psi}_{ab}}{\rho_{b}}
  \right) \cdot \nabla_{a} W_{ab}, \\
  \frac{d\vec{v}_{a}}{dt} &=
  \sum_{b}
  m_{b} \left(
      -\frac{p_{a}}{\rho_{a}^{2}} - \frac{p_{b}}{\rho_{b}^{2}}
      + \Pi_{ab}
  \right) \nabla_{a} W_{ab}

The turms :math:`\vec{\Psi}_{ab}` and :math:`\Pi_{ab}` represent the artificial
density diffusion and artificial viscosity terms, respectively.
|product| uses the following expressions for these terms:

.. list-table:: Weakly-compressible artificial viscosity schemes
  :header-rows: 1

  * - Name
    - Density equation term :math:`\vec{\Psi}_{ab}`
    - Velocty equation term :math:`\Pi_{ab}`

  * - No artificial terms
    - .. math::

        0

    - .. math::

        0

  * - Alpha-Beta
    - .. math::

        0

    - .. math::

        \frac{\mu_{ab}}{\bar{\rho}_{ab}}
        \left( \alpha \bar{c}_{ab} - \beta \mu_{ab} \right), \;
        \mu_{ab} = h
        \frac{ \vec{v}_{ab} \cdot \vec{r}_{ab} }{ |\vec{r}_{ab}|^{2} },

  * - Molteni-Colagrossi
    - .. math::

        \xi \, h c_{0} \, D_{ab} \,
        \frac{\vec{r}_{ab}}{ |\vec{r}_{ab}|^{2} }, \;
        D_{ab} = 2 \rho_{ab}

    - .. math::

        \alpha \, h c_{0} \rho_{0} \,
        \frac{ \vec{v}_{ab} \cdot \vec{r}_{ab} }
             { \rho_{a}\rho_{b} |\vec{r}_{ab}|^{2} }

  * - Delta SPH
    - .. math::

        \xi \, h c_{0} \, D_{ab} \,
        \frac{\vec{r}_{ab}}{ |\vec{r}_{ab}|^{2} }, \;
        D_{ab} = 2 \rho_{ab} -
        (\nabla\rho_{a} + \nabla\rho_{b}) \cdot \vec{r}_{ab},

      where density gradient is computed with :eq:`sph-gradient-2`.

    - .. math::

        \alpha \, h c_{0} \rho_{0} \,
        \frac{ \vec{v}_{ab} \cdot \vec{r}_{ab} }
             { \rho_{a}\rho_{b} |\vec{r}_{ab}|^{2} }

.. ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
