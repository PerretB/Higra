//
// Created by user on 4/2/18.
//

#include "py_graph_image.hpp"
#include "py_common_graph.hpp"
#include "higra/algo/graph_image.hpp"
#include "xtensor-python/pyarray.hpp"
#include "xtensor-python/pytensor.hpp"
#include "pybind11/functional.h"


namespace py = pybind11;


struct def_kalhimsky_2_contour {
    template<typename value_t>
    static
    void def(pybind11::module &m, const char *doc) {
        m.def("_khalimsky2contour", [](const xt::pyarray<value_t> &khalimsky,
                                       bool extra_border) {
                  return hg::khalimsky_2_contour2d(khalimsky, extra_border);
              },
              doc,
              py::arg("khalimsky"),
              py::arg("extra_border") = false);
    }
};

struct def_contour2Khalimsky {
    template<typename value_t>
    static
    void def(pybind11::module &m, const char *doc) {
        m.def("_contour2khalimsky", [](const hg::ugraph &graph,
                                       const std::vector<std::size_t> &shape,
                                       const xt::pyarray<value_t> &weights,
                                       bool add_extra_border) {
                  hg::embedding_grid_2d embedding(shape);
                  return hg::contour2d_2_khalimsky(graph, embedding, weights, add_extra_border);
              },
              doc,
              py::arg("graph"),
              py::arg("shape"),
              py::arg("edgeWeights"),
              py::arg("add_extra_border") = false);
    }
};

void py_init_graph_image(pybind11::module &m) {
    xt::import_numpy();

    m.def("_get_4_adjacency_graph", [](const std::vector<long> &v) {
              return hg::get_4_adjacency_graph(v);
          },
          "Create an explicit undirected 4 adjacency graph of the given shape.",
          py::arg("shape"));

    m.def("_get_8_adjacency_graph", [](const std::vector<long> &v) {
              return hg::get_8_adjacency_graph(v);
          },
          "Create an explicit undirected 8 adjacency graph of the given shape.",
          py::arg("shape"));

    m.def("_get_4_adjacency_implicit_graph", [](const std::vector<long> &v) {
              return hg::get_4_adjacency_implicit_graph(v);
          },
          "Create an explicit undirected 4 adjacency graph of the given shape (edges are not actually stored).",
          py::arg("shape"));

    m.def("_get_8_adjacency_implicit_graph", [](const std::vector<long> &v) {
              return hg::get_8_adjacency_implicit_graph(v);
          },
          "Create an explicit undirected 8 adjacency graph of the given shape (edges are not actually stored).",
          py::arg("shape"));


    add_type_overloads<def_contour2Khalimsky, HG_TEMPLATE_NUMERIC_TYPES>
            (m,
             "Create a contour image in the Khalimsky grid from a 4 adjacency edge-weighted graph."
            );


    add_type_overloads<def_kalhimsky_2_contour, HG_TEMPLATE_NUMERIC_TYPES>
            (m,
             "Create a 4 adjacency edge-weighted graph from a contour image in the Khalimsky grid. "
                     "Returns a tuple of three elements (graph, embedding, edge_weights)."
            );

}
