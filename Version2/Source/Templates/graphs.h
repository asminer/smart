
// $Id$

/*
    Directed graph classes.
    Essentially, explicit Markov chains.
*/


#ifndef GRAPHS_H
#define GRAPHS_H

#include "../defines.h"
#include "../Base/errors.h"
#include "../Base/streams.h"

const int MAX_NODE_ADD = 4096;
const int MAX_EDGE_ADD = 4096;

// ******************************************************************
// *                                                                *
// *                         digraph  class                         *
// *                                                                *
// ******************************************************************

/** Directed graph struct.

    By an immensely clever trick(s), both static and dynamic
    graphs use the same structure.  When the graph is "dynamic",
    nodes and edges can be added to the graph at will.
    We use sparse row-wise storage.

    In dynamic mode, arrays may be slightly larger than needed,
    and are enlarged if they become too small.
    Also, since we use a (circular) linked-list of entries for each row,
    the array "next" holds the next pointers (array indices).
    Entries are not necessarily in order.

    In static mode, arrays are resized to their minimum size,
    and the "next" array is not used.

    Conversion between dynamic and static can be done "in place"
    (except for addition of a next array if necessary)
    in O(num_edges) time.  ;^)

    Note: we always store "by rows".  However, this is not a problem;
    if you need a graph/matrix by columns, store its transpose. 
    There is a nice in-place (almost...) method to perform the transpose.
*/
struct digraph {
  /// Is the matrix transposed?
  bool isTransposed;

  /// Is it safe to add edges/vertices?
  bool isDynamic;

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

public:

  /// Constructor
  digraph();

  /// Destructor
  ~digraph();

  /// For allocating nodes and edges
  void ResizeNodes(int new_nodes);
  void ResizeEdges(int new_edges);

  inline bool IsStatic() const { return !isDynamic; }
  inline bool IsDynamic() const { return isDynamic; }

  inline int NumNodes() const { return num_nodes; }
  inline int NumEdges() const { return num_edges; }

  inline void AddNode() {
    DCASSERT(IsDynamic());
    if (num_nodes >= nodes_alloc) 
	ResizeNodes(MIN(2*nodes_alloc, nodes_alloc+MAX_NODE_ADD));
    row_pointer[num_nodes] = -1; // null
    num_nodes++;
  }

  /** Add an edge to unordered row list.
  */
  void AddEdge(int from, int to);

  /** If this is called every time, row lists will be ordered.
      Duplicate edges are not added twice.
      Returns the "address" (index) of the added / duplicate edge.
  */
  int AddEdgeInOrder(int from, int to);

  void ConvertToStatic();
  void ConvertToDynamic();

  void Transpose();

  /// Dump to a stream (human readable)
  void ShowNodeList(OutputStream &s, int node);

protected:
  /** Converts the circular linked lists to null-terminated ones.
      Used right before conversion to static.
  */
  inline void CircularToTerminated() {
    int s;
    for (s=0; s<num_nodes; s++) {
      if (row_pointer[s]<0) continue;
      int tail = row_pointer[s];    
      row_pointer[s] = next[tail];  
      next[tail] = -1;
    }
  }
};


// ******************************************************************
// *                                                                *
// *                     labeled_digraph  class                     *
// *                                                                *
// ******************************************************************

/** Labeled directed graph struct.

    Derived from digraph class to save some implementation.

*/
template <class LABEL>
struct labeled_digraph : public digraph {
  /// Labels on the edges
  LABEL* value;

public:
  /// Constructor
  labeled_digraph() : digraph() {
    value = NULL;
  }

  /// Destructor
  ~labeled_digraph() {
    isDynamic = true;
    ResizeNodes(0);
    ResizeEdges(0);
  }

  /// For allocating edges
  void ResizeEdges(int new_edges) {
    digraph::ResizeEdges(new_edges);
    LABEL* bar = (LABEL *) realloc(value, new_edges*sizeof(LABEL));
    if (new_edges && (NULL==bar)) OutOfMemoryError("Graph resize");
    value = bar;
  }

  /** Add an edge to unordered row list.
  */
  inline void AddEdge(int from, int to, const LABEL &v) {
    DCASSERT(IsDynamic());
    if (num_edges >= edges_alloc) 
	ResizeEdges(MIN(2*edges_alloc, edges_alloc+MAX_EDGE_ADD));
    DCASSERT(edges_alloc > num_edges);
    value[num_edges] = v;
    digraph::AddEdge(from, to);
  }

