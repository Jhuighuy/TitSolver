/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <memory>
#include <utility>

#include "tit/core/basic_types.hpp"
#include "tit/core/mat.hpp"
#include "tit/core/vec.hpp"

#include "tit/ksp/blas.hpp"
#include "tit/ksp/precond.hpp"
#include "tit/ksp/solver.hpp"
#include "tit/ksp/solver_bicgstab.hpp"
#include "tit/ksp/solver_cg.hpp"
#include "tit/ksp/solver_cgs.hpp"
#include "tit/ksp/solver_gmres.hpp"
#include "tit/ksp/solver_idrs.hpp"
#include "tit/ksp/solver_newton.hpp" // IWYU pragma: keep
#include "tit/ksp/solver_tfqmr.hpp"

#include "tit/testing/test.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

template<class Num, size_t Dim>
class ksp::VectorOperations<Vec<Num, Dim>> {
public:

  static auto Dot(const Vec<Num, Dim>& x, const Vec<Num, Dim>& y) {
    return dot(x, y);
  }

  static auto Norm2(const Vec<Num, Dim>& x) {
    return norm(x);
  }

  static void Set(Vec<Num, Dim>& x, const Vec<Num, Dim>& y) {
    x = y;
  }

  static void Fill(Vec<Num, Dim>& x, Num a) {
    x = Vec<Num, Dim>(a);
  }
  static void RandFill(Vec<Num, Dim>& x) {
    static Num a = 0.0;
    for (size_t i = 0; i < Dim; ++i) x[i] = a += 0.1;
  }

  static void Scale(Vec<Num, Dim>& x, const Vec<Num, Dim>& y, Num a) {
    x = a * y;
  }

  static void ScaleAssign(Vec<Num, Dim>& x, Num a) {
    x *= a;
  }

  static void Add(Vec<Num, Dim>& x,
                  const Vec<Num, Dim>& y,
                  const Vec<Num, Dim>& z) {
    x = y + z;
  }
  static void Add(Vec<Num, Dim>& x,
                  const Vec<Num, Dim>& y,
                  const Vec<Num, Dim>& z,
                  Num b) {
    x = y + z * b;
  }
  static void Add(Vec<Num, Dim>& x,
                  const Vec<Num, Dim>& y,
                  Num a,
                  const Vec<Num, Dim>& z,
                  Num b) {
    x = y * a + z * b;
  }

  static void AddAssign(Vec<Num, Dim>& x, const Vec<Num, Dim>& y) {
    x += y;
  }
  static void AddAssign(Vec<Num, Dim>& x, const Vec<Num, Dim>& y, Num a) {
    x += y * a;
  }

  static void Sub(Vec<Num, Dim>& x,
                  const Vec<Num, Dim>& y,
                  const Vec<Num, Dim>& z) {
    x = y - z;
  }
  static void Sub(Vec<Num, Dim>& x,
                  const Vec<Num, Dim>& y,
                  const Vec<Num, Dim>& z,
                  Num b) {
    x = y - z * b;
  }

