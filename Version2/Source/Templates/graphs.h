
// $Id$

/*
    Directed graph classes.
    Essentially, explicit Markov chains.
*/


#ifndef GRAPHS_H
#define GRAPHS_H

#include "../Base/errors.h"
#include "list.h"
#include "circlist.h"
#include "memmgr.h"

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
      int* foo = (int *) realloc(node_pointer, (1+new_num_nodes)*sizeof(int));
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

class dynamic_digraph {
  List <circ_node> *chains;
  int num_nodes;
  int num_edges;
public:
  dynamic_digraph(int initsize) {
    chains = new List <circ_node> (2);
    num_edges = num_nodes = 0;
  }
  ~dynamic_digraph() {
    delete chains;
  }
  inline int NumNodes() const { return num_nodes; }
  inline int NumEdges() const { return num_edges; }
  inline void AddVertex() {
    chains->Append((circ_node*)NULL);
    num_nodes++;
  }
  /// Returns an existing edge or the passed edge (if it is new).
  circ_node* AddUniqueEdge(int ndx, circ_node* edge);
};

template <class LABEL>
class dynamic_lg {
  struct node : public circ_node {
    LABEL value;
  };
  /// List of chains, each chain a circular-linked list
  List <node> *chains;
  int num_nodes;
  int num_edges;
  bool reversedArcs;
  /// pool of nodes
  Manager <node> *pool;
public:
  dynamic_lg(bool rev, int initsize) {
    reversedArcs = rev;
    chains = NULL;
    pool = NULL;
    num_edges = 0;
    num_nodes = 0;
  };
  ~dynamic_lg() {
    delete chains;
    delete pool;
  }
  inline void AddVertex() {
    if (NULL==chains) chains = new List <node>(2);
    if (NULL==pool) pool = new Manager <node> (1024);
    chains->Append((node*)NULL);
    num_nodes++;
  }
  bool AddArc(int from, int to, const LABEL &value);
  inline int NumEdges() const { return num_edges; }
  /// Copy to more static storage and clear the graph.
//  labeled_graph <LABEL> * Compress();
};

template <class LABEL>
/// Returns true if the arc was new
bool dynamic_lg<LABEL>::AddArc(int from, int to, const LABEL &value)
{
  DCASSERT(num_nodes>0);
  DCASSERT(from>=0);
  DCASSERT(from<chains->Length());
  DCASSERT(to>=0);
  DCASSERT(to<chains->Length());
  bool added = false;

  node *cur = pool->NewObject();
  cur->next = NULL;
  cur->value = value;
  node *tail;
  if (reversedArcs) {
    tail = chains->Item(to);
    cur->index = from;
  } else {
    tail = chains->Item(from);
    cur->index = to;
  }
  node *newtail = (node*) AddElement(tail, cur);
  if (NULL == newtail) {
    // collision?
    newtail = (node*) FindIndex(tail, cur->index); 
    DCASSERT(newtail);
    newtail->value += cur->value;
    newtail = tail;  // for later
    pool->FreeObject(cur);
  } else {
    num_edges++;
    added = true;
  }
  // update list of tails
  if (reversedArcs) {
      chains->SetItem(to, cur);
  } else {
      chains->SetItem(from, cur);
  }
  return added;
}

/*
template <class LABEL>
labeled_graph <LABEL> * dynamic_lg<LABEL>::Compress()
{
  labeled_graph <LABEL> *compact = new labeled_graph;
  compact->ResizeNodes(num_nodes);
 
  // Count number of edges for each node
  compact->node_pointer[i] = 0;
  int i;
  for (i=0; i<= num_nodes; i++) compact->node_pointer[i] = 0;

}
*/

#endif

