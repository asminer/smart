
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "hyper.h"

// for errors
#include "mclib.h"

// #define DEBUG_COMPACT

const int MAX_NODE_ADD = 4096;
const int MAX_EDGE_ADD = 4096;

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

#include "circular.h"

// ******************************************************************
// *                   hypersparse_matrix methods                   *
// ******************************************************************

// ********** Public **********

hypersparse_matrix::hypersparse_matrix()
{
  is_static = false;

  num_rows = 0;
  num_edges = 0;
  row_tailptr = -1;
  
  // initialize rows
  rows_alloc = 4;
  row_index = (long*) malloc(rows_alloc * sizeof(long));
  row_pointer = (long*) malloc(rows_alloc * sizeof(long));
  next_row = (long*) malloc(rows_alloc * sizeof(long));

  // initialize edges
  edges_alloc = 4;
  column_index =  (long*) malloc(edges_alloc * sizeof(long));
  value = (float*) malloc(edges_alloc * sizeof(float));
  next = (long*) malloc(edges_alloc * sizeof(long));
}

hypersparse_matrix::~hypersparse_matrix()
{
  free(row_index);
  free(row_pointer);
  free(next_row);
  free(column_index);
  free(value);
  free(next);
}

bool hypersparse_matrix::AddElement(long from, long to, float wt)
{
  if (IsStatic()) {
    throw MCLib::error(MCLib::error::Finished_Mismatch);
  }

  long rptr = GetRow(from);

  // get a new edge from "end"; allocate more space if necessary
  if (num_edges >= edges_alloc) {
    long newedges = MIN(2*edges_alloc, edges_alloc + MAX_EDGE_ADD);
    ResizeEdges(newedges);
  }

  //add a new edge to the list for this row as necessary
  long prev = FindInCircular(to, row_pointer[rptr], column_index, next);
  if (prev < 0) {
    // empty list
    row_pointer[rptr] = num_edges;
    column_index[num_edges] = to;
    value[num_edges] = wt;
    next[num_edges] = num_edges;
    num_edges++;
    return 0;
  }
  if (to == column_index[prev]) {
    // duplicate edge
    value[prev] += wt;
    return 1;
  }
  // new edge
  column_index[num_edges] = to;
  value[num_edges] = wt;
  next[num_edges] = next[prev];
  next[prev] = num_edges;
  if (prev == row_pointer[rptr]) row_pointer[rptr] = num_edges;  // new tail
  num_edges++;
  return 0;
}

bool hypersparse_matrix::getFirstRow(long &index, long &handle)
{
  if (0==num_rows) return false;
  handle = 0;
  index = row_index[0];
  return true;
}

bool hypersparse_matrix::getNextRow(long &index, long &handle)
{
  handle++;
  if (handle >= num_rows) return false;
  index = row_index[handle];
  return true;
}

void hypersparse_matrix::NormalizeRows(const double* rowsums)
{
  if (IsStatic()) {
    long z = row_pointer[0];
    for (long r = 0; r<num_rows; r++) {
      for (; z<row_pointer[r+1]; z++) {
        value[z] /= rowsums[row_index[r]];
      } // for z
    } // for r
    return;
  }

  if (row_tailptr < 0) return;
  long r = row_tailptr;
  do {
    long z = row_pointer[r];
    do {
      value[z] /= rowsums[row_index[r]];
      z = next[z];
    } while (z != row_pointer[r]);
    r = next_row[r];
  } while (r != row_tailptr);
}

