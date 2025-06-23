/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <random>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/math.hpp"
#include "tit/core/utils.hpp"

namespace tit::ksp {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Vector space.
//

/// Vector space, encapsulates the vector operations.
template<class Num_, class Vec_>
class Space {
public:

  /// Number type.
  using Num = Num_;

  /// Vector type.
  using Vec = Vec_;

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Initialize 𝒙 with the shape of 𝒚.
  constexpr void init(Vec& x_vec, const Vec& y_vec) const =
      delete; // Must be implemented in derived class.

  /// Compute: 𝒙ᵢ ← 𝑎.
  constexpr void fill(Vec& x_vec, const Num& a) const =
      delete; // Must be implemented in derived class.

  /// Compute: 𝒙ᵢ ← 0.
  constexpr void clear(this const auto& self, Vec& x_vec) {
    self.fill(x_vec, Num{0});
  }

  /// Compute: 𝒙ᵢ ← 𝘙𝘢𝘯𝘥𝘰𝘮(𝘴𝘦𝘦𝘥).
  constexpr void rand_fill(Vec& x_vec, uint64_t seed) const =
      delete; // Must be implemented in derived class.

  /// Compute: 𝒙 ← 𝒚.
  constexpr void copy(Vec& x_vec, const Vec& y_vec) const =
      delete; // Must be implemented in derived class.

