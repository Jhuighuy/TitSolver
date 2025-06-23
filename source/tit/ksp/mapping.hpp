/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>

#include "tit/core/checks.hpp"

namespace tit::ksp {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Vector space operator (mapping).
template<class InSpace_, class OutSpace_>
class Mapping {
public:

  /// Input space type.
  using InSpace = InSpace_;

  /// Output space type.
  using OutSpace = OutSpace_;

  /// Input space vector type.
  using InVec = typename InSpace::Vec;

  /// Output space vector type.
  using OutVec = typename OutSpace::Vec;

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Construct a mapping.
  constexpr Mapping(const InSpace& in_space, const OutSpace& out_space) noexcept
      : in_space_{&in_space}, out_space_{&out_space} {}

  /// Get the input vector space.
  constexpr auto in_space() const noexcept -> const InSpace& {
    TIT_ASSERT(in_space_ != nullptr, "Invalid input space pointer!");
    return *in_space_;
  }

  /// Get the to vector space.
  constexpr auto out_space() const noexcept -> const OutSpace& {
    TIT_ASSERT(out_space_ != nullptr, "Invalid output space pointer!");
    return *out_space_;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Compute a mapping product, 𝒚 ← 𝓐(𝒙).
  constexpr void apply(OutVec& y_vec, const InVec& x_vec) const =
      delete; // Must be implemented in derived class.

  /// Compute a chained mapping product, 𝒛 ← 𝓐(𝒚 ← 𝓑(𝒙)).
  constexpr void chapply(this const auto& A,
                         OutVec&          z_vec,
                         auto&            y_vec,
                         const auto&      B,
                         const InVec&     x_vec) {
    B.apply(y_vec, x_vec);
    A.apply(z_vec, y_vec);
  }

  /// Compute a residual, 𝒓 ← 𝒃 - 𝓐(𝒙).
  constexpr void residual(this const auto& self,
                          OutVec&          r_vec,
                          const OutVec&    b_vec,
                          const InVec&     x_vec) {
    self.apply(r_vec, x_vec);
    self.out_space().sub(r_vec, b_vec, r_vec);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  [[no_unique_address]] const InSpace*  in_space_;
  [[no_unique_address]] const OutSpace* out_space_;

}; // class Mapping

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Square vector mapping (square operator).
template<class Space>
class SquareMapping : public Mapping<Space, Space> {
public:

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Construct a square mapping.
  constexpr explicit SquareMapping(const Space& space) noexcept
      : Mapping<Space, Space>{space, space} {}

  /// Get the vector space.
  constexpr auto space() const noexcept -> const Space& {
    TIT_ASSERT(&this->in_space() == &this->out_space(),
               "Input and output spaces pointers are invalid!");
    return this->in_space();
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

}; // class SquareMapping

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Identity mapping.
template<class Space>
class Identity final : public SquareMapping<Space> {
public:

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Construct an identity mapping.
  constexpr explicit Identity(const Space& space) noexcept
      : SquareMapping<Space>{space} {}

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Compute a mapping product, 𝒚 ← 𝓐𝒙.
  constexpr void apply(Space::Vec& y_vec, const Space::Vec& x_vec) const {
    this->space().copy(y_vec, x_vec);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

}; // class Identity

/// Identity type.
template<class IM>
concept identity = std::same_as<IM, Identity<IM>>;

/// Is identity mapping?
template<class IM>
constexpr auto is_identity(const IM& /*im*/) noexcept -> bool {
  // return std::same_as<IM, Identity<typename IM::Space>>;
  return true;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::ksp
