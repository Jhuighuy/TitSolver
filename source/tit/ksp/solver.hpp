/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <optional>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/print.hpp"
#include "tit/core/type.hpp"

#include "tit/ksp/mapping.hpp"

namespace tit::ksp {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Operator equation solver.
template<class Space>
class Solver : public SquareMapping<Space> {
  using SquareMapping<Space>::SquareMapping;
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Operator equation iterative solver.
template<class Mapping_, class Preconditioner_>
class IterativeSolver : public Solver<typename Mapping_::Space> {
public:

  /// Mapping type.
  using Mapping = Mapping_;

  /// Preconditioner type.
  using Preconditioner = Preconditioner_;

  /// Space type.
  using Space = typename Mapping::Space;

  /// Vector type.
  using Vec = typename Space::Vec;

  /// Scalar type.
  using Num = typename Space::Num;

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Construct an iterative solver.
  constexpr IterativeSolver(const Mapping& A, const Preconditioner& P) noexcept
      : Solver<Space>{A.space()}, mapping_{&A}, preconditioner_{&P} {}

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Get the mapping.
  constexpr auto mapping() const noexcept -> const Mapping& {
    return *mapping_;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Does the solver use a preconditioner?
  constexpr auto has_preconditioner() const noexcept -> bool {
    return !is_identity(preconditioner());
  }

  /// Get the preconditioner.
  constexpr auto preconditioner() const noexcept -> const Preconditioner& {
    return *preconditioner_;
  }

  /// Does the solver use a right preconditioner?
  /// @todo Some solvers may not allow a specific preconditioner side.
  constexpr auto has_right_preconditioner() const noexcept -> bool {
    return has_preconditioner() && right_preconditioner_;
  }

  /// Does the solver use a left preconditioner?
  constexpr auto has_left_preconditioner() const noexcept -> bool {
    return has_preconditioner() && !right_preconditioner_;
  }

  /// Use a right preconditioner.
  constexpr void use_right_preconditioner() noexcept {
    right_preconditioner_ = true;
  }

  /// Use a left preconditioner.
  constexpr void use_left_preconditioner() noexcept {
    right_preconditioner_ = false;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Is the number of iterations limit enabled?
  constexpr auto has_num_iterations_limit() const noexcept -> bool {
    return num_iterations_.has_value();
  }

  /// Get the number of iterations.
  constexpr auto num_iterations() const -> size_t {
    TIT_ASSERT(has_num_iterations_limit(),
               "Number of iterations limit is disabled!");
    return num_iterations_.value();
  }

  /// Set the number of iterations.
  constexpr void set_num_iterations(size_t num_iterations) noexcept {
    TIT_ASSERT(num_iterations > 0, "Number of iterations must be positive!");
    num_iterations_ = num_iterations;
  }

  /// Disable the number of iterations limit.
  constexpr void disable_num_iterations_limit() noexcept {
    num_iterations_ = std::nullopt;
  }

  /// Get the current iteration.
  constexpr auto iteration() const noexcept -> size_t {
    return iteration_;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Is the absolute error check enabled?
  constexpr auto has_absolute_error_check() const noexcept -> bool {
    return absolute_tolerance_.has_value();
  }

  /// Get the absolute error tolerance.
  constexpr auto absolute_tolerance() const -> Num {
    TIT_ASSERT(has_absolute_error_check(), "Absolute error check is disabled!");
    return absolute_tolerance_.value();
  }

  /// Set the absolute error tolerance.
  constexpr void set_absolute_tolerance(Num absolute_tolerance) noexcept {
    TIT_ASSERT(absolute_tolerance > Num{0.0},
               "Absolute error tolerance value must be positive!");
    absolute_tolerance_ = absolute_tolerance;
  }

  /// Disable the absolute error check.
  constexpr void disable_absolute_error_check() noexcept {
    absolute_tolerance_ = std::nullopt;
  }

  /// Get the absolute error at the current iteration.
  constexpr auto absolute_error() const noexcept -> Num {
    return absolute_error_;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Is the relative error check enabled?
  constexpr auto has_relative_error_check() const noexcept -> bool {
    return relative_tolerance_.has_value();
  }

  /// Get the relative error tolerance.
  constexpr auto relative_tolerance() const -> Num {
    TIT_ASSERT(has_relative_error_check(), "Relative error check is disabled!");
    return relative_tolerance_.value();
  }

  /// Set the relative error tolerance.
  constexpr void set_relative_tolerance(Num relative_tolerance) noexcept {
    TIT_ASSERT(relative_tolerance > 0.0,
               "Relative error tolerance value must be positive!");
    relative_tolerance_ = relative_tolerance;
  }

  /// Disable the relative error check.
  constexpr void disable_relative_error_check() noexcept {
    relative_tolerance_ = std::nullopt;
  }

  /// Get the relative error at the current iteration.
  constexpr auto relative_error() const noexcept -> Num {
    return relative_error_;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Solve the equation 𝓐(𝒙) = 𝒃 with preconditioner 𝓟(𝒙).
  auto apply(this const auto& self, Vec& x_vec, Vec& b_vec) -> bool {
    // Initialize the solver.
    log("Solver '{}':", type_name_of(self));
    self.absolute_error_ = self.init_(x_vec, b_vec);
    if (self.has_absolute_error_check() &&
        self.absolute_error() < self.absolute_tolerance()) {
      self.finalize_(x_vec, b_vec);
      return true;
    }

    // Iterate the solver.
    bool       converged     = false;
    const auto initial_error = self.absolute_error();
    for (self.iteration_ = 0; !self.has_num_iterations_limit() ||
                              self.iteration_ < self.num_iterations();
         ++self.iteration_) {
      self.absolute_error_ = self.iterate_(x_vec, b_vec);
      self.relative_error_ = self.absolute_error_ / initial_error;
      log("n_iter: {:>4}, |abs_error|: {:>-12e}, |rel_error|: {:>-12e}",
          self.iteration_,
          self.absolute_error_,
          self.relative_error_);

      if ((self.has_absolute_error_check() &&
           (self.absolute_error() < self.absolute_tolerance())) ||
          (self.has_relative_error_check() &&
           self.relative_error() < self.relative_tolerance())) {
        converged = true;
        break;
      }
    }

    // Exit the solver.
    self.finalize_(x_vec, b_vec);
    return converged;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Initialize the iterative solver.
  /// @returns Residual norm of the initial guess, ‖𝒃 - 𝓐(𝒙)‖.
  constexpr auto init_(Vec&       x_vec,
                       const Vec& b_vec) const
      -> Num = delete; // Must be implemented in derived class.

  /// Iterate the solver.
  /// @returns Residual norm of the current guess, ‖𝒃 - 𝓐(𝒙)‖.
  constexpr auto iterate_(Vec&       x_vec,
                          const Vec& b_vec) const
      -> Num = delete; // Must be implemented in derived class.

  /// Finalize the iterations.
  constexpr void finalize_([[maybe_unused]] Vec&       x_vec,
                           [[maybe_unused]] const Vec& b_vec) const {
    // No-op by default.
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  const Mapping*        mapping_              = nullptr;
  const Preconditioner* preconditioner_       = nullptr;
  std::optional<size_t> num_iterations_       = 2000;
  mutable size_t        iteration_            = 0;
  std::optional<Num>    absolute_tolerance_   = Num{1.0e-7};
  std::optional<Num>    relative_tolerance_   = Num{1.0e-7};
  mutable Num           absolute_error_       = Num{0.0};
  mutable Num           relative_error_       = Num{0.0};
  bool                  right_preconditioner_ = false;

}; // class IterativeSolver

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Inner-outer iterative solver.
template<class Mapping, class Preconditioner>
class InnerOuterIterativeSolver :
    public IterativeSolver<Mapping, Preconditioner> {
public:

  using Base = IterativeSolver<Mapping, Preconditioner>;
  using Base::IterativeSolver;
  using typename Base::Num;
  using typename Base::Vec;

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Get the number of inner iterations.
  constexpr auto num_inner_iterations() const noexcept -> size_t {
    return num_inner_iterations_;
  }

  /// Set the number of inner iterations.
  constexpr void set_num_inner_iterations(
      size_t num_inner_iterations) noexcept {
    TIT_ASSERT(num_inner_iterations > 0,
               "Number of inner iterations must be positive!");
    num_inner_iterations_ = num_inner_iterations;
  }

  /// Get the current outer iteration.
  constexpr auto outer_iteration() const noexcept -> size_t {
    return this->iteration() / num_inner_iterations();
  }

  /// Get the current inner iteration.
  constexpr auto inner_iteration() const noexcept -> size_t {
    return this->iteration() % num_inner_iterations();
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  constexpr auto init_(this const auto& self, Vec& x_vec, const Vec& b_vec) {
    return self.outer_init_(x_vec, b_vec);
  }

  constexpr auto iterate_(this const auto& self, Vec& x_vec, const Vec& b_vec)
      -> Num {
    if (self.inner_iteration() == 0) {
      self.inner_init_(x_vec, b_vec);
    }
    const auto residual_norm = self.inner_iterate_(x_vec, b_vec);
    if (self.inner_iteration() == self.num_inner_iterations() - 1) {
      self.inner_finalize_(x_vec, b_vec);
    }
    return residual_norm;
  }

  constexpr void finalize_(this const auto& self,
                           Vec&             x_vec,
                           const Vec&       b_vec) {
    if (self.inner_iteration() != self.num_inner_iterations() - 1) {
      self.inner_finalize_(x_vec, b_vec);
    }
    self.outer_finalize_(x_vec, b_vec);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Initialize the outer iterations.
  /// This function is used invoked only once, in the initialization phase.
  /// @returns Residual norm of the initial guess, ‖𝒃 - 𝓐(𝒙)‖.
  constexpr auto outer_init_(Vec&       x_vec,
                             const Vec& b_vec) const
      -> Num = delete; // Must be implemented in derived class.

  /// Initialize the inner iterations.
  /// This function is invoked before the each inner iteration loop.
  constexpr void inner_init_([[maybe_unused]] Vec&       x_vec,
                             [[maybe_unused]] const Vec& b_vec) const {
    // No-op by default.
  }

  /// Perform the inner iteration.
  /// @returns Residual norm of the current guess, ‖𝒃 - 𝓐(𝒙)‖.
  constexpr auto inner_iterate_(Vec&       x_vec,
                                const Vec& b_vec) const
      -> Num = delete; // Must be implemented in derived class.

  /// Finalize the inner iterations.
  /// This function is called in order to finalize the inner iterations or if
  /// some stopping criterion is met.
  constexpr void inner_finalize_([[maybe_unused]] Vec&       x_vec,
                                 [[maybe_unused]] const Vec& b_vec) const {
    // No-op by default.
  }

  /// Finalize the outer iterations.
  /// This function is used invoked only once, when some stopping criterion is
  /// met.
  constexpr void outer_finalize_([[maybe_unused]] Vec&       x_vec,
                                 [[maybe_unused]] const Vec& b_vec) const {
    // No-op by default.
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  size_t num_inner_iterations_ = 50;

}; // class InnerOuterIterativeSolver

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::ksp
