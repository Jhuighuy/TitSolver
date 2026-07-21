/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, under the MIT License.
 * See /LICENSE.md for license information. SPDX-License-Identifier: MIT
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "tit/prop/spec.hpp"
#include "tit/prop/unit.hpp"
#include "titwcsph/case.hpp"

namespace tit::wcsph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Default values below mirror the hardcoded dam break setup in `wcsph.cpp`.
// TODO(case): the solver currently consumes only a minimal subset of these
// parameters; extend the wiring as the hardcoded setup is dismantled.
auto make_case_spec() -> prop::SpecPtr {
  return prop::RecordSpec{}
      .field("schema",
             "Schema Version",
             prop::IntSpec{}
                 .range(case_schema_version, case_schema_version)
                 .default_value(case_schema_version))
      .field("simulation",
             "Simulation",
             prop::RecordSpec{}
                 .field("title",
                        "Title",
                        prop::StringSpec{}.default_value("Untitled Case"))
                 // End time is measured in scaled time units `t * sqrt(g / H)`,
                 // hence no unit.
                 .field("end_time",
                        "End Time",
                        prop::RealSpec{}.min(0.0).default_value(10.0))
                 .field("gravity",
                        "Gravity",
                        prop::RealSpec{}.min(0.0).default_value(9.81).unit(
                            prop::Unit{"m/s^2"})))
      .field("fluid",
             "Fluid",
             prop::RecordSpec{}
                 .field("density",
                        "Density",
                        prop::RealSpec{}.min(0.0).default_value(1000.0).unit(
                            prop::Unit{"kg/m^3"}))
                 .field("viscosity",
                        "Dynamic Viscosity",
                        prop::RealSpec{}.min(0.0).default_value(0.001).unit(
                            prop::Unit{"Pa*s"})))
      .field(
          "geometry",
          "Geometry",
          prop::VariantSpec{}
              .option(
                  "dam_break",
                  "Dam Break",
                  prop::RecordSpec{}
                      .field("water_height",
                             "Water Column Height",
                             prop::RealSpec{}.min(0.0).default_value(0.6).unit(
                                 prop::Unit{"m"}))
                      .field("water_length",
                             "Water Column Length",
                             prop::RealSpec{}.min(0.0).default_value(1.2).unit(
                                 prop::Unit{"m"}))
                      .field(
                          "pool_width",
                          "Pool Width",
                          prop::RealSpec{}.min(0.0).default_value(3.2196).unit(
                              prop::Unit{"m"}))
                      .field("pool_height",
                             "Pool Height",
                             prop::RealSpec{}.min(0.0).default_value(2.4).unit(
                                 prop::Unit{"m"}))
                      .field(
                          "spacing",
                          "Particle Spacing",
                          prop::RealSpec{}.min(0.0).default_value(0.0075).unit(
                              prop::Unit{"m"})))
              .default_value("dam_break"))
      .field(
          "output",
          "Output",
          prop::RecordSpec{}.field("interval",
                                   "Write Interval",
                                   prop::IntSpec{}.min(1).default_value(100)))
      .box();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::wcsph
