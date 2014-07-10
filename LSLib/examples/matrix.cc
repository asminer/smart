
// $Id$

#include "matrix.h"

#include <stdlib.h>

// ******************************************************************
// *                                                                *
// *                       Macros and such                          *
// *                                                                *
// ******************************************************************

/// SWAP "macro".
template <class T> inline void SWAP(T &x, T &y) { T tmp=x; x=y; y=tmp; }

/// Standard MAX "macro".
template <class T> inline T MAX(T X,T Y) { return ((X>Y)?X:Y); }

/// Standard MIN "macro".
template <class T> inline T MIN(T X,T Y) { return ((X<Y)?X:Y); }


// ******************************************************************
// *                     sparse_matrix  methods                     *
// ******************************************************************

// ********** Public **********

sparse_matrix::sparse_matrix(long nodes, long edges, bool transpose)
{
  is_by_rows = !transpose;
  is_static = false;
  num_nodes = nodes;
  num_edges = 0;  // they will be added

  nodes_alloc = (nodes>0) ? (nodes+1) : 0;
  if (nodes_alloc) 
    row_pointer = (long*) malloc(nodes_alloc * sizeof(long));
  else
    row_pointer = NULL;
  for (long i=0; i<nodes_alloc; i++) row_pointer[i] = -1;
  
  edges_alloc = (edges>0) ? edges : 0;
  if (edges_alloc) {
    column_index =  (long*) malloc(edges_alloc * sizeof(long));
    value = (float*) malloc(edges_alloc * sizeof(float));
    next = (long*) malloc(edges_alloc * sizeof(long));
  } else {
    column_index = NULL;
    value = NULL;
    next = NULL;
  }
}

sparse_matrix::~sparse_matrix()
{
  free(row_pointer);
  free(column_index);
  free(value);
  free(next);
}

void sparse_matrix::AddElement(long from, long to, double wt)
{
  if (IsStatic()) return;
  if ( (from<0) || (from>=num_nodes) || (to<0) || (to>=num_nodes) ) return;
  if (num_edges >= edges_alloc) return;

  if (!is_by_rows)  SWAP(from, to);

  // fix a new edge
  column_index[num_edges] = to;
  value[num_edges] = wt;

  // now, add the new edge to the list.
  AddToCircularList(from, num_edges);
  num_edges++;
}

void sparse_matrix::ConvertToStatic()
{
  if (IsStatic()) return;
  CircularToTerminated();
  Defragment(0);
  // Trash next array
  free(next);
  next = NULL;
  is_static = true;
}

void sparse_matrix::ExportTo(LS_Matrix &A) const
{
  if (!IsStatic()) {
    A.start = 0;
    A.stop = 0;
    A.rowptr = 0;
    A.colindex = 0;
    A.f_value = 0;
    A.d_value = 0;
    return;
  } 

  A.is_transposed = !is_by_rows;
  A.start = 0;
  A.stop = num_nodes;
  A.rowptr = row_pointer;
  A.colindex = column_index;
  A.f_value = value;
  A.d_value = NULL;
}

// ********** Protected **********

bool sparse_matrix::ResizeNodes(long new_nodes)
{
  if (IsStatic()) return false;
  if (nodes_alloc == new_nodes) return true;
  long* foo = (long *) realloc(row_pointer, (1+new_nodes)*sizeof(long));
  if (new_nodes && (NULL==foo)) return false;
  row_pointer = foo;
  nodes_alloc = new_nodes;
  return true;
}

bool sparse_matrix::ResizeEdges(long new_edges)
{
  if (IsStatic()) return false;
  if (edges_alloc == new_edges) return true;
  long* nci = (long *) realloc(column_index, new_edges*sizeof(long));
  float* nv = (float *) realloc(value, new_edges*sizeof(float));
  long* nn = (long *) realloc(next, new_edges*sizeof(long));
  if (new_edges) {
    if ((NULL==nci) || (NULL==nn) || (NULL==nv)) {
      return false;
    }
  }
  column_index = nci;
  value = nv;
  next = nn;
  edges_alloc = new_edges;
  return true;
}

void sparse_matrix::AddToCircularList(long i, long ptr)
{
  if (row_pointer[i] < 0) {
    // row was empty before
    next[ptr] = ptr;  // circle of this node itself
    row_pointer[i] = ptr;
  } else {
    // nonempty row, add edge to tail of circular list
    long tail = row_pointer[i];
    next[ptr] = next[tail];
    next[tail] = ptr;
    row_pointer[i] = ptr;
  }
}

void sparse_matrix::CircularToTerminated()
{
  long s;
  for (s=0; s<num_nodes; s++) {
      if (row_pointer[s]<0) continue;
      long tail = row_pointer[s];    
      row_pointer[s] = next[tail];  
      next[tail] = -1;
  }
}

void sparse_matrix::Defragment(long first_slot)
{
  // make lists contiguous by swapping, forwarding pointers
  long i = first_slot; // everything before i is contiguous, after i is linked
  long s;
  for (s=0; s<num_nodes; s++) {
    long list = row_pointer[s];
    row_pointer[s] = i;
    while (list >= 0) {
      // traverse forwarding arcs if necessary...
      while (list<i) list = next[list];
      long nextlist = next[list];
      if (i!=list) {
        // swap i and list, set up forward
        next[list] = next[i];
        SWAP(column_index[i], column_index[list]);
        SWAP(value[i], value[list]);
        next[i] = list;  // forwarding info
      }
      list = nextlist;
      i++;
    } // while list
  } // for s
  row_pointer[num_nodes] = i;
}

