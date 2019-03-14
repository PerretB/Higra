/***************************************************************************
* Copyright ESIEE Paris (2018)                                             *
*                                                                          *
* Contributor(s) : Benjamin Perret                                         *
*                                                                          *
* Distributed under the terms of the CECILL-B License.                     *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#pragma once

#include "common.hpp"
#include "../graph.hpp"
#include "../structure/fibonacci_heap.hpp"

namespace hg {

    namespace binary_partition_tree_internal {

        template<typename T>
        struct heap_element {
            using self_t = heap_element<T>;
            T value;
            index_t index;

            bool operator==(const self_t &rhs) const { return value == rhs.value; }

            bool operator!=(const self_t &rhs) const { return value != rhs.value; }

            bool operator<(const self_t &rhs) const { return value < rhs.value; }

            bool operator>(const self_t &rhs) const { return value > rhs.value; }

            bool operator<=(const self_t &rhs) const { return value <= rhs.value; }

            bool operator>=(const self_t &rhs) const { return value >= rhs.value; }

        };

        /**
         * This structure is provided by the binary partition algorithm when two nodes are merged in order to
         * compute the edge weight between the newly created node and one of its neighbouring node.
         * @tparam T Value type of edge weights
         */
        template<typename T>
        struct new_neighbour {
            using value_type = T;
        private:
            index_t m_neighbour_vertex;
            index_t m_edge1_index;
            index_t m_edge2_index;
            mutable T m_new_edge_weight;

        public:

            /**
             *
             * @param neighbour_vertex The index of the existing neighbour of the newly created node
             * @param edge1_index The index of the edge linking the first merged node with the neighbouring node
             * @param edge2_index The index of the edge linking the second merged node with the neighbouring node:
             * might be set to invalid_index (default value) if no such edge exists.
             */
            new_neighbour(index_t neighbour_vertex, index_t edge1_index, index_t edge2_index = invalid_index) :
                    m_neighbour_vertex(neighbour_vertex), m_edge1_index(edge1_index), m_edge2_index(edge2_index) {

            }

            /**
             * Number of edges between the merged nodes and the neighbour node.
             * @return
             */
            auto num_edges() const {
                return (m_edge2_index == invalid_index) ? 1 : 2;
            }

            /**
             * The index of the edge linking the first merged node with the neighbouring node.
             * @return
             */
            const auto &first_edge_index() const {
                return m_edge1_index;
            }

            /**
             * The index of the edge linking the first merged node with the neighbouring node.
             * @return
             */
            auto &first_edge_index() {
                return m_edge1_index;
            }

            /**
             * The index of the edge linking the second merged node with the neighbouring node (invalid_index if num_edges() < 2).
             * @return
             */
            const auto &second_edge_index() const {
                return m_edge2_index;
            }

            /**
            * The index of the edge linking the second merged node with the neighbouring node (invalid_index if num_edges() < 2).
            * @return
            */
            auto &second_edge_index() {
                return m_edge2_index;
            }

            /**
             * The index of the neighbour node.
             * @return
             */
            const auto &neighbour_vertex() const {
                return m_neighbour_vertex;
            }

            /**
             * The index of the neighbour node.
             * @return
             */
            auto &neighbour_vertex() {
                return m_neighbour_vertex;
            }

            /**
             * The new value of the edge linking the new node to the new neighbour.
             * @return
             */
            auto &new_edge_weight() const {
                return m_new_edge_weight;
            }

            /**
             * The index of the edge linking the new node to the neighbour node.
             * @return
             */
            auto new_edge_index() const {
                return m_edge1_index;
            }

        };

    }

    /**
     * Compute the binary partition tree of the graph.
     *
     * At each step:
     * 1 - the algorithm finds the edge of smallest weight.
     * 2 - the two vertices linked by this edge are merged: the new vertex is the parent of the two merged vertices
     * 3 - the weight of the edges linking the new vertex to the remaining vertices of the graph are updated according
     *      to the user provided function (weight_function)
     * 4 - repeat until a single edge remain
     *
     * The initial weight of the edges (xedge_weights) and the callback (weight_function) determine the shape of the
     * hierarchy.
     *
     * The weight_function callback can be anything that defining the operator() and should follow the following pattern:
     *
     * struct my_weighter {
     *  ...
     *
     *  template<typename graph_t, typename neighbours_t>
     *  void operator()(const graph_t &g,               // the current state of the graph
     *                  index_t fusion_edge_index,      // the edge between the two vertices being merged
     *                  index_t new_region,             // the new vertex in the graph
     *                  index_t merged_region1,         // the first vertex merged
     *                  index_t merged_region2,         // the second vertex merged
     *                  neighbours_t &new_neighbours){  // list of edges to be weighted (see below)
     *      ...
     *      for (auto &n: new_neighbours) {
     *          ...
     *          n.new_edge_weight() = new_edge_value; // define the weight of this edge
     *      }
     *  }
     *
     * Each element in the parameter new_neighbours represent an edge between the new vertex and another vertex of
     * the graph. For each element of the list, the following methods are available:
     *  - neighbour_vertex(): the other vertex
     *  - num_edges(): returns 2 if both the two merged vertices add an edge linking themselves with neighbour_vertex()
     *      and 1 otherwise
     *  - first_edge_index(): the index of the edge linking one of the merged region to neighbour_vertex()
     *  - second_edge_index(): the index of the edge linking the other merged region to neighbour_vertex() (only if num_edges()==2)
     *  - new_edge_weight(): weight of the new edge (THIS HAS TO BE DEFINED IN THE WEIGHTING FUNCTION)
     *  - new_edge_index(): the index of the new edge: the weighting function will probably have to track new weight values
     *
     * Example of weighting function: binary_partition_tree_min_linkage
     *
     * @tparam graph_t
     * @tparam weighter
     * @tparam T
     * @param graph
     * @param xedge_weights
     * @param weight_function
     * @return
     */
    template<typename graph_t, typename weighter, typename T>
    auto
    binary_partition_tree(const graph_t &graph, const xt::xexpression<T> &xedge_weights, weighter weight_function) {
        using weight_t = typename T::value_type;
        using heap_t = fibonacci_heap<binary_partition_tree_internal::heap_element<weight_t> >;
        
        auto &edge_weights = xedge_weights.derived_cast();
        hg_assert_edge_weights(graph, edge_weights);

        auto g = copy_graph<undirected_graph<hash_setS> >(graph); // optimized for removal

        auto num_points = num_vertices(g);
        auto num_nodes_tree = num_points * 2 - 1;

        array_1d<index_t> parents = xt::arange(num_nodes_tree);
        array_1d<weight_t> levels = xt::zeros<weight_t>({num_nodes_tree});

        // optimization to detect already visited neighbours during neighbour search
        array_1d<index_t> new_neighbour_indices({num_nodes_tree}, invalid_index);

        // active edges are in the heap and still present in the graph (removed edges are leazily left in the heap)
        // TODO: check for performance impact
        array_1d<bool> active = xt::zeros<bool>({num_edges(g)});

        // special structure to store the list of neighbours adjacent to the fused regions.
        std::vector<binary_partition_tree_internal::new_neighbour<weight_t> > new_neighbours;
        const decltype(new_neighbours) &const_new_neighbours = new_neighbours;

        // init heap
        heap_t heap;
        array_1d<typename heap_t::value_handle> heap_handles({num_edges(g)}, nullptr);

        for (auto v: vertex_iterator(graph)) {
            for (auto &e: out_edge_iterator(v, g)) {
                if (!active(e)) {
                    heap_handles(e) = heap.push({edge_weights(e), e});
                    active(e) = true;
                }
            }
        }

        // main loop
        size_t current_num_nodes_tree = num_points;
        while (!heap.empty() && current_num_nodes_tree < num_nodes_tree) {

            auto heap_handle = heap.top();
            auto min_element = heap_handle->get_value();

            auto fusion_edge_index = min_element.index;
            auto fusion_edge_weight = min_element.value;

            heap.pop();
            heap_handles[fusion_edge_index] = nullptr;

            if (active[fusion_edge_index]) {
                active[fusion_edge_index] = false;
                // create new region, update tree
                auto new_parent = g.add_vertex();
                auto fusion_edge = edge_from_index(fusion_edge_index, g);
                auto region1 = source(fusion_edge, g);
                auto region2 = target(fusion_edge, g);
                parents[region1] = new_parent;
                parents[region2] = new_parent;
                levels[new_parent] = fusion_edge_weight;
                current_num_nodes_tree++;

                // remove fusion edge
                remove_edge(fusion_edge_index, g);
                active[fusion_edge_index] = false;

                // search for neighbours of region1 and region2 and store them in new_neighbours
                new_neighbours.clear();
                auto explore_region = [&g, &new_neighbours, &new_neighbour_indices](
                        index_t region) {
                    for (auto e: out_edge_iterator(region, g)) {
                        auto n = other_vertex(e, region, g);
                        if (new_neighbour_indices[n] != invalid_index) {
                            new_neighbours[new_neighbour_indices[n]].second_edge_index() = e;
                        } else {
                            new_neighbour_indices[n] = new_neighbours.size();
                            new_neighbours.emplace_back(n, e);
                        }
                    }
                };

                explore_region(region1);
                explore_region(region2);
                for (auto &n: new_neighbours) {
                    new_neighbour_indices[n.neighbour_vertex()] = invalid_index;
                }

                // update edge weights
                if (!new_neighbours.empty()) { // should only happen at last iteration
                    // external callback : compute new edge weights
                    weight_function(g, fusion_edge_index, new_parent, region1, region2, const_new_neighbours);

                    // process new weights, update heap and things
                    for (auto &nn: new_neighbours) {
                        if (nn.num_edges() > 1) {
                            active[nn.second_edge_index()] = false;
                            // not removed from heap: maybe not necessary
                            remove_edge(nn.second_edge_index(), g);
                        }
                        set_edge(nn.first_edge_index(), nn.neighbour_vertex(), new_parent, g);
                        heap.update(heap_handles[nn.first_edge_index()], {nn.new_edge_weight(), nn.first_edge_index()});
                        active[nn.first_edge_index()] = true;
                    }
                }
            }
        }
        return make_node_weighted_tree(tree(parents), std::move(levels));
    }


    /**
     * Weighting function to be used in conjunction to the binary_partition_tree method in order to perform a single linkage clustering.
     *
     * Given a graph G, with initial edge weights W,
     * the distance d(X,Y) between any two regions X, Y is defined as :
     *      d(X,Y) = min {W({x,y}) | x in X, y in Y, {x,y} in G }
     *
     * Warning: this is a demonstration: in practice, the bptCanonical function (hierarchy_core.hpp)
     * can compute the single linkage clustering more efficiently.
     *
     * Consider using the helper factory function make_binary_partition_tree_min_linkage
     *
     * @tparam T
     */
    template<typename T>
    struct binary_partition_tree_min_linkage {

        T &m_weights;

        /**
         * Initialize the clustering with given edge weights
         * @param weights
         */
        binary_partition_tree_min_linkage(T &weights) : m_weights(weights) {
            HG_LOG_INFO("Please consider using bpt_canonical to compute the minimum linkage binary partition tree for improved performances.");
        }

        template<typename graph_t, typename neighbours_t>
        void operator()(const graph_t &g,
                        index_t fusion_edge_index,
                        index_t new_region,
                        index_t merged_region1,
                        index_t merged_region2,
                        neighbours_t &new_neighbours) {

            for (auto &n: new_neighbours) {
                auto min_value = m_weights[n.first_edge_index()];
                if (n.num_edges() > 1) {
                    if (m_weights[n.second_edge_index()] < min_value)
                        min_value = m_weights[n.second_edge_index()];
                }
                n.new_edge_weight() = min_value;
                m_weights[n.new_edge_index()] = min_value;
            }
        }
    };


    /**
    * Weighting function to be used in conjunction to the binary_partition_tree method in order to perform a complete linkage clustering.
    *
    * Given a graph G, with initial edge weights W,
    * the distance d(X,Y) between any two regions X, Y is defined as :
    *      d(X,Y) = max {W({x,y}) | x in X, y in Y, {x,y} in G }
    *
    * Consider using the helper factory function make_binary_partition_tree_complete_linkage
    *
    * @tparam T
    */
    template<typename T>
    struct binary_partition_tree_complete_linkage {

        T &m_weights;

        /**
         * Initialize the clustering with given edge weights
         * @param weights
         */
        binary_partition_tree_complete_linkage(T &weights) : m_weights(weights) {
        }

        template<typename graph_t, typename neighbours_t>
        void operator()(const graph_t &g,
                        index_t fusion_edge_index,
                        index_t new_region,
                        index_t merged_region1,
                        index_t merged_region2,
                        neighbours_t &new_neighbours) {

            for (auto &n: new_neighbours) {
                auto max_value = m_weights[n.first_edge_index()];
                if (n.num_edges() > 1) {
                    if (max_value < m_weights[n.second_edge_index()])
                        max_value = m_weights[n.second_edge_index()];
                }
                n.new_edge_weight() = max_value;
                m_weights[n.new_edge_index()] = max_value;
            }
        }
    };

    /**
    * Weighting function to be used in conjunction to the binary_partition_tree method in order to perform an average linkage clustering.
    *
    * Given a graph G, with initial edge values V with associated weights W,
    * the distance d(X,Y) between any two regions X, Y is defined as :
    *      d(X,Y) = (1 / Z) + sum_{x in X, y in Y, {x,y} in G} V({x,y}) x W({x,y})
    * with Z = sum_{x in X, y in Y, {x,y} in G} W({x,y})
     *
    * Consider using the helper factory function make_binary_partition_tree_average_linkage
    *
    * @tparam T
    */
    template<typename T>
    struct binary_partition_tree_average_linkage {
        using value_type = typename std::decay<T>::type::value_type;

        T m_values;
        T m_weights;

        binary_partition_tree_average_linkage(const T &values, const T &weights) : m_values(values), m_weights(weights) {
            hg_assert_same_shape(values, weights);
        }

        template<typename graph_t, typename neighbours_t>
        void operator()(const graph_t &g,
                        index_t fusion_edge_index,
                        index_t new_region,
                        index_t merged_region1,
                        index_t merged_region2,
                        neighbours_t &new_neighbours) {

            for (auto &n: new_neighbours) {
                value_type new_value;
                value_type new_weight;
                if (n.num_edges() > 1) {
                    new_weight = m_weights[n.first_edge_index()] + m_weights[n.second_edge_index()];
                    new_value = (m_values[n.first_edge_index()] * m_weights[n.first_edge_index()]
                            + m_values[n.second_edge_index()] * m_weights[n.second_edge_index()])
                            / new_weight;
                } else{
                    new_weight = m_weights[n.first_edge_index()];
                    new_value = m_values[n.first_edge_index()];
                }
                n.new_edge_weight() = new_value;
                m_values[n.new_edge_index()] = new_value;
                m_weights[n.new_edge_index()] = new_weight;
            }
        }
    };

    /**
     * Factory function for binary_partition_tree_min_linkage
     * @tparam Q
     * @param weights
     * @return
     */
    template<typename Q>
    auto make_binary_partition_tree_min_linkage(Q &&weights) {
        return binary_partition_tree_min_linkage<Q>(std::forward<Q>(weights));
    }

    /**
     * Factory function for binary_partition_tree_complete_linkage
     * @tparam Q
     * @param weights
     * @return
     */
    template<typename Q>
    auto make_binary_partition_tree_complete_linkage(Q &&weights) {
        return binary_partition_tree_complete_linkage<Q>(std::forward<Q>(weights));
    }

    /**
     * Factory function for binary_partition_tree_average_linkage
     * @tparam Q
     * @param weights
     * @return
     */
    template<typename Q>
    auto make_binary_partition_tree_average_linkage(Q &&values, Q &&weights) {
        return binary_partition_tree_average_linkage<Q>(std::forward<Q>(values), std::forward<Q>(weights));
    }

}
