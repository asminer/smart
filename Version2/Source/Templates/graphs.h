
// $Id$

/*
    Directed graph classes.
    Essentially, explicit Markov chains.
*/


#ifndef GRAPHS_H
#define GRAPHS_H

#include "../Base/errors.h"
#include "../Base/streams.h"
#include "list.h"
#include "circlist.h"
#include "memmgr.h"

// ******************************************************************
// *                                                                *
// *                      directed_graph  class                     *
// *                                                                *
// ******************************************************************

/** A simple directed graph struct (static).
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

// ******************************************************************
// *                                                                *
// *                       labeled_graph class                      *
// *                                                                *
// ******************************************************************

/** A labeled directed graph struct (static).
    In other words, a sparsely stored matrix.
*/
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
  /// Dump to a stream (human readable)
  void Show(OutputStream &s);
};

template <class LABEL>
void labeled_graph <LABEL>::Show(OutputStream &s)
{
  const char* storage = (reversedArcs) ? "incoming" : "outgoing";
  const char* rowlabel = (reversedArcs) ? "Into" : "Out of";
  const char* collabel = (reversedArcs) ? "Out of" : "Into";
  s << "Labeled graph, stored via " << storage << " edges\n";
  int i;
  int j=node_pointer[0];
  for (i=0; i<num_nodes; i++) {
    s << "\t" << rowlabel << " vertex " << i << "\n";
    for (; j<node_pointer[i+1]; j++) {
      s << "\t\t" << collabel << " vertex " << edge_index[j];
      s << "\t label " << values[j] << "\n";
    } 
  }
  s << "End of graph\n";
}


// ******************************************************************
// *                                                                *
// *                      dynamic_digraph class                     *
// *                                                                *
// ******************************************************************

/** Directed graph class, where both vertices and edges can be added.
    
*/
class dynamic_digraph {
  int num_nodes;
  int num_edges;
protected:
  bool reversedArcs;
  List <circ_node> *chains;
public:
  dynamic_digraph(bool rev) {
    reversedArcs = rev;
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
  inline circ_node* AddEdge(int from, int to, circ_node* edge) {
    DCASSERT(edge);
    if (reversedArcs) {
      edge->index = from;
      return AddUniqueEdge(to, edge);
    } else {
      edge->index = to;
      return AddUniqueEdge(from, edge);
    }
  }
protected:
  /// Returns an existing edge or the passed edge (if it is new).
  circ_node* AddUniqueEdge(int ndx, circ_node* edge) {
    DCASSERT(edge);
    DCASSERT(ndx>=0);
    DCASSERT(ndx<num_nodes);
    circ_node* thing = AddElement(chains->Item(ndx), edge);
    DCASSERT(thing);
    if ((thing->index == edge->index) && (thing != edge)) {
      // existing edge
      return thing;
    }
    chains->SetItem(ndx, thing);
    num_edges++;
    return edge;
  }
};


// ******************************************************************
// *                                                                *
// *                      dynamic_labeled class                     *
// *                                                                *
// ******************************************************************

/** Directed graph class with labels.
*/
template <class LABEL>
class dynamic_labeled : public dynamic_digraph {
  typedef circ_node_data <LABEL> node;
  /// pool of nodes
  Manager <node> *pool;
public:
  dynamic_labeled(Manager <node> *p, bool rev) : dynamic_digraph(rev) {
    pool = p;
    DCASSERT(pool->ObjectSize() >= sizeof(node));
  };
  ~dynamic_labeled() {
    // Don't delete pool, it might be shared
  }
  /// Returns true if the arc was new; otherwise labels are summed
  bool AddLabeledEdge(int from, int to, const LABEL &value) {
    node *cur = pool->NewObject();
    cur->value = value;
    node *bar = (node*) AddEdge(from, to, cur);
    if (bar!=cur) {
      // existing edge, add labels
      bar->value += cur->value;
      pool->FreeObject(cur);
      return false;
    } 
    return true;
  };
  /// Dump to a stream (human readable)
  void Show(OutputStream &s);
  /// Copy to more static storage and clear the graph.
  labeled_graph <LABEL> * CompressAndDestroy();
};

template <class LABEL>
void dynamic_labeled <LABEL>::Show(OutputStream &s)
{
  const char* storage = (reversedArcs) ? "incoming" : "outgoing";
  const char* rowlabel = (reversedArcs) ? "Into" : "Out of";
  const char* collabel = (reversedArcs) ? "Out of" : "Into";
  s << "Dynamic labeled graph, stored via " << storage << " edges\n";
  int i;
  for (i=0; i<NumNodes(); i++) {
    s << "\t" << rowlabel << " vertex " << i << "\n";
    node* ptr = (node*) chains->Item(i);
    if (NULL==ptr) continue;
    do {
      ptr = (node*) ptr->next;
      s << "\t\t" << collabel << " vertex " << ptr->index;
      s << "\t label " << ptr->value << "\n";
    } while (chains->Item(i) != ptr);
  }
  s << "End of graph\n";
}

template <class LABEL>
labeled_graph <LABEL> * dynamic_labeled<LABEL>::CompressAndDestroy()
{
  labeled_graph <LABEL> *compact = new labeled_graph <LABEL>;
  compact->ResizeNodes(NumNodes());
  compact->ResizeEdges(NumEdges());
  compact->reversedArcs = reversedArcs;

  int edgeptr = 0;
  int i;
  for (i=0; i<NumNodes(); i++) {
    compact->node_pointer[i] = edgeptr;
    // traverse list for vertex i
    node* ptr = (node*) chains->Item(i);
    if (NULL==ptr) continue;
    node* next = (node*) ptr->next;
    do {
      ptr = next; 
      compact->edge_index[edgeptr] = ptr->index;
      compact->values[edgeptr] = ptr->value;
      edgeptr++;
      next = (node*) ptr->next;
      pool->FreeObject(ptr);
    } while (chains->Item(i) != ptr);
  }
  compact->node_pointer[i] = edgeptr;
  chains->Clear();
  return compact;
}

#endif