bool hypersparse_matrix
::traverseRow(long i, GraphLib::generic_graph::element_visitor &x)
{
  if (IsStatic()) {
    long r = findRowIndex(i);
    if (r<0) return false;    // empty row
      /*
    for (r=0; r<num_rows; r++) {
      if (i < row_index[r]) return false; // empty row
      if (i == row_index[r]) break;
    } // for r
    if (r >= num_rows) return false;  // empty row
      */
    for (long z = row_pointer[r]; z<row_pointer[r+1]; z++)
      if (x.visit(i, column_index[z], value+z)) return true;
    return false;
  }

  // find row, if non-empty
  if (row_tailptr < 0) return false;
  if (row_index[row_tailptr] < i) return false;
  long ptr = next_row[row_tailptr];
  for (;;) {
      if (i < row_index[ptr]) return false;  // empty row
      if (i > row_index[ptr]) {
        if (row_tailptr == ptr) return false;  // past the last nonempty row
        ptr = next_row[ptr];
        continue;
      }
      // i == row_index[ptr]
      break;
  } // loop
  
  // traverse row i
  long cp;
  for (cp=next[row_pointer[ptr]]; cp != row_pointer[ptr]; cp=next[cp]) {
    if (x.visit(i, column_index[cp], value+cp)) return true;
  } // for cp
  // cp should be row_pointer[ptr], the last element in the list
  return x.visit(i, column_index[cp], value+cp);
}

bool hypersparse_matrix
::traverseCol(long i, GraphLib::generic_graph::element_visitor &x)
{
  if (IsStatic()) {
    long z = row_pointer[0];
    for (long r = 0; r<num_rows; r++) {
      for (; z<row_pointer[r+1]; z++) {
        if (column_index[z] != i) continue;
        if (x.visit(row_index[r], i, value+z)) return true;
      } // for z
    } // for r
    return false;
  }

  if (row_tailptr < 0) return false;
  long r = row_tailptr;
  do {
    long z = row_pointer[r];
    do {
      if (i==column_index[z]) {
        if (x.visit(row_index[r], i, value+z)) return true;
      }
      z = next[z];
    } while (z != row_pointer[r]);
    r = next_row[r];
  } while (r != row_tailptr);
  return false;
}

bool hypersparse_matrix
::traverseAll(GraphLib::generic_graph::element_visitor &x)
{
  if (IsStatic()) {
    for (long r=0; r<num_rows; r++) {
      for (long z = row_pointer[r]; z<row_pointer[r+1]; z++)
        if (x.visit(row_index[r], column_index[z], value+z)) return true;
    } // for r
    return false;
  }
  // Dynamic
  if (row_tailptr < 0) return false;
  long ptr = next_row[row_tailptr];
  do {
    long cp;
    for (cp=next[row_pointer[ptr]]; cp != row_pointer[ptr]; cp=next[cp]) {
      if (x.visit(row_index[ptr], column_index[cp], value+cp)) return true;
    } // for cp
    // cp should be row_pointer[ptr], the last element in the list
    if (x.visit(row_index[ptr], column_index[cp], value+cp)) return true;
    ptr = next_row[ptr];
  } while (ptr != row_tailptr);
  return false;
}

void hypersparse_matrix::ConvertToStatic(bool normalize, bool tighten)
{
  if (IsStatic()) return;
#ifdef DEBUG_COMPACT
  printf("Dynamic matrix:\n");
  Dump();
#endif
  CircularToTerminated();
  if (normalize) {
    for (long i=0; i<num_rows; i++) {
      long ptr = row_pointer[i];
      // DCASSERT(ptr>=0);
      double total = 0.0;
      while (ptr>=0) {
        total += value[ptr];
        ptr = next[ptr];
      } // while
      // DCASSERT(total > 0);
      ptr = row_pointer[i];
      while (ptr>=0) {
        value[ptr] /= total;
        ptr = next[ptr];
      }
    } // for i
  }

  // Defragment the row list
  long i = 0;
  Defragment(row_tailptr, i, row_index, row_pointer, next_row);
  row_tailptr = 0;

  // Defragment the column entries
  i = 0;
  for (long r = 0; r<num_rows; r++) {
    long rptr = row_pointer[r];
    row_pointer[r] = i;
    Defragment(rptr, i, column_index, value, next);
  }
  row_pointer[num_rows] = i;

  if (tighten) {
    free(next_row);
    next_row = 0;
    free(next);
    next = 0;
    if (rows_alloc > num_rows) {
      row_pointer = (long*) realloc(row_pointer, (1+num_rows)*sizeof(long));
      row_index = (long*) realloc(row_index, (1+num_rows)*sizeof(long));
      rows_alloc = num_rows+1;
    } // if nodes_alloc > rpsize
    if (edges_alloc > num_edges) {
      column_index = (long*) realloc(column_index, num_edges*sizeof(long));
      value = (float*) realloc(value, num_edges*sizeof(float));
      edges_alloc = num_edges;
    } // if edges_alloc > num_edges
  }
  is_static = true;
#ifdef DEBUG_COMPACT
  printf("Static matrix:\n");
  Dump();
#endif
}

