.. ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   Part of the Tit Solver project, under the MIT License
   See /LICENSE.md for license information.
   SPDX-License-Identifier: MIT
   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Vectors
=======

.. code-block:: cpp

   #include <tit/core/vec.hpp>

Overview
--------

The header file ``tit/core/vec.hpp`` defines a fixed-sized SIMD-powered
algebraic vector class ``Vec<Num, Dim>``.

``Vec`` can be constructed with braces, its size and element type will be
deduced.

.. code-block:: cpp

   // Construct a double-precision vector holding five elements:
   const auto a = tit::Vec{5.0, 7.0, 1.0, 2.0, 4.0};

All common component-wise operations (``+``, ``-``, ``*`` and ``/``) are
supported.

.. code-block:: cpp

   tit::Vec<double, N> u{...};
   tit::Vec<double, N> v{...};

   // Addition and subtraction:
   const auto w2 = -u;
   const auto w2 = u + v;
   const auto w3 = u - v;

   // Scaling:
   const auto w3 = 3.0 * u;
   const auto w4 = u / 5.0;
   const auto w5 = u - 7.0 * u;

   // Component-wise multiplication and division:
   const auto w6 = u * v;
   const auto w7 = u / v;

The operation result type is deduced from the arguments.

.. code-block:: cpp

   tit::Vec<int, N> i{...};
   tit::Vec<double, N> d{...};

   const auto r1 = 0.5 * i; // will be `tit::Vec<double, N>`
   const auto r2 = i + d;   // will be `tit::Vec<double, N>`

API reference
-------------

.. ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``Vec<Num, Dim>`` class
^^^^^^^^^^^^^^^^^^^^^^^

.. doxygenclass:: tit::Vec
   :project: tit_core
   :members:
   :members-only:

.. ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Arithmetics
^^^^^^^^^^^

.. doxygengroup:: tit-vec-arithmetics
   :project: tit_core
   :content-only:

.. ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Reduction
^^^^^^^^^

.. doxygengroup:: tit-vec-reduction
   :project: tit_core
   :content-only:

.. ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Dot and cross products, normalization
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. doxygengroup:: tit-vec-product
   :project: tit_core
   :content-only:

.. ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Comparison
^^^^^^^^^^

.. doxygengroup:: tit-vec-comparison
   :project: tit_core
   :content-only:
   :members:

.. ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