  /** If this is called every time, row lists will be ordered.
      If there is already an edge between the specified nodes,
      the given label will be added to the existing label (via +=).
      The function returns the "address" of the edge.
  */
  int AddEdgeInOrder(int from, int to, const LABEL &v) {
    DCASSERT(IsDynamic());
    if (num_edges >= edges_alloc) 
	ResizeEdges(MIN(2*edges_alloc, edges_alloc+MAX_EDGE_ADD));
    DCASSERT(edges_alloc > num_edges);
    value[num_edges] = v;
    int p = digraph::AddEdgeInOrder(from, to);
    if (p<num_edges-1) {
      // existing edge, sum values
     value[p] += v;
    }
    return p;
  }

  void ConvertToStatic();

  /// Dump to a stream (human readable)
  void ShowNodeList(OutputStream &s, int node);

};


// ******************************************************************
// *                                                                *
// *                     labeled_digraph methods                    *
// *                                                                *
// ******************************************************************


template <class LABEL>
void labeled_digraph <LABEL> :: ConvertToStatic()
/* Exactly the same as the unlabeled case, except we
   also must swap values.
*/
{
  if (IsStatic()) return;

  // First: convert circular lists to lists with null terminator
  CircularToTerminated();

#ifdef DEBUG_GRAPH
  Output << "Dynamic to static, dynamic arrays are:\n";
  Output << "row_pointer: [";
  Output.PutArray(row_pointer, num_nodes);
  Output << "]\n";
  Output.flush();
  Output << "column_index: [";
  Output.PutArray(column_index, num_edges);
  Output << "]\n";
  Output.flush();
  Output << "       value: [";
  Output.PutArray(value, num_edges);
  Output << "]\n";
  Output.flush();
  Output << "        next: [";
  Output.PutArray(next, num_edges);
  Output << "]\n";
  Output.flush();
#endif
  
  // make lists contiguous by swapping, forwarding pointers
  int i = 0; // everything before here is contiguous, after here is linked
  int s;
  for (s=0; s<num_nodes; s++) {
    int list = row_pointer[s];
    row_pointer[s] = i;
    while (list >= 0) {
      // traverse forwarding arcs if necessary...
      while (list<i) list = next[list];
      int nextlist = next[list];
      if (i!=list) {
        // swap i and list, set up forward
        next[list] = next[i];
        SWAP(column_index[i], column_index[list]);
	// This is...
	SWAP(value[i], value[list]);  
	// ... the only difference for labeled edges
        next[i] = list;  // forwarding info
      }
      list = nextlist;
      i++;
    } // while list
  } // for s
  row_pointer[num_nodes] = i;
  
  // resize arrays to be "tight"
  ResizeNodes(num_nodes);
  ResizeEdges(num_edges);

  // Trash next array
  free(next);
  next = NULL;
  isDynamic = false;

#ifdef DEBUG_GRAPH
  Output << "Static arrays are:\n";
  Output << "row_pointer: [";
  Output.PutArray(row_pointer, num_nodes+1);
  Output << "]\n";
  Output.flush();
  Output << "column_index: [";
  Output.PutArray(column_index, num_edges);
  Output << "]\n";
  Output.flush();
  Output << "       value: [";
  Output.PutArray(value, num_edges);
  Output << "]\n";
  Output.flush();
#endif
}

template <class LABEL>
void labeled_digraph <LABEL> :: ShowNodeList(OutputStream &s, int node)
{
  const char* fromstr = (isTransposed) ? "\tTo node " : "\tFrom node ";
  const char* tostr = (isTransposed) ? "\t\tFrom node " : "\t\tTo node ";
  const char* labelstr = "\tLabel ";
  int e;
  if (IsStatic()) {
    s << fromstr << node << "\n";
    for (e=row_pointer[node]; e<row_pointer[node+1]; e++) {
      s << tostr << column_index[e];
      s << labelstr << value[e] << "\n";
    }
  } else {
    s << fromstr << node << "\n";
    if (row_pointer[node]<0) return;
    int front = next[row_pointer[node]];
    e = front;
    do {
      s << tostr << column_index[e];
      s << labelstr << value[e] << "\n";
      e = next[e];
    } while (e!=front);
  }
}


#endif

