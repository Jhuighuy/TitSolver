/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
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
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <concepts>
#include <type_traits>

namespace tit::meta {

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

/** Check that T is in Us. */
template<class T, class... Us>
inline constexpr bool in_list_v = //
    (... || std::same_as<std::remove_cvref_t<T>, std::remove_cvref_t<Us>>);

/** Check that all Ts are unique. */
template<class... Ts>
inline constexpr bool all_unique_v = true;
// clang-format off
template<class T, class... Ts>
inline constexpr bool all_unique_v<T, Ts...> =
    (!in_list_v<T, Ts...>) && all_unique_v<Ts...>;
// clang-format on

template<class... Ts>
  requires all_unique_v<Ts...>
class Set {
public:

  Set() = default;

  consteval Set(Ts...)
    requires (sizeof...(Ts) != 0)
  {}

  consteval auto operator|(Set<>) const {
    return Set<Ts...>{};
  }
  template<class U, class... Us>
  consteval auto operator|(Set<U, Us...>) const {
    if constexpr (in_list_v<U, Ts...>) return Set<Ts...>{} | Set<Us...>{};
    else return Set<Ts..., U>{} | Set<Us...>{};
  }

  template<class U>
  static consteval bool contains(U) {
    return in_list_v<U, Ts...>;
  }
};

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

template<class T>
static consteval auto _type_name_impl() {
  return __PRETTY_FUNCTION__;
}

template<class T>
inline constexpr auto type_name = _type_name_impl<T>();

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

} // namespace tit::meta

template<class T>
using required_fields_t = typename T::required_fields;

template<class ParticleView, class... Fields>
concept particle_view = true;
template<class ParticleView, class... Fields>
concept particle_cloud = true;