  /// Compute: 𝒙 ⇆ 𝒚.
  constexpr void swap(Vec& x_vec, Vec& y_vec) const noexcept {
    std::swap(x_vec, y_vec);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Compute 𝒙 ← 𝑎⋅𝒚.
  constexpr void scale(Vec&       x_vec,
                       const Vec& y_vec,
                       const Num& a) const =
      delete; // Must be implemented in derived class.

  /// Compute 𝒙 ← 𝑎⋅𝒙.
  constexpr void scale_assign(this const auto& self, Vec& x_vec, const Num& a) {
    self.scale(x_vec, x_vec, a);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Compute: 𝒙 ← 𝒚 + 𝒛.
  constexpr void add(Vec&       x_vec,
                     const Vec& y_vec,
                     const Vec& z_vec) const =
      delete; // Must be implemented in derived class.

  /// Compute: 𝒙 ← 𝒚 + 𝑏⋅𝒛.
  constexpr void add(this const auto& self,
                     Vec&             x_vec,
                     const Vec&       y_vec,
                     const Vec&       z_vec,
                     const Num&       b) {
    TIT_ASSERT(&x_vec != &y_vec, "X and Y vectors must be different!");
    self.scale(x_vec, z_vec, b);
    self.add_assign(x_vec, y_vec);
  }

  /// Compute: 𝒙 ← 𝑎⋅𝒚 + 𝑏⋅𝒛.
  constexpr void add(this const auto& self,
                     Vec&             x_vec,
                     const Vec&       y_vec,
                     const Num&       a,
                     const Vec&       z_vec,
                     const Num&       b) {
    if (abs(a) > abs(b)) {
      self.add(x_vec, y_vec, z_vec, b / a);
      self.scale_assign(self, x_vec, a);
    } else {
      self.add(x_vec, z_vec, y_vec, a / b);
      self.scale_assign(self, x_vec, b);
    }
  }

  /// Compute: 𝒙 ← 𝒙 + 𝒚.
  constexpr void add_assign(this const auto& self,
                            Vec&             x_vec,
                            const Vec&       y_vec) {
    self.add(x_vec, x_vec, y_vec);
  }

  /// Compute: 𝒙 ← 𝒙 + 𝑎⋅𝒚.
  constexpr void add_assign(this const auto& self,
                            Vec&             x_vec,
                            const Vec&       y_vec,
                            const Num&       a) {
    self.add(x_vec, x_vec, y_vec, a);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Compute: 𝒙 ← 𝒚 - 𝒛.
  constexpr void sub(Vec&       x_vec,
                     const Vec& y_vec,
                     const Vec& z_vec) const =
      delete; // Must be implemented in derived class.

  /// Compute: 𝒙 ← 𝒚 - 𝑏⋅𝒛.
  constexpr void sub(this const auto& self,
                     Vec&             x_vec,
                     const Vec&       y_vec,
                     const Vec&       z_vec,
                     const Num&       b) {
    TIT_ASSERT(&x_vec != &y_vec, "X and Y vectors must be different!");
    self.scale(x_vec, z_vec, -b);
    self.add(x_vec, y_vec, x_vec);
  }

  /// Compute: 𝒙 ← 𝑎⋅𝒚 - 𝑏⋅𝒛.
  constexpr void sub(this const auto& self,
                     Vec&             x_vec,
                     const Vec&       y_vec,
                     const Num&       a,
                     const Vec&       z_vec,
                     const Num&       b) {
    if (abs(a) > abs(b)) {
      self.sub(x_vec, y_vec, z_vec, b / a);
      self.scale_assign(self, x_vec, a);
    } else {
      self.sub(x_vec, z_vec, y_vec, a / b);
      self.scale_assign(self, x_vec, b);
    }
  }

  /// Compute: 𝒙 ← 𝒙 - 𝒚.
  constexpr void sub_assign(this const auto& self,
                            Vec&             x_vec,
                            const Vec&       y_vec) {
    self.sub(x_vec, x_vec, y_vec);
  }

  /// Compute: 𝒙 ← 𝒙 - 𝑎⋅𝒚.
  constexpr void sub_assign(this const auto& self,
                            Vec&             x_vec,
                            const Vec&       y_vec,
                            const Num&       a) {
    self.sub(x_vec, x_vec, y_vec, a);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Compute: <𝒙⋅𝒚>.
  constexpr auto dot(const Vec& x_vec, const Vec& y_vec) const
      -> Num = delete; // Must be implemented in derived class.

  /// Compute: <𝒙⋅𝒙>.
  constexpr auto norm2(this const auto& self, const Vec& x_vec) -> Num {
    return self.dot(x_vec, x_vec);
  }

  /// Compute: ‖𝒙‖.
  constexpr auto norm(this const auto& self, const Vec& x_vec) -> Num {
    return sqrt(self.norm2(x_vec));
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

}; // class Space

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Euclidean vector space.
//

/// Euclidean vector space, implements the vector operations in terms of the
/// vector element access methods.
template<class Num, class Vec>
class EuclideanSpace : public Space<Num, Vec> {
public:

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Get the vector element at the given index.
  /// @{
  constexpr auto at(Vec& vec, const size_t index)
      -> Num& = delete; // Must be implemented in derived class.
  constexpr auto at(const Vec& vec, const size_t index) const
      -> const Num& = delete; // Must be implemented in derived class.
  /// @}

  /// Get the dimension of the vector.
  constexpr auto dim(const Vec& x_vec) const
      -> size_t = delete; // Must be implemented in derived class.

  /// Ensure that the vectors have the same dimension and return it.
  constexpr auto dims(this const auto& self,
                      const Vec&       vec,
                      const auto&... rest_vecs) -> size_t {
    TIT_ASSERT(is_all_of(self.dim(vec), self.dim(rest_vecs)...),
               "Vectors must have the same dimension!");
    return self.dim(vec);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Compute: 𝒙ᵢ ← 𝑎.
  constexpr void fill(this const auto& self, Vec& x_vec, const Num& a) {
    for (size_t i = 0; i < self.dim(x_vec); ++i) {
      self.at(x_vec, i) = a;
    }
  }

  /// Compute: 𝒙ᵢ ← 𝘙𝘢𝘯𝘥𝘰𝘮(𝘴𝘦𝘦𝘥).
  constexpr void rand_fill(this const auto& self, Vec& x_vec, uint64_t seed)
    requires std::floating_point<Num>
  {
    std::mt19937_64                rand{seed};
    std::uniform_real_distribution dist{Num{-1}, Num{1}};
    for (size_t i = 0; i < self.dim(x_vec); ++i) {
      self.at(x_vec, i) = dist(rand);
    }
  }

  /// Compute: 𝒙 ← 𝒚.
  constexpr void copy(this const auto& self, Vec& x_vec, const Vec& y_vec) {
    for (size_t i = 0; i < self.dims(x_vec, y_vec); ++i) {
      self.at(x_vec, i) = self.at(y_vec, i);
    }
  }

  /// Compute: 𝒙 ⇆ 𝒚.
  constexpr void swap(this const auto& self, Vec& x_vec, Vec& y_vec) noexcept {
    for (size_t i = 0; i < self.dims(x_vec, y_vec); ++i) {
      std::swap(self.at(x_vec, i), self.at(y_vec, i));
    }
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Compute: 𝒙 ← 𝑎⋅𝒚.
  constexpr void scale(this const auto& self,
                       Vec&             x_vec,
                       const Vec&       y_vec,
                       const Num&       a) {
    for (size_t i = 0; i < self.dims(x_vec, y_vec); ++i) {
      self.at(x_vec, i) = a * self.at(y_vec, i);
    }
  }

  /// Compute: 𝒙 ← 𝑎⋅𝒙.
  constexpr void scale_assign(this const auto& self, Vec& x_vec, const Num& a) {
    for (size_t i = 0; i < self.dim(x_vec); ++i) {
      self.at(x_vec, i) *= a;
    }
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Compute: 𝒙 ← 𝒚 + 𝒛.
  constexpr void add(this const auto& self,
                     Vec&             x_vec,
                     const Vec&       y_vec,
                     const Vec&       z_vec) {
    for (size_t i = 0; i < self.dims(x_vec, y_vec, z_vec); ++i) {
      self.at(x_vec, i) = self.at(y_vec, i) + self.at(z_vec, i);
    }
  }

  /// Compute: 𝒙 ← 𝒚 + 𝑏⋅𝒛.
  constexpr void add(this const auto& self,
                     Vec&             x_vec,
                     const Vec&       y_vec,
                     const Vec&       z_vec,
                     const Num&       b) {
    for (size_t i = 0; i < self.dims(x_vec, y_vec, z_vec); ++i) {
      self.at(x_vec, i) = self.at(y_vec, i) + b * self.at(z_vec, i);
    }
  }

  /// Compute: 𝒙 ← 𝑎⋅𝒚 + 𝑏⋅𝒛.
  constexpr void add(this const auto& self,
                     Vec&             x_vec,
                     const Vec&       y_vec,
                     const Num&       a,
                     const Vec&       z_vec,
                     const Num&       b) {
    for (size_t i = 0; i < self.dims(x_vec, y_vec, z_vec); ++i) {
      self.at(x_vec, i) = a * self.at(y_vec, i) + b * self.at(z_vec, i);
    }
  }

  /// Compute: 𝒙 ← 𝒙 + 𝒚.
  constexpr void add_assign(this const auto& self,
                            Vec&             x_vec,
                            const Vec&       y_vec) {
    for (size_t i = 0; i < self.dims(x_vec, y_vec); ++i) {
      self.at(x_vec, i) += self.at(y_vec, i);
    }
  }

  /// Compute: 𝒙 ← 𝒙 + 𝑎⋅𝒚.
  constexpr void add_assign(this const auto& self,
                            Vec&             x_vec,
                            const Vec&       y_vec,
                            const Num&       a) {
    for (size_t i = 0; i < self.dims(x_vec, y_vec); ++i) {
      self.at(x_vec, i) += a * self.at(y_vec, i);
    }
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Compute: 𝒙 ← 𝒚 - 𝒛.
  constexpr void sub(this const auto& self,
                     Vec&             x_vec,
                     const Vec&       y_vec,
                     const Vec&       z_vec) {
    for (size_t i = 0; i < self.dims(x_vec, y_vec, z_vec); ++i) {
      self.at(x_vec, i) = self.at(y_vec, i) - self.at(z_vec, i);
    }
  }

  /// Compute: 𝒙 ← 𝒚 - 𝑏⋅𝒛.
  constexpr void sub(this const auto& self,
                     Vec&             x_vec,
                     const Vec&       y_vec,
                     const Vec&       z_vec,
                     const Num&       b) {
    for (size_t i = 0; i < self.dims(x_vec, y_vec, z_vec); ++i) {
      self.at(x_vec, i) = self.at(y_vec, i) - b * self.at(z_vec, i);
    }
  }

  /// Compute: 𝒙 ← 𝑎⋅𝒚 - 𝑏⋅𝒛.
  constexpr void sub(this const auto& self,
                     Vec&             x_vec,
                     const Vec&       y_vec,
                     const Num&       a,
                     const Vec&       z_vec,
                     const Num&       b) {
    for (size_t i = 0; i < self.dims(x_vec, y_vec, z_vec); ++i) {
      self.at(x_vec, i) = a * self.at(y_vec, i) - b * self.at(z_vec, i);
    }
  }

  /// Compute: 𝒙 ← 𝒙 - 𝒚.
  constexpr void sub_assign(this const auto& self,
                            Vec&             x_vec,
                            const Vec&       y_vec) {
    for (size_t i = 0; i < self.dims(x_vec, y_vec); ++i) {
      self.at(x_vec, i) -= self.at(y_vec, i);
    }
  }

  /// Compute: 𝒙 ← 𝒙 - 𝑎⋅𝒚.
  constexpr void sub_assign(this const auto& self,
                            Vec&             x_vec,
                            const Vec&       y_vec,
                            const Num&       a) {
    for (size_t i = 0; i < self.dims(x_vec, y_vec); ++i) {
      self.at(x_vec, i) -= a * self.at(y_vec, i);
    }
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Compute: <𝒙⋅𝒚>.
  constexpr auto dot(this const auto& self, const Vec& x_vec, const Vec& y_vec)
      -> Num {
    Num r{0};
    for (size_t i = 0; i < self.dims(x_vec, y_vec); ++i) {
      r += self.at(x_vec, i) * self.at(y_vec, i);
    }
    return r;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

}; // class EuclideanSpace

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::ksp
