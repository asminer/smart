
// $Id$

/*
    Directed graph classes.
    Essentially, explicit Markov chains.
*/


#ifndef GRAPHS_H
#define GRAPHS_H

#include "../Base/errors.h"

/** A simple directed graph struct.
    Useful to unify the graph algorithms that don't care
    about edge values, like computing SCCs.
*/
struct directed_graph {
  /// Number of nodes in graph
  int num_nodes;

  /// Number of edges in graph
  int num_edges;

  /** Are the arcs reversed?
      Normally, we store arcs "from" a node, "to" a node;
      set this to true if we store them backwards.
  */
  bool reversedArcs;

  /** Array of size num_nodes + 1.
      If reversedArcs is false: [n] points to the first edge from node n.
      If reversedArcs is true:  [n] points to the first edge to node n.
  */
  int* node_pointer;   

  /** Array of size num_edges.
      If reversedArcs is false: [p] is the node pointed to by this edge.
      If reversedArcs is true:  [p] is the node where the edge originates from.
  */
  int* edge_index;

public:
  /// Empty constructor
  directed_graph() {
    num_nodes = num_edges = 0;
    node_pointer = edge_index = NULL;
  }
  ~directed_graph() {
    ResizeEdges(0);
    ResizeNodes(0);
  }
  void ResizeNodes(int new_num_nodes) {
    DCASSERT(new_num_nodes>=0);
    if (new_num_nodes != num_nodes) {
      int* foo = (int *) realloc(node_pointer, new_num_nodes*sizeof(int));
      if (new_num_nodes && (NULL==foo)) OutOfMemoryError("Graph resize");
      num_nodes = new_num_nodes;
      node_pointer = foo;
    }
  }
  void ResizeEdges(int new_num_edges) {
    DCASSERT(new_num_edges>=0);
    if (new_num_edges != num_edges) {
      int* foo = (int *) realloc(edge_index, new_num_edges*sizeof(int));
      if (new_num_edges && (NULL==foo)) OutOfMemoryError("Graph resize");
      num_edges = new_num_edges;
      edge_index = foo;
    }
  }
};

template <class LABEL>
struct labeled_graph : public directed_graph {
  LABEL* values;
public:
  labeled_graph() : directed_graph() {
    values = NULL;
  }
  ~labeled_graph() {
    ResizeEdges(0);
  }
  void ResizeEdges(int new_num_edges) {
    DCASSERT(new_num_edges>=0);
    if (new_num_edges != num_edges) {
      directed_graph::ResizeEdges(new_num_edges);
      LABEL* foo = (LABEL *) realloc(values, new_num_edges*sizeof(LABEL));
      if (new_num_edges && (NULL==foo)) OutOfMemoryError("Graph resize");
      values = foo;
    }
  }
};

#endif

