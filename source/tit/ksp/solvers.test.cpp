/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "tit/core/basic_types.hpp"
#include "tit/core/mat.hpp"
#include "tit/core/vec.hpp"

#include "tit/ksp/mapping.hpp"
#include "tit/ksp/solvers.hpp"
#include "tit/ksp/space.hpp"

#include "tit/testing/test.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

template<class Num, size_t Dim>
class VecSpace final : public ksp::Space<Num, Vec<Num, Dim>> {
public:

  constexpr auto dot(const Vec<Num, Dim>& x_vec,
                     const Vec<Num, Dim>& y_vec) const {
    return tit::dot(x_vec, y_vec);
  }

  constexpr void init(Vec<Num, Dim>& x_vec, const Vec<Num, Dim>& y_vec) const {
    x_vec = y_vec;
  }

  constexpr void copy(Vec<Num, Dim>& x_vec, const Vec<Num, Dim>& y_vec) const {
    x_vec = y_vec;
  }

  constexpr void fill(Vec<Num, Dim>& x_vec, const Num& a) const {
    x_vec = Vec<Num, Dim>(a);
  }
  constexpr void rand_fill(Vec<Num, Dim>& x_vec) const {
    static Num a = 0.0;
    for (size_t i = 0; i < Dim; ++i) {
      x_vec[i] = a * (a - 1.0);
      a += 1.0;
    }
  }

  constexpr void scale(Vec<Num, Dim>&       x_vec,
                       const Vec<Num, Dim>& y_vec,
                       const Num&           a) const {
    x_vec = a * y_vec;
  }
  constexpr void scale_assign(Vec<Num, Dim>& x_vec, const Num& a) const {
    x_vec *= a;
  }

  constexpr void add(Vec<Num, Dim>&       x_vec,
                     const Vec<Num, Dim>& y_vec,
                     const Vec<Num, Dim>& z_vec) const {
    x_vec = y_vec + z_vec;
  }
  constexpr void add(Vec<Num, Dim>&       x_vec,
                     const Vec<Num, Dim>& y_vec,
                     const Vec<Num, Dim>& z_vec,
                     const Num&           b) const {
    x_vec = y_vec + b * z_vec;
  }
  constexpr void add(Vec<Num, Dim>&       x_vec,
                     const Vec<Num, Dim>& y_vec,
                     const Num&           a,
                     const Vec<Num, Dim>& z_vec,
                     const Num&           b) const {
    x_vec = a * y_vec + b * z_vec;
  }
  constexpr void add_assign(Vec<Num, Dim>&       x_vec,
                            const Vec<Num, Dim>& y_vec) const {
    x_vec += y_vec;
  }
  constexpr void add_assign(Vec<Num, Dim>&       x_vec,
                            const Vec<Num, Dim>& y_vec,
                            const Num&           a) const {
    x_vec += a * y_vec;
  }

  constexpr void sub(Vec<Num, Dim>&       x_vec,
                     const Vec<Num, Dim>& y_vec,
                     const Vec<Num, Dim>& z_vec) const {
    x_vec = y_vec - z_vec;
  }
  constexpr void sub(Vec<Num, Dim>&       x_vec,
                     const Vec<Num, Dim>& y_vec,
                     const Vec<Num, Dim>& z_vec,
                     const Num&           b) const {
    x_vec = y_vec - b * z_vec;
  }
  constexpr void sub(Vec<Num, Dim>&       x_vec,
                     const Vec<Num, Dim>& y_vec,
                     const Num&           a,
                     const Vec<Num, Dim>& z_vec,
                     const Num&           b) const {
    x_vec = a * y_vec - b * z_vec;
  }
  constexpr void sub_assign(Vec<Num, Dim>& x_vec, const Vec<Num, Dim>& y_vec) {
    x_vec -= y_vec;
  }
  constexpr void sub_assign(Vec<Num, Dim>&       x_vec,
                            const Vec<Num, Dim>& y_vec,
                            const Num&           a) const {
    x_vec -= a * y_vec;
  }

}; // class VecSpace

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

template<class Num, size_t Dim>
class MatMapping final : public ksp::SquareMapping<VecSpace<Num, Dim>> {
public:

  using Space = VecSpace<Num, Dim>;

  constexpr MatMapping(const Space& space, Mat<Num, Dim> mat) noexcept
      : ksp::SquareMapping<VecSpace<Num, Dim>>{space}, mat_{std::move(mat)} {}

  constexpr void apply(Space::Vec& y_vec, const Space::Vec& x_vec) const {
    y_vec = mat_ * x_vec;
  }

private:

  Mat<Num, Dim> mat_;

}; // class MatMapping

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TEST_CASE("ksp::IterativeSolver") {
  const Mat M{
      {2.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
      {1.0, 2.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0},
      {0.0, 1.0, 2.0, 1.0, 0.0, 0.0, 0.0, 0.0},
      {0.0, 0.0, 1.0, 2.0, 1.0, 0.0, 0.0, 0.0},
      {0.0, 0.0, 0.0, 1.0, 2.0, 1.0, 0.0, 0.0},
      {0.0, 0.0, 0.0, 0.0, 1.0, 2.0, 1.0, 0.0},
      {0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 2.0, 1.0},
      {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 2.0},
  };
  const VecSpace<double, 8>   V{};
  const MatMapping<double, 8> A{V, M};
  const ksp::Identity         P{V};

  const Vec      u_vec{1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0};
  Vec            b_vec = M * u_vec;
  Vec<double, 8> x_vec{};

  SUBCASE("BiCGStab") {
    const ksp::BiCGStabSolver solver{A, P};
    solver.apply(x_vec, b_vec);
    CHECK_APPROX_EQ(x_vec, u_vec);
  }
  SUBCASE("BiCGStabL") {
    const ksp::BiCGStabLSolver solver{A, P};
    solver.apply(x_vec, b_vec);
    CHECK_APPROX_EQ(x_vec, u_vec);
  }
  SUBCASE("CG") {
    const ksp::CGSolver solver{A, P};
    solver.apply(x_vec, b_vec);
    CHECK_APPROX_EQ(x_vec, u_vec);
  }
  SUBCASE("CGS") {
    const ksp::CGSSolver solver{A, P};
    solver.apply(x_vec, b_vec);
    CHECK_APPROX_EQ(x_vec, u_vec);
  }
  SUBCASE("GMRES") {
    const ksp::GMRESSolver solver{A, P};
    solver.apply(x_vec, b_vec);
    CHECK_APPROX_EQ(x_vec, u_vec);
  }
  SUBCASE("FGMRES") {
    const ksp::FGMRESSolver solver{A, P};
    solver.apply(x_vec, b_vec);
    CHECK_APPROX_EQ(x_vec, u_vec);
  }
  SUBCASE("IDRS") {
    const ksp::IDRSSolver solver{A, P};
    solver.apply(x_vec, b_vec);
    CHECK_APPROX_EQ(x_vec, u_vec);
  }
  SUBCASE("TFQMR") {
    const ksp::TFQMRSolver solver{A, P};
    solver.apply(x_vec, b_vec);
    CHECK_APPROX_EQ(x_vec, u_vec);
  }
  SUBCASE("TFQMR1") {
    const ksp::TFQMR1Solver solver{A, P};
    solver.apply(x_vec, b_vec);
    CHECK_APPROX_EQ(x_vec, u_vec);
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit
