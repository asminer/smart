
// $Id$

/*
    Directed graph classes, implementation.
*/


#include "../Base/streams.h"
#include "../Base/errors.h"

// #include "graphs.h"

// #define DEBUG_GRAPH

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

  inline bool IsStatic() const { return NULL==next; }
  inline bool IsDynamic() const { return NULL!=next; }

  inline int NumNodes() const { return num_nodes; }
  inline int NumEdges() const { return num_edges; }

  inline void AddNode() {
    DCASSERT(IsDyanamic());
    if (num_nodes >= nodes_alloc) ResizeNodes(2*nodes_alloc);
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

};


// ******************************************************************
// *                                                                *
// *                         digraph methods                        *
// *                                                                *
// ******************************************************************

digraph::digraph()
{
  num_nodes = 0;
  num_edges = 0;
  row_pointer = NULL;
  nodes_alloc = 0;
  column_index = NULL;
  next = NULL;
  edges_alloc = 0;
  isTransposed = false;
}

digraph::~digraph()
{
  ResizeNodes(0);
  ResizeEdges(0);
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

void digraph::ResizeEdges(int new_edges) 
{
  DCASSERT(IsDynamic());
  if (edges_alloc != new_edges) {
      int* foo = (int *) realloc(column_index, new_edges*sizeof(int));
      if (new_edges && (NULL==foo)) OutOfMemoryError("Graph resize");
      column_index = foo;
      foo = (int *) realloc(next, new_edges*sizeof(int));
      if (new_edges && (NULL==foo)) OutOfMemoryError("Graph resize");
      next = foo;
      edges_alloc = new_edges;
  }
}

void digraph::AddEdge(int from, int to)
{
  // Sanity checks
  DCASSERT(IsDynamic());
  DCASSERT(from>=0);
  DCASSERT(from<num_nodes);
  DCASSERT(to>=0);
  DCASSERT(to<num_nodes);

  if (num_edges >= edges_alloc) ResizeEdges(2*edges_alloc);
  DCASSERT(edges_alloc > num_edges);
  column_index[num_edges] = to;
  if (row_pointer[from] < 0) {
    // row was empty before
    next[num_edges] = num_edges;  // circle of this node itself
    row_pointer[from] = num_edges;
  } else {
    // nonempty row, add edge to tail of circular list
    int tail = row_pointer[from];
    next[num_edges] = next[tail];
    next[tail] = num_edges;
    row_pointer[from] = num_edges;
  }
  num_edges++;
}

int digraph::AddEdgeInOrder(int from, int to)
{
  DCASSERT(IsDynamic());
  if (num_edges >= edges_alloc) ResizeEdges(2*edges_alloc);
  DCASSERT(edges_alloc > num_edges);
  column_index[num_edges] = to;
  if (row_pointer[from] < 0) {
    // row was empty before
    next[num_edges] = num_edges;  // circle of this node itself
    row_pointer[from] = num_edges;
    return num_edges++;
  } 
  // Row is not empty.
  int prev = row_pointer[from];
  if (to > column_index[prev]) {
    // Fast and easy special case: new edge at end of list
    next[num_edges] = next[prev];
    next[prev] = num_edges;
    row_pointer[from] = num_edges;
    return num_edges++;
  }
  // Find spot for this edge
  while (1) {
    int curr = next[prev];
    if (to < column_index[curr]) {
      // edge goes here!
      next[num_edges] = curr;
      next[prev] = num_edges;
      return num_edges++;
    }
    if (to == column_index[curr]) {
      // duplicate edge, don't add it
      return curr;
    }
    prev = curr;
  }
  // never get here.
}

void digraph::ConvertToStatic()
{
  if (IsStatic()) return;

  // First: convert circular lists to lists with null terminator
  int s;
  for (s=0; s<num_nodes; s++) {
    if (row_pointer[s]<0) continue;
    int tail = row_pointer[s];    
    row_pointer[s] = next[tail];  
    next[tail] = -1;
  }

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
  Output << "        next: [";
  Output.PutArray(next, num_edges);
  Output << "]\n";
  Output.flush();
#endif
  
  // make lists contiguous by swapping, forwarding pointers
  int i = 0; // everything before here is contiguous, after here is linked
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
#endif
}

void digraph::ConvertToDynamic()
{
  if (IsDynamic()) return;

  // Add next array
  next = (int *) realloc(next, edges_alloc*sizeof(int));
  if (edges_alloc && (NULL==next)) OutOfMemoryError("Graph conversion");

  // Fill next array, set row pointers to list *tails*
  int s;
  for (s=0; s<num_nodes; s++) {
    if (row_pointer[s] == row_pointer[s+1]) {
      // empty row
      row_pointer[s] = -1;
      continue;
    }
    int e;
    for (e=row_pointer[s]; e<row_pointer[s+1]-1; e++) {
      next[e] = e+1;
    }
    e = row_pointer[s+1]-1;
    next[e] = row_pointer[s];
    row_pointer[s] = e;
  }
  row_pointer[num_nodes] = -1;

#ifdef DEBUG_GRAPH
  Output << "Static to dynamic, dynamic arrays are:\n";
  Output << "row_pointer: [";
  Output.PutArray(row_pointer, num_nodes);
  Output << "]\n";
  Output.flush();
  Output << "column_index: [";
  Output.PutArray(column_index, num_edges);
  Output << "]\n";
  Output.flush();
  Output << "        next: [";
  Output.PutArray(next, num_edges);
  Output << "]\n";
  Output.flush();
#endif
}

void digraph::Transpose()
{
  DCASSERT(IsDynamic());

  // we need another row_pointer array; everything else is "in place"
  int* col_pointer = (int *) malloc((num_nodes+1)*sizeof(int));
  if (NULL==col_pointer) OutOfMemoryError("Graph Transpose");
  int s;
  for (s=0; s<=num_nodes; s++) col_pointer[s] = -1;

  // Convert row lists into column lists
  for (s=0; s<num_nodes; s++) {
    if (row_pointer[s] < 0) continue; // empty list
    int front = next[row_pointer[s]];
    int ptr = front;
    do {
      int nextptr = next[ptr];  // save next ptr, it will be trashed
      // change column index to row
      int col = column_index[ptr];
      column_index[ptr] = s;
      // add entry to appropriate column
      if (col_pointer[col] < 0) {
	// empty column
	next[ptr] = ptr;
      } else {
	// add to end of column
	next[ptr] = col_pointer[col];
	next[col_pointer[col]] = ptr;
      }
      col_pointer[col] = ptr;
      // advance
      ptr = nextptr;  
    } while (ptr!=front);
  }

  // update row pointers and sizes
  free(row_pointer);
  row_pointer = col_pointer;
  nodes_alloc = num_nodes;

  // toggle
  isTransposed = !(isTransposed);

#ifdef DEBUG_GRAPH
  Output << "Transposed graph, dynamic arrays are now:\n";
  Output << "row_pointer: [";
  Output.PutArray(row_pointer, num_nodes);
  Output << "]\n";
  Output.flush();
  Output << "column_index: [";
  Output.PutArray(column_index, num_edges);
  Output << "]\n";
  Output.flush();
  Output << "        next: [";
  Output.PutArray(next, num_edges);
  Output << "]\n";
  Output.flush();
#endif
}

void digraph::ShowNodeList(OutputStream &s, int node)
{
  const char* fromstr = (isTransposed) ? "\tTo node " : "\tFrom node ";
  const char* tostr = (isTransposed) ? "\t\tFrom node " : "\t\tTo node ";
  int e;
  if (IsStatic()) {
    s << fromstr << node << "\n";
    for (e=row_pointer[node]; e<row_pointer[node+1]; e++) {
      s << tostr << column_index[e] << "\n";
    }
  } else {
    s << fromstr << node << "\n";
    if (row_pointer[node]<0) return;
    int front = next[row_pointer[node]];
    e = front;
    do {
      s << tostr << column_index[e] << "\n";
      e = next[e];
    } while (e!=front);
  }
}


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
    if (num_edges >= edges_alloc) ResizeEdges(2*edges_alloc);
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
    if (num_edges >= edges_alloc) ResizeEdges(2*edges_alloc);
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

  void Transpose();

  /// Dump to a stream (human readable)
  void ShowNodeList(OutputStream &s, int node);

};

// ******************************************************************
// *                                                                *
// *                     labeled_digraph methods                    *
// *                                                                *
// ******************************************************************


