/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <string_view>
#include <utility>

#include "tit/sph/field2.hpp"

namespace tit::sph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto field_rank(FieldID id) noexcept -> FieldRank {
  using enum FieldID;
  using enum FieldRank;
  // NOLINTBEGIN(bugprone-branch-clone)
  switch (id) {
    case h:              return scalar;
    case m:              return scalar;
    case gamma:          return scalar;
    case grad_gamma:     return vector;
    case phi:            return scalar;
    case rho:            return scalar;
    case grad_rho:       return vector;
    case drho_dt:        return scalar;
    case p:              return scalar;
    case cs:             return scalar;
    case v:              return vector;
    case grad_v:         return matrix;
    case dv_dt:          return vector;
    case r:              return vector;
    case N:              return vector;
    case L:              return matrix;
    case dr:             return vector;
    case scratch_scalar: return scalar;
    case scratch_vector: return vector;
    case scratch_matrix: return matrix;
    default:             std::unreachable();
  }
  // NOLINTEND(bugprone-branch-clone)
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto field_name(FieldID id) noexcept -> std::string_view {
  using enum FieldID;
  switch (id) {
    case h:              return "h";
    case m:              return "m";
    case gamma:          return "gamma";
    case grad_gamma:     return "grad_gamma";
    case phi:            return "phi";
    case rho:            return "rho";
    case grad_rho:       return "grad_rho";
    case drho_dt:        return "drho_dt";
    case p:              return "p";
    case cs:             return "cs";
    case v:              return "v";
    case grad_v:         return "grad_v";
    case dv_dt:          return "dv_dt";
    case r:              return "r";
    case N:              return "N";
    case L:              return "L";
    case dr:             return "dr";
    case scratch_scalar: return "_scratch_scalar";
    case scratch_vector: return "_scratch_vector";
    case scratch_matrix: return "_scratch_matrix";
    default:             std::unreachable();
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::sph
