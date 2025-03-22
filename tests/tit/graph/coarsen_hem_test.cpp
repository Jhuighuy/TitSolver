/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <vector>

#include "tit/core/cmd.hpp"
#include "tit/core/io.hpp"

#include "tit/graph/coarsen/hem.hpp"
#include "tit/graph/graph.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto run_test(CmdArgs /*args*/) -> int {
  constexpr const auto* input_file_name = "input_graph_1.txt";
  constexpr const auto* output_file_name = "coarsen_hem_output.txt";

  graph::WeightedGraph graph{};

  println("Reading graph from file: '{}'.", input_file_name);
  graph.read(input_file_name);

  println("Coarsening graph...");
  std::vector<graph::node_t> coarse_to_fine{};
  std::vector<graph::node_t> fine_to_coarse{};
  graph::WeightedGraph coarse_graph{};
  graph::coarsen_hem(graph, coarse_graph, coarse_to_fine, fine_to_coarse);

  println("Writing coarsened graph to file: '{}'.", output_file_name);
  coarse_graph.write(output_file_name);

  return 0;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit

TIT_IMPLEMENT_MAIN(run_test)
