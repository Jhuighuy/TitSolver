/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <string_view>

#include "tit/core/str_utils.hpp"

#include "tit/py/cast.hpp"
#include "tit/py/error.hpp"
#include "tit/py/number.hpp"
#include "tit/py/object.hpp"
#include "tit/py/sequence.hpp"

namespace tit::py::impl {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto Converter<bool>::object(bool value) -> Object {
  return Bool{value};
}

auto Converter<bool>::extract(const Object& obj) -> bool {
  return expect<Bool>(obj).val();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto Converter<long long>::object(long long value) -> Object {
  return Int{value};
}

auto Converter<long long>::extract(const Object& obj) -> long long {
  return expect<Int>(obj).val();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto Converter<double>::object(double value) -> Object {
  return Float{value};
}

auto Converter<double>::extract(const Object& obj) -> double {
  return expect<Float>(obj).val();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto Converter<std::string_view>::object(std::string_view value) -> Object {
  return Str{value};
}

auto Converter<std::string_view>::extract(const Object& obj) -> CStrView {
  return expect<Str>(obj).val();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::py::impl
