/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

namespace tit {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Quaternion.
template<class Num>
class Quat final {
public:

  /// Construct a quaternion.
  constexpr Quat() = default;

  /// Construct a quaternion from the given components.
  constexpr Quat(Num x, Num y, Num z, Num w) noexcept
      : x_{x}, y_{y}, z_{z}, w_{w} {}

  /// Get the components.
  /// @{
  constexpr auto x(this auto& self) noexcept -> auto& {
    return self.x_;
  }
  constexpr auto y(this auto& self) noexcept -> auto& {
    return self.y_;
  }
  constexpr auto z(this auto& self) noexcept -> auto& {
    return self.z_;
  }
  constexpr auto w(this auto& self) noexcept -> auto& {
    return self.w_;
  }
  /// @}

  /// Multiply the quaternion.
  friend constexpr auto operator*(const Quat& a, const Quat& b) noexcept {
    return Quat{a.w() * b.x() + a.x() * b.w() + a.y() * b.z() - a.z() * b.y(),
                a.w() * b.y() + a.y() * b.w() + a.z() * b.x() - a.x() * b.z(),
                a.w() * b.z() + a.z() * b.w() + a.x() * b.y() - a.y() * b.x(),
                a.w() * b.w() - a.x() * b.x() - a.y() * b.y() - a.z() * b.z()};
  }

private:

  Num x_{};
  Num y_{};
  Num z_{};
  Num w_{};

}; // class Quat

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit
