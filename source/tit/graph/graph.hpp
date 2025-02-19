/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ *|
 * Part of BlueTit Solver, licensed under Apache 2.0 with Commons Clause.
 * Commercial use, including SaaS, requires a separate license, see /LICENSE.md
\* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <algorithm>
#include <concepts>
#include <ranges>
#include <span>
#include <tuple>
#include <utility>
#include <vector>

#include "tit/core/basic_types.hpp"
#include "tit/core/checks.hpp"
#include "tit/core/containers/multivector.hpp"
#include "tit/core/tuple_utils.hpp"
#include "tit/core/type_utils.hpp"
#include "tit/core/utils.hpp"

namespace tit::graph {

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Node type alias, for convenience.
using node_t = size_t;

/// Weight type alias, for convenience.
using weight_t = ssize_t;

/// Unit weight.
inline constexpr weight_t unit_weight = 1;

/// Part type alias, for convenience.
using part_t = size_t;

// NOLINTBEGIN(*-non-private-member-variables-in-classes)

/// Node and node weight.
struct wnode_t final {
  node_t node = npos;       ///< Node index.
  weight_t node_weight = 0; ///< Node weight.

  /// Construct an invalid weighted node.
  constexpr wnode_t() noexcept = default;

  /// Construct a weighted node.
  constexpr wnode_t(node_t node_, weight_t node_weight_) noexcept
      : node{node_}, node_weight{node_weight_} {}

  /// Construct a weighted node from a tuple-like argument.
  template<tuple_like_with_items<node_t, weight_t> Arg>
    requires different_from<Arg, wnode_t>
  constexpr wnode_t(const Arg& arg) noexcept {
    *this = arg;
  }

  /// Assign a tuple-like argument to the weighted node.
  template<tuple_like_with_items<node_t, weight_t> Arg>
    requires different_from<Arg, wnode_t>
  constexpr auto operator=(const Arg& arg) noexcept -> wnode_t& {
    std::tie(node, node_weight) = arg;
    return *this;
  }
};

/// Weighted connection.
struct wconn_t final {
  node_t neighbor = npos;   ///< Neighbor node index.
  weight_t edge_weight = 0; ///< Edge weight.

  /// Construct an invalid weighted connection.
  constexpr wconn_t() noexcept = default;

  /// Construct a weighted connection.
  constexpr wconn_t(node_t neighbor_, weight_t edge_weight_) noexcept
      : neighbor{neighbor_}, edge_weight{edge_weight_} {}

  /// Construct a weighted connection from a tuple-like argument.
  template<tuple_like_with_items<node_t, weight_t> Arg>
    requires different_from<Arg, wconn_t>
  constexpr wconn_t(const Arg& arg) noexcept {
    *this = arg;
  }

  /// Assign a tuple-like argument to the weighted connection.
  template<tuple_like_with_items<node_t, weight_t> Arg>
    requires different_from<Arg, wconn_t>
  constexpr auto operator=(const Arg& arg) noexcept -> wconn_t& {
    std::tie(neighbor, edge_weight) = arg;
    return *this;
  }
};

/// Edge.
struct edge_t final {
  node_t node = npos;     ///< Node index.
  node_t neighbor = npos; ///< Neighbor node index.

  /// Construct an invalid edge.
  constexpr edge_t() noexcept = default;

  /// Construct an edge.
  constexpr edge_t(node_t node_, node_t neighbor_) noexcept
      : node{node_}, neighbor{neighbor_} {}

  /// Construct an edge from a tuple-like argument.
  template<tuple_like_with_items<node_t, node_t> Arg>
    requires different_from<Arg, edge_t>
  constexpr explicit edge_t(const Arg& arg) noexcept {
    *this = arg;
  }

  /// Assign a tuple-like argument to the edge.
  template<tuple_like_with_items<node_t, node_t> Arg>
    requires different_from<Arg, edge_t>
  constexpr auto operator=(const Arg& arg) noexcept -> edge_t& {
    std::tie(node, neighbor) = arg;
    return *this;
  }
};

/// Weighted edge.
struct wedge_t final {
  node_t node = npos;       ///< Node index.
  node_t neighbor = npos;   ///< Neighbor node index.
  weight_t edge_weight = 0; ///< Edge weight.

  /// Construct an invalid weighted edge.
  constexpr wedge_t() noexcept = default;

  /// Construct a weighted edge.
  constexpr wedge_t(node_t node_,
                    node_t neighbor_,
                    weight_t edge_weight_) noexcept
      : node{node_}, neighbor{neighbor_}, edge_weight{edge_weight_} {}

  /// Construct a weighted edge from an edge and a weight.
  constexpr wedge_t(const edge_t& edge, weight_t edge_weight_) noexcept
      : node{edge.node}, neighbor{edge.neighbor}, edge_weight{edge_weight_} {}

  /// Construct a weighted edge from a node and weighted connection.
  constexpr wedge_t(node_t node_, const wconn_t& wconn) noexcept
      : node{node_}, neighbor{wconn.neighbor}, edge_weight{wconn.edge_weight} {}

  /// Construct a weighted edge from a tuple-like argument.
  template<tuple_like_with_items<node_t, node_t, weight_t> Arg>
    requires different_from<Arg, wedge_t>
  constexpr wedge_t(const Arg& arg) noexcept {
    *this = arg;
  }

  /// Assign a tuple-like argument to the weighted edge.
  template<tuple_like_with_items<node_t, node_t, weight_t> Arg>
    requires different_from<Arg, wedge_t>
  constexpr auto operator=(const Arg& arg) noexcept -> wedge_t& {
    std::tie(node, neighbor, edge_weight) = arg;
    return *this;
  }
};

// NOLINTEND(*-non-private-member-variables-in-classes)

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Compressed sparse adjacency graph.
template<class AdjacencyContainer>
class BaseGraph final : public AdjacencyContainer {
public:

  /// @todo Do not inherit from the container, use composition instead.
  using AdjacencyContainer::AdjacencyContainer;

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Number of graph nodes.
  constexpr auto num_nodes() const noexcept -> size_t {
    return this->size();
  }

  /// Range of the graph nodes.
  constexpr auto nodes() const noexcept {
    return std::views::iota(node_t{0}, num_nodes());
  }

  /// Node weight.
  constexpr auto weight(size_t node) const noexcept -> weight_t {
    TIT_ASSERT(node < num_nodes(), "Node index is out of range!");
    return unit_weight;
  }

  /// Node weights.
  constexpr auto weights() const noexcept {
    return std::views::repeat(unit_weight, num_nodes());
  }

  /// Range of the graph nodes and node weights.
  constexpr auto wnodes() const noexcept {
    return std::views::zip(nodes(), weights()) |
           std::views::transform([](const auto& zip) { return wnode_t{zip}; });
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Neighbors of the given node.
  constexpr auto edges(node_t node) const noexcept {
    TIT_ASSERT(node < num_nodes(), "Node index is out of range!");
    return (*this)[node];
  }

  /// Neighbors of the given node and the corresponding edge weights.
  constexpr auto wedges(size_t node) const noexcept {
    TIT_ASSERT(node < num_nodes(), "Node index is out of range!");
    return (*this)[node] | std::views::transform([node](node_t neighbor) {
             return wconn_t{neighbor, unit_weight};
           });
  }

  /// Range of the unique graph edges.
  /// @{
  constexpr auto edges() const noexcept {
    return //
        nodes() | std::views::transform([this](node_t node) {
          return this->edges(node) |
                 std::views::take_while([node](node_t neighbor) { //
                   return neighbor < node;
                 }) |
                 std::views::transform([node](node_t neighbor) {
                   /// @todo We should swap the arguments here.
                   return edge_t{neighbor, node};
                 });
        }) |
        std::views::join;
  }
  template<class Func>
  constexpr auto transform_edges(Func func) const {
    return //
        nodes() | std::views::transform([this, func](node_t node) {
          return this->edges(node) |
                 std::views::take_while([node](node_t neighbor) { //
                   return neighbor < node;
                 }) |
                 std::views::transform([node](node_t neighbor) {
                   return std::pair{neighbor, node};
                 }) |
                 std::views::transform(func);
        }) |
        std::views::join;
  }
  /// @}

  /// Range of the unique graph edges and the corresponding edge weights.
  constexpr auto wedges() const noexcept {
    return edges() | std::views::transform([](const edge_t& edge) {
             return wedge_t{edge, unit_weight};
           });
  }

}; // class BaseGraph

/// Unweighted graph type.
template<class G>
concept unweighted_graph = specialization_of<G, BaseGraph>;

/// Alias for a graph using a multivector container.
using Graph = BaseGraph<Multivector<node_t>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Compressed sparse adjacency graph with node and edge weights.
template<class AdjacencyContainer,
         class NodeWeightsContainer = std::vector<weight_t>>
class BaseWeightedGraph final {
public:

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Construct an empty weighted graph.
  constexpr BaseWeightedGraph() = default;

