
// $Id$

/*
    Directed graph classes, implementation.
*/



#include "graphs.h"


//#define DEBUG_GRAPH

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

void digraph::AddToCircularList(int i, int ptr)
{
  if (row_pointer[i] < 0) {
    // row was empty before
    next[ptr] = ptr;  // circle of this node itself
    row_pointer[i] = ptr;
  } else {
    // nonempty row, add edge to tail of circular list
    int tail = row_pointer[i];
    next[ptr] = next[tail];
    next[tail] = ptr;
    row_pointer[i] = ptr;
  }
}

int digraph::AddToOrderedCircularList(int i, int ptr)
{
  if (row_pointer[i] < 0) {
    // row was empty before
    next[ptr] = ptr;  // circle of this node itself
    row_pointer[i] = ptr;
    return ptr;
  } 
  // Row is not empty.
  int prev = row_pointer[i];
  if (column_index[ptr] > column_index[prev]) {
    // Fast and easy special case: new edge at end of list
    next[ptr] = next[prev];
    next[prev] = ptr;
    row_pointer[i] = ptr;
    return ptr;
  }
  // Find spot for this edge
  while (1) {
    int curr = next[prev];
    if (column_index[ptr] < column_index[curr]) {
      // edge goes here!
      next[ptr] = curr;
      next[prev] = ptr;
      return ptr;
    }
    if (column_index[ptr] == column_index[curr]) {
      // duplicate edge, don't add it
      return curr;
    }
    prev = curr;
  }
  // never get here.
}

void digraph::Defragment(int first_slot)
{
  DCASSERT(IsDynamic());
#ifdef DEBUG_GRAPH
  Output << "Linked lists before defragment:\n";
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
  int i = first_slot; // everything before i is contiguous, after i is linked
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
  
#ifdef DEBUG_GRAPH
  Output << "Linked lists after defragment:\n";
  Output << "row_pointer: [";
  Output.PutArray(row_pointer, num_nodes);
  Output << "]\n";
  Output.flush();
  Output << "column_index: [";
  Output.PutArray(column_index, num_edges);
  Output << "]\n";
  Output.flush();
  Output << "  fwd / next: [";
  Output.PutArray(next, num_edges);
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
  int* old_row_pointer = (int *) malloc((num_nodes+1)*sizeof(int));
  if (NULL==old_row_pointer) OutOfMemoryError("Graph Transpose");

  CircularToTerminated();

  int s;
  for (s=0; s<=num_nodes; s++) old_row_pointer[s] = -1;
  SWAP(row_pointer, old_row_pointer);
  nodes_alloc = num_nodes;

  // Convert old row lists into column lists
  for (s=0; s<num_nodes; s++) {
    while (old_row_pointer[s]>=0) {
      int ptr = old_row_pointer[s];
      old_row_pointer[s] = next[ptr];
      // change column index to row
      int col = column_index[ptr];
      column_index[ptr] = s;
      AddToOrderedCircularList(col, ptr);
    } // while
  } // for s

  // done with old matrix
  free(old_row_pointer);

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