void hypersparse_matrix::ExportRow(long i, long& row, LS_Vector* A) const
{
  row = row_index[i];
  if (0==A) return;
  A->d_value = 0;
  if (IsStatic()) {
    A->size = (row_pointer[i+1] - row_pointer[i]);
    A->index = column_index + row_pointer[i];
    A->f_value = value + row_pointer[i];
  } else {
    A->size = 0;
    A->index = 0;
    A->f_value = 0;
  }
}

void hypersparse_matrix::ExportRowCopy(long i, LS_Vector &A) const
{
  if (!IsStatic()) throw MCLib::error(MCLib::error::Finished_Mismatch);
  A.d_value = 0;
  A.size = (row_pointer[i+1] - row_pointer[i]);
  if (0==A.size) {
    A.index = 0;
    A.f_value = 0;
    return;
  }
  long* index = (long*) malloc(A.size * sizeof(long));
  float* fval = (float*) malloc(A.size * sizeof(float));
  if (0==index || 0==fval) {
    free(index);
    free(fval);
    throw MCLib::error(MCLib::error::Out_Of_Memory);
  }
  memcpy(index, column_index + row_pointer[i], A.size * sizeof(long));
  memcpy(fval, value + row_pointer[i], A.size * sizeof(float));

  A.index = index;
  A.f_value = fval;
}

void hypersparse_matrix::Clear()
{
  if (is_static) {
    if (0==next) 
      next = (long*) malloc(edges_alloc * sizeof(long));
    if (0==next_row)
      next_row = (long*) malloc(rows_alloc * sizeof(long));
    is_static = false;
  }
  row_tailptr = -1;
  num_rows = 0;
  num_edges = 0;
}

inline void writeArray(const char* name, long* A, long n)
{
  printf("%s: ", name);
  if (A) {
    printf("[%ld", A[0]);
    for (long i=1; i<n; i++) printf(", %ld", A[i]);
    printf("]\n");
  } else printf("null\n");
}

inline void writeArray(const char* name, float* A, long n)
{
  printf("%s: ", name);
  if (A) {
    printf("[%f", A[0]);
    for (long i=1; i<n; i++) printf(", %f", A[i]);
    printf("]\n");
  } else printf("null\n");
}

void hypersparse_matrix::Dump()
{
  printf("num_rows: %ld\n", num_rows);
  printf("rows_alloc: %ld\n", rows_alloc);
  printf("row_tailptr: %ld\n", row_tailptr);
  printf("num_edges: %ld\n", num_edges);
  printf("edges_alloc: %ld\n", edges_alloc);
  writeArray("row_index", row_index, num_rows);
  writeArray("row_pointer", row_pointer, 1+num_rows);
  writeArray("next_row", next_row, num_rows);
  writeArray("column_index", column_index, num_edges);
  writeArray("value", value, num_edges);
  writeArray("next", next, num_edges);
}

long hypersparse_matrix::ReportMemTotal() const 
{
  long mem = 0;
  if (row_pointer)    mem += rows_alloc * sizeof(long);
  if (row_index)      mem += rows_alloc * sizeof(long);
  if (next_row)       mem += rows_alloc * sizeof(long);
  if (column_index)   mem += edges_alloc * sizeof(long);
  if (value)          mem += edges_alloc * sizeof(float);
  if (next)           mem += edges_alloc * sizeof(long);
  return mem;
}


bool hypersparse_matrix::getForward(const intset& x, intset &y) const
{
  long answer = false;
  if (IsStatic()) {
    for (long r = 0; r<num_rows; r++) 
      if (x.contains(row_index[r])) 
        for (long z=row_pointer[r]; z<row_pointer[r+1]; z++)
          if (!y.testAndAdd(column_index[z])) answer = true;
    return answer;
  }
  // Dynamic
  if (row_tailptr < 0) return answer;
  long r = row_tailptr;
  do {
    long z = row_pointer[r];
    if (x.contains(row_index[r]))
      do {
        if (!y.testAndAdd(column_index[z])) answer = true;
        z = next[z];
      } while (z != row_pointer[r]);
    r = next_row[r];
  } while (r != row_tailptr);
  return answer;
}

