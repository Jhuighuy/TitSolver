/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <iostream>

#include "tit/core/print.hpp"
#include "tit/prop/spec.hpp"

auto main() -> int {
  using namespace tit;

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  const auto spec =
      tit::prop::AppSpec{}
          .name("test")
          .description("Test application.")
          .field("flag_1", "Required flag.", prop::BoolSpec{})
          .field("flag_2",
                 "Optional flag.",
                 prop::BoolSpec{}.default_value(true))
          .field("int_1",
                 "Required integer with bounds.",
                 prop::IntSpec{}.min(1))
          .field("optional_float",
                 "Optional float with bounds.",
                 prop::RealSpec{}.max(10.0).default_value(0.5))
          .field("optional_string",
                 "Optional string.",
                 prop::StringSpec{}.default_value("hello"))
          .field("required_enum",
                 "Required enum.",
                 prop::EnumSpec{}
                     .option("a", "Option 1")
                     .option("b", "Option 2")
                     .option("c", "Option 3"))
          .field(
              "record",
              "Record with required and optional fields.",
              prop::RecordSpec{}
                  .field("required_field", "Required field.", prop::IntSpec{})
                  .field("optional_field", "Optional field.", prop::IntSpec{}))
          .field(
              "array",
              "Array of variants of records.",
              prop::ArraySpec{}.item(
                  prop::VariantSpec{}
                      .option("circle",
                              "Circle shape.",
                              prop::RecordSpec{}.field("radius",
                                                       "Radius.",
                                                       prop::IntSpec{}))
                      .option("rectangle",
                              "Rectangle shape.",
                              prop::RecordSpec{}
                                  .field("width", "Width.", prop::IntSpec{})
                                  .field("height", "Height.", prop::IntSpec{}))
                      .option("triangle",
                              "Triangle shape.",
                              prop::RecordSpec{}
                                  .field("width", "Width.", prop::IntSpec{})
                                  .field("height", "Height.", prop::IntSpec{})
                                  .field("base", "Base.", prop::IntSpec{}))));

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  println_separator();
  println();

  prop::spec_dump_json(spec, std::cout);

  println();
  println_separator();
  println();

  prop::spec_dump_yaml_config(spec, std::cout);

  println();
  println_separator();

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  return 0;
}
