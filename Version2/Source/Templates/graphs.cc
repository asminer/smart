
// $Id$

/*
    Directed graph classes, implementation.
*/


#include "../Base/errors.h"

// #include "graphs.h"


/** Directed graph struct.

    By an immensely clever trick(s), both static and dynamic
    graphs use the same structure.  When the graph is "dynamic",
    nodes and edges can be added to the graph at will.
    We use sparse row-wise storage.
    TBD: allow column-wise and add a Transpose function.

    In dynamic mode, arrays may be slightly larger than needed,
    and are enlarged if they become too small.
    Also, since we use a (circular) linked-list of entries for each row,
    the array "next" holds the next pointers (array indices).

    In static mode, arrays are resized to their minimum size,
    and the "next" array is not used.

    Conversion between dynamic and static can be done "in place"
    (except for addition of a next array if necessary)
    in O(num_edges) time.  ;^)

    TBD: "Transpose" method
    TBD: "isTransposed" member
*/
struct digraph {
  /// Number of nodes in graph
  int num_nodes;

  /// Number of edges in graph
  int num_edges;

  /** Array of size num_nodes + 1 (or larger).
      The extra pointer is unused in "dynamic" mode.
  */
  int* row_pointer;   

  /// Size of row_pointer array.
  int nodes_alloc;

  /** Array of size num_edges (or larger).
  */
  int* column_index;

  /** The index of the next entry in the list (dynamic mode).
      A negative value is used for "null".
  */
  int* next;

  /// Size of the next and column_index arrays (and possibly others)
  int edges_alloc;

  inline bool IsStatic() const { return NULL==next; }
  inline bool IsDynamic() const { return NULL!=next; }

  inline void AddNode() {
    DCASSERT(IsDyanamic());
    if (num_nodes >= nodes_alloc) ResizeNodes(2*nodes_alloc);
    row_pointer[num_nodes] = -1; // null
    num_nodes++;
  }

  void AddEdge(int from, int to);

  void ConvertToStatic();
  void ConvertToDynamic();

  /// Dump to a stream (human readable)
  void Show(OutputStream &s, int node);

protected:
  void ResizeNodes(int new_nodes);
  void ResizeArcs(int new_edges);
};


// ******************************************************************
// *                                                                *
// *                         digraph methods                        *
// *                                                                *
// ******************************************************************

void digraph::AddEdge(int from, int to)
{
  DCASSERT(IsDynamic());
  if (num_edges >= edges_alloc) ResizeEdges(2*edges_alloc);
  column_index[num_edges] = to;
  if (row_pointer[from] < 0) {
    // row was empty before
    next[num_edges] = num_edges;  // circle of this node itself
    row_pointer[from] = num_edges;
    num_edges++;
    return;
  } 
  // nonempty row, find spot for this edge in list
}

void digraph::ResizeNodes(int new_nodes) 
{
  DCASSERT(IsDynamic());
  if (nodes_alloc != new_nodes) {
      int* foo = (int *) realloc(row_pointer, (1+new_nodes)*sizeof(int));
      if (new_nodes && (NULL==foo)) OutOfMemoryError("Graph resize");
      row_pointer = foo;
      nodes_alloc = new_nodes;
  }
}

void digraph::ResizeArcs(int new_edges) 
{
  DCASSERT(IsDynamic());
  if (edges_alloc != new_edges) {
      int* foo = (int *) realloc(column_index, new_edges*sizeof(int));
      if (new_nodes && (NULL==foo)) OutOfMemoryError("Graph resize");
      column_index = foo;
      foo = (int *) realloc(next, new_edges*sizeof(int));
      if (new_nodes && (NULL==foo)) OutOfMemoryError("Graph resize");
      next = foo;
      edges_alloc = new_edges;
  }
}

void digraph::Show(OutputStream &s, int node)
{
  int e;
  if (IsStatic()) {
    s << "\tFrom node " << node << "\n";
    for (e=row_pointer[node]; e<row_pointer[node+1]; e++) {
      s << "\t\tTo node " << column_index[e] << "\n";
    }
  }
}

