/***************************************************************************
* Copyright ESIEE Paris (2018)                                             *
*                                                                          *
* Contributor(s) : Benjamin Perret                                         *
*                                                                          *
* Distributed under the terms of the CECILL-B License.                     *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#include <boost/test/unit_test.hpp>
#include "higra/image/graph_image.hpp"
#include "higra/hierarchy/hierarchy_core.hpp"
#include "higra/algo/tree.hpp"
#include "test_utils.hpp"
#include "xtensor/xindex_view.hpp"
#include "xtensor/xrandom.hpp"

BOOST_AUTO_TEST_SUITE(hierarchyCore);

    using namespace hg;
    using namespace std;

    struct _data {

        hg::tree t;

        _data() : t(xt::xarray<long>{5, 5, 6, 6, 6, 7, 7, 7}) {
        }

    } data;


    BOOST_AUTO_TEST_CASE(testBPTtrivial) {

        auto graph = get_4_adjacency_graph({1, 2});

        xt::xarray<double> edge_weights{2};

        auto res = bpt_canonical(graph, edge_weights);
        auto &tree = res.tree;
        auto &altitudes = res.altitudes;
        auto &mst = res.mst;
        BOOST_CHECK(num_vertices(tree) == 3);
        BOOST_CHECK(num_edges(tree) == 2);
        BOOST_CHECK(xt::allclose(tree.parents(), xt::xarray<unsigned int>({2, 2, 2})));
        BOOST_CHECK(xt::allclose(altitudes, xt::xarray<double>({0, 0, 2})));
        BOOST_CHECK(num_vertices(mst) == 2);
        BOOST_CHECK(num_edges(mst) == 1);

    }


    BOOST_AUTO_TEST_CASE(testBPT) {

        auto graph = get_4_adjacency_graph({2, 3});

        xt::xarray<double> edge_weights{1, 0, 2, 1, 1, 1, 2};

        auto res = bpt_canonical(graph, edge_weights);
        auto &tree = res.tree;
        auto &altitudes = res.altitudes;
        auto &mst = res.mst;
        BOOST_CHECK(num_vertices(tree) == 11);
        BOOST_CHECK(num_edges(tree) == 10);
        BOOST_CHECK(xt::allclose(hg::parents(tree), xt::xarray<unsigned int>({6, 7, 9, 6, 8, 9, 7, 8, 10, 10, 10})));
        BOOST_CHECK(xt::allclose(altitudes, xt::xarray<double>({0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 2})));
        BOOST_CHECK(num_vertices(mst) == 6);
        BOOST_CHECK(num_edges(mst) == 5);
        std::vector<ugraph::edge_descriptor> ref = {{0, 3, 0},
                                                    {0, 1, 1},
                                                    {1, 4, 2},
                                                    {2, 5, 3},
                                                    {1, 2, 4}};
        for (index_t i = 0; i < (index_t) ref.size(); i++) {
            auto e = edge_from_index(i, mst);
            BOOST_CHECK(e == ref[i]);
        }
    }


    BOOST_AUTO_TEST_CASE(testTreeSimplification) {

        auto t = data.t;

        hg::array_1d<double> altitudes{0, 0, 0, 0, 0, 1, 2, 2};

        auto criterion = xt::equal(altitudes, xt::index_view(altitudes, t.parents()));

        auto res = hg::simplify_tree(t, criterion);
        auto nt = res.tree;
        auto nm = res.node_map;

        BOOST_CHECK(num_vertices(nt) == 7);

        hg::array_1d<index_t> refp{5, 5, 6, 6, 6, 6, 6};
        BOOST_CHECK(refp == hg::parents(nt));

        hg::array_1d<index_t> refnm{0, 1, 2, 3, 4, 5, 7};
        BOOST_CHECK(refnm == nm);
    }

    BOOST_AUTO_TEST_CASE(testQuasiFlatZoneHierarchy) {

        auto graph = get_4_adjacency_graph({2, 3});

        xt::xarray<double> edge_weights{1, 0, 2, 1, 1, 1, 2};

        auto res = quasi_flat_zones_hierarchy(graph, edge_weights);
        auto rtree = res.tree;
        auto altitudes = res.altitudes;
        tree tref(array_1d<index_t>{6, 7, 8, 6, 7, 8, 7, 9, 9, 9});
        BOOST_CHECK(testTreeIsomorphism(rtree, tref));
        BOOST_CHECK(xt::allclose(altitudes, xt::xarray<double>({0, 0, 0, 0, 0, 0, 0, 1, 1, 2})));
    }

    BOOST_AUTO_TEST_CASE(testSaliencyMap) {

        auto graph = get_4_adjacency_graph({2, 4});

        tree t(xt::xarray<long>{8, 8, 9, 9, 10, 10, 11, 11, 12, 13, 12, 14, 13, 14, 14});
        xt::xarray<double> altitudes{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3};

        auto sm = saliency_map(graph, t, altitudes);
        xt::xarray<double> sm_ref = {0, 1, 2, 1, 0, 3, 3, 0, 3, 0};

        BOOST_CHECK(sm == sm_ref);
    }

    BOOST_AUTO_TEST_CASE(testSaliencyMap_bpt_qzf_equiv) {

        index_t size = 25;
        auto graph = get_4_adjacency_graph({size, size});
        auto edge_weights = xt::eval(xt::random::randint<int>({num_edges(graph)}, 0, 25));

        auto bpt = bpt_canonical(graph, edge_weights);
        auto qfz = quasi_flat_zones_hierarchy(graph, edge_weights);

        auto sm_bpt = saliency_map(graph, bpt.tree, bpt.altitudes);
        auto sm_qfz = saliency_map(graph, qfz.tree, qfz.altitudes);

        BOOST_CHECK(sm_bpt == sm_qfz);
    }

BOOST_AUTO_TEST_SUITE_END();