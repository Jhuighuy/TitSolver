# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
# Commercial use, including SaaS, requires a separate license, see /LICENSE.md
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_test(
  NAME "tit/graph/unit"
  COMMAND "tit_graph_tests"
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_executable(
  NAME graph_coarsen_gem_test
  SOURCES "coarsen_gem_test.cpp"
  DEPENDS tit::graph
)

add_tit_test(
  NAME "tit/graph/coarsen/gem"
  COMMAND "tit_graph_coarsen_gem_test"
  INPUT_FILES "input_graph_1.txt"
  MATCH_FILES "coarsen_gem_output.txt"
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_executable(
  NAME graph_coarsen_hem_test
  SOURCES "coarsen_hem_test.cpp"
  DEPENDS tit::graph
)

add_tit_test(
  NAME "tit/graph/coarsen/hem"
  COMMAND "tit_graph_coarsen_hem_test"
  INPUT_FILES "input_graph_1.txt"
  MATCH_FILES "coarsen_hem_output.txt"
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_executable(
  NAME graph_echo_test
  SOURCES "echo_test.cpp"
  DEPENDS tit::graph
)

add_tit_test(
  NAME "tit/graph/echo"
  COMMAND "tit_graph_echo_test"
  INPUT_FILES "input_graph_1.txt"
  MATCH_FILES "echo_output.txt"
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_executable(
  NAME partition_greedy_test
  SOURCES "partition_greedy_test.cpp"
  DEPENDS tit::graph
)

add_tit_test(
  NAME "tit/graph/partition/greedy"
  COMMAND "tit_partition_greedy_test"
  INPUT_FILES "input_graph_1.txt"
  MATCH_FILES "partition_greedy_output.txt"
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

add_tit_executable(
  NAME partition_multilevel_test
  SOURCES "partition_multilevel_test.cpp"
  DEPENDS tit::graph
)

add_tit_test(
  NAME "tit/graph/partition/multilevel"
  COMMAND "tit_partition_multilevel_test"
  INPUT_FILES "input_graph_1.txt"
  MATCH_FILES "partition_multilevel_output.txt"
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