bool hypersparse_matrix::getBackward(const intset& y, intset &x) const
{
  long answer = false;
  if (IsStatic()) {
    for (long r = 0; r<num_rows; r++) 
      if (!x.contains(row_index[r])) 
        for (long z=row_pointer[r]; z<row_pointer[r+1]; z++) 
          if (y.contains(column_index[z])) {
            x.addElement(row_index[r]);
            answer = true;
            break;
          }
    return answer;
  }
  // Dynamic
  if (row_tailptr < 0) return answer;
  long r = row_tailptr;
  do {
    long z = row_pointer[r];
    if (!x.contains(row_index[r]))
      do {
        if (y.contains(column_index[z])) {
          x.addElement(row_index[r]);
          answer = true;
          break;
        }
        z = next[z];
      } while (z != row_pointer[r]);
    r = next_row[r];
  } while (r != row_tailptr);
  return answer;
}

void hypersparse_matrix::selectEdge(long &state, double acc, double u) const
{
  if (!IsStatic()) return;
  long i = findRowIndex(state);
  if (i<0) return;

  for (long z = row_pointer[i]; z<row_pointer[i+1]; z++) {
    acc += value[z];
    if (acc >= u) {
      state = column_index[z];
      return;
    }
  } // for z
}

// ********** Protected **********

long hypersparse_matrix::GetRow(long ri)
{
  long ptr = FindInCircular(ri, row_tailptr, row_index, next_row);
  if (ptr < 0 || row_index[ptr] != ri) {
    // need to add row
    if (num_rows+1 >= rows_alloc) {
      long newrows = MIN(2*rows_alloc, rows_alloc + MAX_NODE_ADD);
      if (newrows < 0) {
        throw MCLib::error(MCLib::error::Out_Of_Memory);
      }
      long* nri = (long*) realloc(row_index, newrows * sizeof(long));
      long* nrp = (long*) realloc(row_pointer, newrows * sizeof(long));
      long* nnr = (long*) realloc(next_row, newrows * sizeof(long));
      if ((0==nri) || (0==nrp) || (0==nnr)) {
        throw MCLib::error(MCLib::error::Out_Of_Memory);
      }
      rows_alloc = newrows;
      row_index = nri;
      row_pointer = nrp;
      next_row = nnr;
    }
    row_index[num_rows] = ri;
    row_pointer[num_rows] = -1;
    if (ptr < 0) {
      row_tailptr = num_rows;   
      next_row[num_rows] = num_rows;
    } else {
      next_row[num_rows] = next_row[ptr];
      next_row[ptr] = num_rows;
      if (ptr == row_tailptr) row_tailptr = num_rows;
    }
    ptr = num_rows;
    num_rows++;
  } 
  return ptr;
}

void hypersparse_matrix::ResizeEdges(long new_edges)
{
  if (IsStatic()) {
    throw MCLib::error(MCLib::error::Finished_Mismatch);
  }
  long* nci = (long *) realloc(column_index, new_edges*sizeof(long));
  float* nv = (float *) realloc(value, new_edges*sizeof(float));
  long* nn = (long *) realloc(next, new_edges*sizeof(long));
  if (new_edges) {
    if ((0==nci) || (0==nn) || (0==nv)) {
      throw MCLib::error(MCLib::error::Out_Of_Memory);
    }
  }
  column_index = nci;
  value = nv;
  next = nn;
  edges_alloc = new_edges;
}


void hypersparse_matrix::CircularToTerminated()
{
  // fix list of rows:
  if (row_tailptr >= 0) {
    long tail = row_tailptr;
    row_tailptr = next_row[tail];
    next_row[tail] = -1;   
  }

  // fix each row list:
  for (long s=0; s<num_rows; s++) {
      if (row_pointer[s]<0) continue;
      long tail = row_pointer[s];    
      row_pointer[s] = next[tail];  
      next[tail] = -1;
  }
}

