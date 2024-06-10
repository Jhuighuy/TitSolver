/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <memory>
#include <utility>

#include "tit/core/basic_types.hpp"
#include "tit/core/mat.hpp"
#include "tit/core/vec.hpp"

// NOLINTBEGIN
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include "tit/ksp/Operator.hpp"       // IWYU pragma: keep
#include "tit/ksp/Preconditioner.hpp" // IWYU pragma: keep
#include "tit/ksp/Solver.hpp"         // IWYU pragma: keep
#include "tit/ksp/SolverBiCgStab.hpp" // IWYU pragma: keep
#include "tit/ksp/SolverCg.hpp"       // IWYU pragma: keep
#include "tit/ksp/SolverCgs.hpp"      // IWYU pragma: keep
#include "tit/ksp/SolverGmres.hpp"    // IWYU pragma: keep
#include "tit/ksp/SolverIdrs.hpp"     // IWYU pragma: keep
#include "tit/ksp/SolverNewton.hpp"   // IWYU pragma: keep
#include "tit/ksp/SolverTfqmr.hpp"    // IWYU pragma: keep
#include "tit/ksp/Vector.hpp"         // IWYU pragma: keep
#pragma GCC diagnostic pop
// NOLINTEND

#include "tit/testing/test.hpp"

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

template<class Num, size_t Dim>
class ksp::VectorOperations<Vec<Num, Dim>> {
public:

  static auto Dot(const Vec<Num, Dim>& xVec, const Vec<Num, Dim>& yVec) {
    return dot(xVec, yVec);
  }

  static auto Norm2(const Vec<Num, Dim>& xVec) {
    return norm(xVec);
  }

  static void Swap(Vec<Num, Dim>& xVec, Vec<Num, Dim>& yVec) {
    std::swap(xVec, yVec);
  }

  static void Set(Vec<Num, Dim>& xVec, const Vec<Num, Dim>& yVec) {
    xVec = yVec;
  }

  static void Fill(Vec<Num, Dim>& xVec, Num a) {
    xVec = Vec<Num, Dim>(a);
  }
  static void RandFill(Vec<Num, Dim>& xVec) {
    static Num a = 0.0;
    for (size_t i = 0; i < Dim; ++i) xVec[i] = a += 0.1;
  }

  static void Scale(Vec<Num, Dim>& xVec, const Vec<Num, Dim>& yVec, Num a) {
    xVec = a * yVec;
  }

  static void ScaleAssign(Vec<Num, Dim>& xVec, Num a) {
    xVec *= a;
  }

  static void Add(Vec<Num, Dim>& xVec,
                  const Vec<Num, Dim>& yVec,
                  const Vec<Num, Dim>& zVec) {
    xVec = yVec + zVec;
  }
  static void Add(Vec<Num, Dim>& xVec,
                  const Vec<Num, Dim>& yVec,
                  const Vec<Num, Dim>& zVec,
                  Num b) {
    xVec = yVec + zVec * b;
  }
  static void Add(Vec<Num, Dim>& xVec,
                  const Vec<Num, Dim>& yVec,
                  Num a,
                  const Vec<Num, Dim>& zVec,
                  Num b) {
    xVec = yVec * a + zVec * b;
  }

  static void AddAssign(Vec<Num, Dim>& xVec, const Vec<Num, Dim>& yVec) {
    xVec += yVec;
  }
  static void AddAssign(Vec<Num, Dim>& xVec, const Vec<Num, Dim>& yVec, Num a) {
    xVec += yVec * a;
  }

  static void Sub(Vec<Num, Dim>& xVec,
                  const Vec<Num, Dim>& yVec,
                  const Vec<Num, Dim>& zVec) {
    xVec = yVec - zVec;
  }
  static void Sub(Vec<Num, Dim>& xVec,
                  const Vec<Num, Dim>& yVec,
                  const Vec<Num, Dim>& zVec,
                  Num b) {
    xVec = yVec - zVec * b;
  }

