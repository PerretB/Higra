import higra as hg


@hg.data_consumer("edge_weights")
def bpt_canonical(graph, edge_weights):
    """
    Compute the canonical binary partition tree (binary tree by altitude ordering) of the given weighted graph.


    :param graph:
    :param edge_weights:
    :return: Tree (with attributes "leaf_graph", "altitudes" and "mst")
    """

    tree, altitudes, mst = hg._bpt_canonical(graph, edge_weights)

    original_graph = hg.get_attribute(graph, "original_graph")
    if original_graph:
        hg.set_attribute(tree, "leaf_graph", original_graph)
        hg.set_attribute(mst, "original_graph", original_graph)
    else:
        hg.set_attribute(tree, "leaf_graph", graph)
        hg.set_attribute(mst, "original_graph", graph)

    hg.set_attribute(tree, "altitudes", altitudes)
    hg.set_attribute(tree, "mst", mst)

    return tree


@hg.data_consumer("deleted_vertices")
def simplify_tree(tree, deleted_vertices):
    """
    Creates a copy of the current Tree and deletes the vertices i such that deletedVertices[i] is true.

    The attribute "node_map" of the returned tree is an array that maps any node index i of the new tree,
    to the index of this node in the original tree.

    :param tree:
    :param deleted_vertices:
    :return: simplified tree (with attributes "node_map")
    """

    new_tree, node_map = hg._simplify_tree(tree, deleted_vertices)

    hg.set_attribute(new_tree, "leaf_graph", hg.getAttribute(tree, "leaf_graph"))
    hg.set_attribute(new_tree, "node_map", node_map)

    return new_tree


@hg.data_consumer("altitudes", "lca_map")
def saliency(tree, altitudes, lca_map):
    """
    Compute the saliency map of the given tree
    :param tree:
    :param altitudes: altitudes of the vertices of the tree
    :param lca_map: array containing the lowest common ancestor of the source and target vertices of each edge
    where saliency need to be computed
    :return: altitudes[lca_map]
    """

    return altitudes[lca_map]
