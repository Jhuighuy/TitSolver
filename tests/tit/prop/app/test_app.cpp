/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// Behavioral test app: parses arguments, prints the resulting property tree
// as indented JSON to stdout. Covers all property system features.

#include "tit/core/print.hpp"

#include "tit/prop/app.hpp"
#include "tit/prop/spec.hpp"
#include "tit/prop/tree.hpp"

namespace tit::prop {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class TestApp final : public App {
public:

  auto spec() const -> AppSpec override {
    return AppSpec{}
        .name("prop_app")
        .description("Property system behavioral test application.")
        .field("debug", "Enable debug mode", BoolSpec{}.default_value(false))
        .field("workers",
               "Number of worker threads",
               IntSpec{}.range(1, 64).default_value(4))
        .field("threshold",
               "Threshold value",
               RealSpec{}.range(0.0, 1.0).default_value(0.5))
        .field("output",
               "Output file path",
               StringSpec{}.default_value("./output"))
        .field("mode",
               "Run mode",
               EnumSpec{}
                   .option("fast", "Fast mode")
                   .option("safe", "Safe mode")
                   .option("debug", "Debug mode")
                   .default_value("fast"))
        .field("physics",
               "Physics settings",
               RecordSpec{}
                   .field("gravity",
                          "Gravitational acceleration",
                          RealSpec{}.default_value(9.81))
                   .field("density",
                          "Reference density",
                          RealSpec{}.range(0.1, 1000.0).default_value(1.0))
                   .field("config",
                          "Physics sub-config",
                          RecordSpec{}.field("substeps",
                                             "Substep count",
                                             IntSpec{}.default_value(2))))
        .field("tags", "String tags", ArraySpec{}.item(StringSpec{}))
        .field("shape",
               "Active shape",
               VariantSpec{}
                   .option("circle",
                           "Circle",
                           RecordSpec{}.field("radius",
                                              "Circle radius",
                                              RealSpec{}.default_value(1.0)))
                   .option("rect",
                           "Rectangle",
                           RecordSpec{}
                               .field("width",
                                      "Rectangle width",
                                      RealSpec{}.default_value(2.0))
                               .field("height",
                                      "Rectangle height",
                                      RealSpec{}.default_value(1.5)))
                   .default_value("circle"));
  }

  void run(Tree props) override {
    println("{}", tree_dump_json(props));
  }

}; // class TestApp

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit::prop

TIT_IMPLEMENT_APP(tit::prop::TestApp)

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
