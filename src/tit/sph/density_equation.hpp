/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*\
 * Copyright (C) 2020-2023 Oleg Butakov                                       *
 *                                                                            *
 * Permission is hereby granted, free of charge, to any person obtaining a    *
 * copy of this software and associated documentation files (the "Software"), *
 * to deal in the Software without restriction, including without limitation  *
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,   *
 * and/or sell copies of the Software, and to permit persons to whom the      *
 * Software is furnished to do so, subject to the following conditions:       *
 *                                                                            *
 * The above copyright notice and this permission notice shall be included in *
 * all copies or substantial portions of the Software.                        *
 *                                                                            *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR *
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   *
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    *
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER *
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    *
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        *
 * DEALINGS IN THE SOFTWARE.                                                  *
\*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#pragma once

#include "tit/core/meta.hpp"
#include "tit/sph/field.hpp"

namespace tit::sph {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/******************************************************************************\
 ** Basic summation density.
\******************************************************************************/
class SummationDensity {
public:

  /** Set of particle fields that are required. */
  static constexpr auto required_fields = meta::Set{};

}; // class SummationDensity

/******************************************************************************\
 ** Basic summation density.
\******************************************************************************/
class GradHSummationDensity : public SummationDensity {
public:

  /** Set of particle fields that are required. */
  static constexpr auto required_fields =
      meta::Set{Omega} | SummationDensity::required_fields;

}; // class GradHSummationDensity

/******************************************************************************\
 ** Continuity equation as equation for density.
\******************************************************************************/
class ContinuityEquation {
public:

  /** Set of particle fields that are required. */
  static constexpr auto required_fields = meta::Set{drho_dt};

}; // class ContinuityEquation

/** Density equation type. */
template<class DensityEquation>
concept density_equation =
    std::movable<DensityEquation> &&
    (std::derived_from<DensityEquation, SummationDensity> ||
     std::same_as<DensityEquation, ContinuityEquation>);

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit::sph