  static void SubAssign(Vec<Num, Dim>& xVec, const Vec<Num, Dim>& yVec) {
    xVec -= yVec;
  }
  static void SubAssign(Vec<Num, Dim>& xVec, const Vec<Num, Dim>& yVec, Num a) {
    xVec -= yVec * a;
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
  const auto Op = ksp::MakeOperator<Vec<double, 8>>(
      [A](auto& y, const auto& z) { y = A * z; });
  Vec<double, 8> u{};
  SUBCASE("BiCGStab") {
    ksp::BiCgStabSolver<Vec<double, 8>> solver{};
    solver.Solve(u, b, *Op);
    CHECK(approx_equal_to(u, x));
  }
  SUBCASE("BiCGStabL") {
    ksp::BiCgStabLSolver<Vec<double, 8>> solver{};
    solver.Solve(u, b, *Op);
    CHECK(approx_equal_to(u, x));
  }
  SUBCASE("CG") {
    ksp::CgSolver<Vec<double, 8>> solver{};
    solver.Solve(u, b, *Op);
    CHECK(approx_equal_to(u, x));
  }
  SUBCASE("CGS") {
    ksp::CgsSolver<Vec<double, 8>> solver{};
    solver.Solve(u, b, *Op);
    CHECK(approx_equal_to(u, x));
  }
  SUBCASE("GMRES") {
    ksp::GmresSolver<Vec<double, 8>> solver{};
    solver.Solve(u, b, *Op);
    CHECK(approx_equal_to(u, x));
  }
  SUBCASE("FGMRES") {
    ksp::FgmresSolver<Vec<double, 8>> solver{};
    solver.Solve(u, b, *Op);
    CHECK(approx_equal_to(u, x));
  }
  SUBCASE("IDRs") {
    ksp::IdrsSolver<Vec<double, 8>> solver{};
    solver.Solve(u, b, *Op);
    CHECK(approx_equal_to(u, x));
  }
  SUBCASE("TFQMR") {
    ksp::TfqmrSolver<Vec<double, 8>> solver{};
    solver.Solve(u, b, *Op);
    CHECK(approx_equal_to(u, x));
  }
  SUBCASE("TFQMR1") {
    ksp::Tfqmr1Solver<Vec<double, 8>> solver{};
    solver.Solve(u, b, *Op);
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
  const auto Op = ksp::MakeOperator<Vec<double, 8>>(
      [A](auto& y, const auto& z) { y = A * z; });
  auto Pre = std::make_unique<ksp::IdentityPreconditioner<Vec<double, 8>>>();
  Vec<double, 8> u{};
  SUBCASE("BiCGStab Left") {
    ksp::BiCgStabSolver<Vec<double, 8>> solver{};
    solver.PreSide = ksp::PreconditionerSide::Left;
    solver.PreOp = std::move(Pre);
    solver.Solve(u, b, *Op);
    CHECK(approx_equal_to(u, x));
  }
  SUBCASE("BiCGStab Right") {
    ksp::BiCgStabSolver<Vec<double, 8>> solver{};
    solver.PreSide = ksp::PreconditionerSide::Right;
    solver.PreOp = std::move(Pre);
    solver.Solve(u, b, *Op);
    CHECK(approx_equal_to(u, x));
  }
  SUBCASE("BiCGStabL") {
    // Looks like BiCgStabL has no preconditioner side option.
    ksp::BiCgStabLSolver<Vec<double, 8>> solver{};
    solver.PreOp = std::move(Pre);
    solver.Solve(u, b, *Op);
    CHECK(approx_equal_to(u, x));
  }
  SUBCASE("CG") {
    ksp::CgSolver<Vec<double, 8>> solver{};
    solver.PreOp = std::move(Pre);
    solver.Solve(u, b, *Op);
    CHECK(approx_equal_to(u, x));
  }
  SUBCASE("CGS Left") {
    ksp::CgsSolver<Vec<double, 8>> solver{};
    solver.PreSide = ksp::PreconditionerSide::Left;
    solver.PreOp = std::move(Pre);
    solver.Solve(u, b, *Op);
    CHECK(approx_equal_to(u, x));
  }
  SUBCASE("CGS Right") {
    ksp::CgsSolver<Vec<double, 8>> solver{};
    solver.PreSide = ksp::PreconditionerSide::Right;
    solver.PreOp = std::move(Pre);
    solver.Solve(u, b, *Op);
    CHECK(approx_equal_to(u, x));
  }
  SUBCASE("GMRES Left") {
    ksp::GmresSolver<Vec<double, 8>> solver{};
    solver.PreSide = ksp::PreconditionerSide::Left;
    solver.PreOp = std::move(Pre);
    solver.Solve(u, b, *Op);
    CHECK(approx_equal_to(u, x));
  }
  SUBCASE("GMRES Right") {
    ksp::GmresSolver<Vec<double, 8>> solver{};
    solver.PreSide = ksp::PreconditionerSide::Right;
    solver.PreOp = std::move(Pre);
    solver.Solve(u, b, *Op);
    CHECK(approx_equal_to(u, x));
  }
  SUBCASE("FGMRES Left") {
    ksp::FgmresSolver<Vec<double, 8>> solver{};
    solver.PreSide = ksp::PreconditionerSide::Left;
    solver.PreOp = std::move(Pre);
    solver.Solve(u, b, *Op);
    CHECK(approx_equal_to(u, x));
  }
  SUBCASE("FGMRES Right") {
    ksp::FgmresSolver<Vec<double, 8>> solver{};
    solver.PreSide = ksp::PreconditionerSide::Right;
    solver.PreOp = std::move(Pre);
    solver.Solve(u, b, *Op);
    CHECK(approx_equal_to(u, x));
  }
  SUBCASE("IDRs Left") {
    ksp::IdrsSolver<Vec<double, 8>> solver{};
    solver.PreSide = ksp::PreconditionerSide::Left;
    solver.PreOp = std::move(Pre);
    solver.Solve(u, b, *Op);
    CHECK(approx_equal_to(u, x));
  }
  SUBCASE("IDRs Right") {
    ksp::IdrsSolver<Vec<double, 8>> solver{};
    solver.PreSide = ksp::PreconditionerSide::Right;
    solver.PreOp = std::move(Pre);
    solver.Solve(u, b, *Op);
    CHECK(approx_equal_to(u, x));
  }
  SUBCASE("TFQMR Left") {
    ksp::TfqmrSolver<Vec<double, 8>> solver{};
    solver.PreSide = ksp::PreconditionerSide::Left;
    solver.PreOp = std::move(Pre);
    solver.Solve(u, b, *Op);
    CHECK(approx_equal_to(u, x));
  }
  SUBCASE("TFQMR Right") {
    ksp::TfqmrSolver<Vec<double, 8>> solver{};
    solver.PreSide = ksp::PreconditionerSide::Right;
    solver.PreOp = std::move(Pre);
    solver.Solve(u, b, *Op);
    CHECK(approx_equal_to(u, x));
  }
  SUBCASE("TFQMR1 Left") {
    ksp::Tfqmr1Solver<Vec<double, 8>> solver{};
    solver.PreSide = ksp::PreconditionerSide::Left;
    solver.PreOp = std::move(Pre);
    solver.Solve(u, b, *Op);
    CHECK(approx_equal_to(u, x));
  }
  SUBCASE("TFQMR1 Right") {
    ksp::Tfqmr1Solver<Vec<double, 8>> solver{};
    solver.PreSide = ksp::PreconditionerSide::Right;
    solver.PreOp = std::move(Pre);
    solver.Solve(u, b, *Op);
    CHECK(approx_equal_to(u, x));
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
