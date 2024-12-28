/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of the Tit Solver project, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <ranges>

#include "tit/core/basic_types.hpp"
#include "tit/core/str_utils.hpp"

#include "tit/python/nanobind.hpp"

namespace tit::python {

namespace nb = nanobind;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Bind the range view class.
template<std::ranges::view Range>
void bind_range(nb::module_& m, CStrView name) {
  // Bind the range class and define the iterator.
  nb::class_<Range> cls(m, name.c_str());
  cls.def(
      "__iter__",
      [](const Range& self) {
        return nb::make_iterator(nb::type<Range>(),
                                 "iterator",
                                 std::ranges::begin(self),
                                 std::ranges::end(self));
      },
      nb::keep_alive<0, 1>());

  // Define the length of the range if it is a sized range.
  if constexpr (std::ranges::sized_range<Range>) {
    cls.def("__len__", [](const Range& self) { //
      return std::ranges::ssize(self);
    });
  }

  // Define the indexing operator if it is a random access range.
  if constexpr (std::ranges::random_access_range<Range>) {
    cls.def("__getitem__", [](const Range& self, ssize_t index) {
      const auto size = std::ranges::ssize(self);
      if (index < 0) index += size;
      if (index < 0 || index >= size) throw nb::index_error();
      return self[index];
    });
  }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::python
