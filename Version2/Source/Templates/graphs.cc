
// $Id$

/*
    Directed graph classes, implementation.
*/



#include "graphs.h"


// #define DEBUG_GRAPH

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
  isDynamic = true;
}

digraph::~digraph()
{
  isDynamic = true;
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

  if (num_edges >= edges_alloc) 
	ResizeEdges(MIN(2*edges_alloc, edges_alloc+MAX_EDGE_ADD));
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
  if (num_edges >= edges_alloc) 
	ResizeEdges(MIN(2*edges_alloc, edges_alloc+MAX_EDGE_ADD));
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
  Output << "        next: [";
  Output.PutArray(next, num_edges);
  Output << "]\n";
  Output.flush();
#endif
  
  // make lists contiguous by swapping, forwarding pointers
  int i = 0; // everything before i is contiguous, after i is linked
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

  isDynamic = true;

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
	next[ptr] = next[col_pointer[col]];
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