  /// Construct a weighted graph from containers.
  constexpr BaseWeightedGraph(AdjacencyContainer adjacency,
                              NodeWeightsContainer node_weights)
      : adjacency_{std::move(adjacency)},
        node_weights_{std::move(node_weights)} {}

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Number of graph nodes.
  constexpr auto num_nodes() const noexcept -> size_t {
    return adjacency_.size();
  }

  /// Range of the graph nodes.
  constexpr auto nodes() const noexcept {
    return std::views::iota(node_t{0}, num_nodes());
  }

  /// Node weight.
  constexpr auto weight(size_t node) const noexcept -> weight_t {
    TIT_ASSERT(node < num_nodes(), "Node index is out of range!");
    return node_weights_[node];
  }

  /// Node weights.
  constexpr auto weights() const noexcept {
    return std::span{node_weights_};
  }

  /// Range of the graph nodes and node weights.
  constexpr auto wnodes() const noexcept {
    return std::views::zip(nodes(), weights()) |
           std::views::transform([](const auto& zip) { return wnode_t{zip}; });
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Neighbors of the given node.
  constexpr auto edges(node_t node) const noexcept {
    TIT_ASSERT(node < num_nodes(), "Node index is out of range!");
    return adjacency_[node] | std::views::transform([](const wconn_t& wconn) {
             return wconn.neighbor;
           });
  }

  /// Neighbors of the given node and the corresponding edge weights.
  constexpr auto wedges(size_t node) const noexcept {
    TIT_ASSERT(node < num_nodes(), "Node index is out of range!");
    return adjacency_[node];
  }

  /// Range of the unique graph edges.
  constexpr auto edges() const noexcept {
    return edges() | std::views::transform([](const wedge_t& wedge) {
             return edge_t{wedge.node, wedge.neighbor};
           });
  }

  /// Range of the unique graph edges and the corresponding edge weights.
  constexpr auto wedges() const noexcept {
    return //
        nodes() | std::views::transform([this](node_t node) {
          return this->wedges(node) |
                 std::views::take_while([node](const wconn_t& wconn) {
                   return wconn.neighbor < node;
                 }) |
                 std::views::transform([node](const wconn_t& wconn) {
                   return wedge_t{node, wconn};
                 });
        }) |
        std::views::join;
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  /// Clear the graph.
  constexpr void clear() noexcept {
    adjacency_.clear();
    node_weights_.clear();
  }

  /// Append a new node with the given weight and adjacency.
  template<std::ranges::input_range Conns>
    requires std::constructible_from<wconn_t,
                                     std::ranges::range_reference_t<Conns>>
  constexpr void append_node(weight_t node_weight, Conns&& conns) {
    TIT_ASSUME_UNIVERSAL(Conns, conns);
    node_weights_.push_back(node_weight);
    adjacency_.append_bucket(conns);
  }

  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

private:

  AdjacencyContainer adjacency_;
  NodeWeightsContainer node_weights_;

}; // class BaseWeightedGraph

/// Weighted graph type.
template<class G>
concept weighted_graph = specialization_of<G, BaseWeightedGraph>;

/// Alias for a weighted graph using a multivector container.
using WeightedGraph = BaseWeightedGraph<Multivector<wconn_t>>;

/// Alias for a weighted graph with a capped number of edges.
template<size_t MaxNumEdges>
using CapWeightedGraph =
    BaseWeightedGraph<CapMultivector<wconn_t, MaxNumEdges>>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Graph type.
template<class G>
concept graph = unweighted_graph<G> || weighted_graph<G>;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

/// Node predicate.
template<class NodePred>
concept node_predicate = std::predicate<NodePred, node_t>;

/// Node mapping range.
template<class M, class /*SourceGraph*/, class /*TargetGraph*/>
concept node_mapping = true; /// @todo Implement this.

/// Node partitioning range.
template<class NodeParts, class /*Graph*/>
concept node_parts = true; /// @todo Implement this.

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

} // namespace tit::graph