  static void SubAssign(Vec<Num, Dim>& x, const Vec<Num, Dim>& y) {
    x -= y;
  }
  static void SubAssign(Vec<Num, Dim>& x, const Vec<Num, Dim>& y, Num a) {
    x -= y * a;
  }

}; // class ksp::VectorOperations

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("KSP") {
  const Vec<double, 8> x{1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0};
  const Mat<double, 8> A{
      {2.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
      {1.0, 2.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0},
      {0.0, 1.0, 2.0, 1.0, 0.0, 0.0, 0.0, 0.0},
      {0.0, 0.0, 1.0, 2.0, 1.0, 0.0, 0.0, 0.0},
      {0.0, 0.0, 0.0, 1.0, 2.0, 1.0, 0.0, 0.0},
      {0.0, 0.0, 0.0, 0.0, 1.0, 2.0, 1.0, 0.0},
      {0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 2.0, 1.0},
      {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 2.0},
  };
  const auto b = A * x;
  const auto Op = [A](auto& y, const auto& z) { y = A * z; };
  Vec<double, 8> u{};
  SUBCASE("BiCGStab") {
    ksp::BiCGStab<Vec<double, 8>> solver{};
    ksp::solve(solver, Op, u, b);
    CHECK(approx_equal_to(u, x));
  }
  SUBCASE("BiCGStabL") {
    ksp::BiCGStabL<Vec<double, 8>> solver{};
    ksp::solve(solver, Op, u, b);
    CHECK(approx_equal_to(u, x));
  }
  SUBCASE("CG") {
    ksp::CG<Vec<double, 8>> solver{};
    ksp::solve(solver, Op, u, b);
    CHECK(approx_equal_to(u, x));
  }
  SUBCASE("CGS") {
    ksp::CGS<Vec<double, 8>> solver{};
    ksp::solve(solver, Op, u, b);
    CHECK(approx_equal_to(u, x));
  }
  SUBCASE("GMRES") {
    ksp::GMRES<Vec<double, 8>> solver{};
    ksp::solve(solver, Op, u, b);
    CHECK(approx_equal_to(u, x));
  }
  SUBCASE("FGMRES") {
    ksp::FGMRES<Vec<double, 8>> solver{};
    ksp::solve(solver, Op, u, b);
    CHECK(approx_equal_to(u, x));
  }
  SUBCASE("IDRs") {
    ksp::IDRs<Vec<double, 8>> solver{};
    ksp::solve(solver, Op, u, b);
    CHECK(approx_equal_to(u, x));
  }
  SUBCASE("TFQMR") {
    ksp::TFQMR<Vec<double, 8>> solver{};
    ksp::solve(solver, Op, u, b);
    CHECK(approx_equal_to(u, x));
  }
  SUBCASE("TFQMR1") {
    ksp::TFQMR1<Vec<double, 8>> solver{};
    ksp::solve(solver, Op, u, b);
    CHECK(approx_equal_to(u, x));
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("KSP with Preconditioner") {
  const Vec<double, 8> x{1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0};
  const Mat<double, 8> A{
      {2.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
      {1.0, 2.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0},
      {0.0, 1.0, 2.0, 1.0, 0.0, 0.0, 0.0, 0.0},
      {0.0, 0.0, 1.0, 2.0, 1.0, 0.0, 0.0, 0.0},
      {0.0, 0.0, 0.0, 1.0, 2.0, 1.0, 0.0, 0.0},
      {0.0, 0.0, 0.0, 0.0, 1.0, 2.0, 1.0, 0.0},
      {0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 2.0, 1.0},
      {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 2.0},
  };
  const auto b = A * x;
  const auto Op = [A](auto& y, const auto& z) { y = A * z; };
  auto Pre = std::make_unique<ksp::IdentityPreconditioner<Vec<double, 8>>>();
  Vec<double, 8> u{};
  SUBCASE("BiCGStab Left") {
    ksp::BiCGStab<Vec<double, 8>> solver{};
    solver.PreSide = ksp::PreconditionerSide::Left;
    solver.PreOp = std::move(Pre);
    ksp::solve(solver, Op, u, b);
    CHECK(approx_equal_to(u, x));
  }
  SUBCASE("BiCGStab Right") {
    ksp::BiCGStab<Vec<double, 8>> solver{};
    solver.PreSide = ksp::PreconditionerSide::Right;
    solver.PreOp = std::move(Pre);
    ksp::solve(solver, Op, u, b);
    CHECK(approx_equal_to(u, x));
  }
  SUBCASE("BiCGStabL") {
    // Looks like BiCgStabL has no preconditioner side option.
    ksp::BiCGStabL<Vec<double, 8>> solver{};
    solver.PreOp = std::move(Pre);
    ksp::solve(solver, Op, u, b);
    CHECK(approx_equal_to(u, x));
  }
  SUBCASE("CG") {
    ksp::CG<Vec<double, 8>> solver{};
    solver.PreOp = std::move(Pre);
    ksp::solve(solver, Op, u, b);
    CHECK(approx_equal_to(u, x));
  }
  SUBCASE("CGS Left") {
    ksp::CGS<Vec<double, 8>> solver{};
    solver.PreSide = ksp::PreconditionerSide::Left;
    solver.PreOp = std::move(Pre);
    ksp::solve(solver, Op, u, b);
    CHECK(approx_equal_to(u, x));
  }
  SUBCASE("CGS Right") {
    ksp::CGS<Vec<double, 8>> solver{};
    solver.PreSide = ksp::PreconditionerSide::Right;
    solver.PreOp = std::move(Pre);
    ksp::solve(solver, Op, u, b);
    CHECK(approx_equal_to(u, x));
  }
  SUBCASE("GMRES Left") {
    ksp::GMRES<Vec<double, 8>> solver{};
    solver.PreSide = ksp::PreconditionerSide::Left;
    solver.PreOp = std::move(Pre);
    ksp::solve(solver, Op, u, b);
    CHECK(approx_equal_to(u, x));
  }
  SUBCASE("GMRES Right") {
    ksp::GMRES<Vec<double, 8>> solver{};
    solver.PreSide = ksp::PreconditionerSide::Right;
    solver.PreOp = std::move(Pre);
    ksp::solve(solver, Op, u, b);
    CHECK(approx_equal_to(u, x));
  }
  SUBCASE("FGMRES Left") {
    ksp::FGMRES<Vec<double, 8>> solver{};
    solver.PreSide = ksp::PreconditionerSide::Left;
    solver.PreOp = std::move(Pre);
    ksp::solve(solver, Op, u, b);
    CHECK(approx_equal_to(u, x));
  }
  SUBCASE("FGMRES Right") {
    ksp::FGMRES<Vec<double, 8>> solver{};
    solver.PreSide = ksp::PreconditionerSide::Right;
    solver.PreOp = std::move(Pre);
    ksp::solve(solver, Op, u, b);
    CHECK(approx_equal_to(u, x));
  }
  SUBCASE("IDRs Left") {
    ksp::IDRs<Vec<double, 8>> solver{};
    solver.PreSide = ksp::PreconditionerSide::Left;
    solver.PreOp = std::move(Pre);
    ksp::solve(solver, Op, u, b);
    CHECK(approx_equal_to(u, x));
  }
  SUBCASE("IDRs Right") {
    ksp::IDRs<Vec<double, 8>> solver{};
    solver.PreSide = ksp::PreconditionerSide::Right;
    solver.PreOp = std::move(Pre);
    ksp::solve(solver, Op, u, b);
    CHECK(approx_equal_to(u, x));
  }
  SUBCASE("TFQMR Left") {
    ksp::TFQMR<Vec<double, 8>> solver{};
    solver.PreSide = ksp::PreconditionerSide::Left;
    solver.PreOp = std::move(Pre);
    ksp::solve(solver, Op, u, b);
    CHECK(approx_equal_to(u, x));
  }
  SUBCASE("TFQMR Right") {
    ksp::TFQMR<Vec<double, 8>> solver{};
    solver.PreSide = ksp::PreconditionerSide::Right;
    solver.PreOp = std::move(Pre);
    ksp::solve(solver, Op, u, b);
    CHECK(approx_equal_to(u, x));
  }
  SUBCASE("TFQMR1 Left") {
    ksp::TFQMR1<Vec<double, 8>> solver{};
    solver.PreSide = ksp::PreconditionerSide::Left;
    solver.PreOp = std::move(Pre);
    ksp::solve(solver, Op, u, b);
    CHECK(approx_equal_to(u, x));
  }
  SUBCASE("TFQMR1 Right") {
    ksp::TFQMR1<Vec<double, 8>> solver{};
    solver.PreSide = ksp::PreconditionerSide::Right;
    solver.PreOp = std::move(Pre);
    ksp::solve(solver, Op, u, b);
    CHECK(approx_equal_to(u, x));
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
