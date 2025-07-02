/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *\
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <fstream>
#include <string>
#include <vector>

#include "tit/core/cmd.hpp"
#include "tit/core/print.hpp"
#include "tit/core/sys/utils.hpp"

#include "tit/graph/graph.hpp"
#include "tit/graph/partition/multilevel.hpp"

namespace tit {
namespace {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

auto run_test(CmdArgs /*args*/) -> int {
  const auto input_file_name =
      std::string{*get_env("TEST_DATA_DIR")} + "/graphs/graph_1.txt";
  constexpr const auto* output_file_name = "output.txt";

  graph::WeightedGraph graph{};

  println("Reading graph from file: '{}'.", input_file_name);
  graph.read(input_file_name);

  println("Partitioning graph...");
  std::vector<graph::part_t> parts(graph.num_nodes());
  graph::multilevel_partition(graph, parts, 4);

  println("Writing parts to file: '{}'.", output_file_name);
  std::ofstream file(output_file_name);
  file << parts.size() << "\n";
  for (const auto part : parts) {
    file << part << "\n";
  }

  return 0;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace
} // namespace tit

TIT_IMPLEMENT_MAIN(run_test)
