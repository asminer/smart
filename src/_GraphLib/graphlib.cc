
#include "graphlib.h"

#include <stdio.h> 
#include <stdlib.h>
#include <string.h>

#include "sccs.h"

// External libraries

#include "intset.h"

#ifdef ALLOW_OLD_GRAPH_INTERFACE

const int MAJOR_VERSION = 2;
const int MINOR_VERSION = 5;

#else

const int MAJOR_VERSION = 3;
const int MINOR_VERSION = 0;

#endif

// #define DEBUG_DEFRAG
// #define DEBUG_TRANSPOSE_FROM

// ******************************************************************
// *                                                                *
// *                       Macros and such                          *
// *                                                                *
// ******************************************************************

inline void ShowArray(const char* name, const long* ptr, long N)
{
  if (0==ptr) {
    printf("%s: null\n", name);
    return;
  }
  printf("%s: [%ld", name, ptr[0]);
  for (long i=1; i<N; i++) printf(", %ld", ptr[i]);
  printf("]\n");
}

// ******************************************************************
// *                                                                *
// *                         error  methods                         *
// *                                                                *
// ******************************************************************

const char* GraphLib::error::getString() const
{
  switch (errcode) {
    case Not_Implemented:   return "Not implemented";
    case Bad_Index:         return "Bad index";
    case Out_Of_Memory:     return "Out of memory";
    case Format_Mismatch:   return "Format mismatch";
    case Finished_Mismatch: return "Finished graph";
    case Miscellaneous:     return "Misc. error";
  };
  return "Unknown error";
}


// ******************************************************************
// *                                                                *
// *                       timer_hook methods                       *
// *                                                                *
// ******************************************************************

GraphLib::timer_hook::timer_hook()
{
}

GraphLib::timer_hook::~timer_hook()
{
}

// ******************************************************************
// *                                                                *
// *                   BF_graph_traversal methods                   *
// *                                                                *
// ******************************************************************

GraphLib::BF_graph_traversal::BF_graph_traversal()
{
}

GraphLib::BF_graph_traversal::~BF_graph_traversal()
{
}

// ******************************************************************
// *                                                                *
// *                     BF_with_queue  methods                     *
// *                                                                *
// ******************************************************************

GraphLib::BF_with_queue::BF_with_queue(long max_queue)
 : BF_graph_traversal()
{
  queue_alloc = max_queue;
  queue = new long[max_queue];
  queueReset();
}

GraphLib::BF_with_queue::~BF_with_queue()
{
  delete[] queue;
}

bool GraphLib::BF_with_queue::hasNodesToExplore()
{
  return !queueEmpty();
}

long GraphLib::BF_with_queue::getNextToExplore()
{
  return queuePop();
}

// ******************************************************************
// *                                                                *
// *                    node_renumberer  methods                    *
// *                                                                *
// ******************************************************************

GraphLib::node_renumberer::node_renumberer()
{
}

GraphLib::node_renumberer::~node_renumberer()
{
}

// ******************************************************************
// *                                                                *
// *                    array_renumberer methods                    *
// *                                                                *
// ******************************************************************

GraphLib::array_renumberer::array_renumberer(long* nn)
{
  newnumber = nn;
}

GraphLib::array_renumberer::~array_renumberer()
{
  delete[] newnumber;
}

long GraphLib::array_renumberer::new_number(long s) const
{
  return newnumber[s];
}

// ******************************************************************
// *                                                                *
// *                   static_classifier  methods                   *
// *                                                                *
// ******************************************************************

GraphLib::static_classifier::static_classifier() 
{
  num_classes = 0;
  class_start = 0;
}

GraphLib::static_classifier::static_classifier(const static_classifier &SC)
{
  num_classes = 0;
  class_start = 0;
  replace(SC);
}

GraphLib::static_classifier::~static_classifier()
{
  delete[] class_start;
}

long GraphLib::static_classifier::classOfNode(long s) const 
{
  // Insane values:
  if (s<class_start[0]) return -1;
  if (s>=class_start[num_classes]) return num_classes;
  // Binary search
  // Find c such that class_start[c] <= s < class_start[c+1].
  // Invariant: class_start[low] <= s < class_start[high].
  // Stop when low + 1 == high.
  long low = 0; 
  long high = num_classes;
  while (high>low+1) {
    long mid = (high+low)/2;
    if (s< class_start[mid])  high = mid;
    else                      low = mid;
  }
  return low;
}

size_t GraphLib::static_classifier::getMemTotal() const
{
  return (1+num_classes)*sizeof(long);
}

void GraphLib::static_classifier::rebuild(long nc, long* sizes)
{
  delete[] class_start;
  class_start = sizes;
  num_classes = nc;
  // Shift everything to the right,
  // so that class_start[i] is the size of class i-1
  for (long c=num_classes; c>0; c--) {
    class_start[c] = class_start[c-1];
  }
  class_start[0] = 0;
  //
  // Accumulate, so that class_start[i] is equal to
  // the sum from j=0 to i-1 of size of class j
  //
  for (long c=2; c<=num_classes; c++) {
    class_start[c] += class_start[c-1];
  }
}

void GraphLib::static_classifier::replace(long nc, long* starts)
{
  delete[] class_start;
  class_start = starts;
  num_classes = nc;
  // TBD - should we check the array?
}

void GraphLib::static_classifier::replace(const static_classifier &SC)
{
  delete[] class_start;
  num_classes = SC.num_classes;
  class_start = new long[num_classes+1];
  memcpy(class_start, SC.class_start, (1+num_classes)*sizeof(long));
}

// ******************************************************************
// *                                                                *
// *                  abstract_classifier  methods                  *
// *                                                                *
// ******************************************************************

GraphLib::abstract_classifier::abstract_classifier(long ns, long nc)
{
  num_nodes = ns;
  num_classes = nc;
}

GraphLib::abstract_classifier::~abstract_classifier()
{
}

// ******************************************************************
// *                                                                *
// *                   general_classifier methods                   *
// *                                                                *
// ******************************************************************

GraphLib::general_classifier::general_classifier(long* C, long ns, long nc)
 : abstract_classifier(ns, nc)
{
  class_of_node = C;
}

GraphLib::general_classifier::~general_classifier()
{
  delete[] class_of_node;
}

long GraphLib::general_classifier::classOfNode(long s) const
{
  // TBD?
  // CHECK_RANGE(0, s, getNumNodes());
  return class_of_node[s];
}

GraphLib::node_renumberer*
GraphLib::general_classifier::buildRenumbererAndStatic(static_classifier &C) const
{
  //
  // First, count the number of states in each class.
  // Use an extra element because eventually these will be converted
  // to start indexes as needed for the static_classifier.
  //
  long* starts = new long[1+getNumClasses()];
  for (long i=0; i<=getNumClasses(); i++) {
    starts[i] = 0;
  }
  for (long i=0; i<getNumNodes(); i++) {
    starts[class_of_node[i]]++;
  }

  //
  // Determine starting index per class
  //
  for (long i=1; i<=getNumClasses(); i++) {
    starts[i] += starts[i-1];
  }
  for (long i=getNumClasses(); i; i--) {
    starts[i] = starts[i-1];
  }
  starts[0] = 0;

  //
  // Build the renumbering array.
  // For node s, its new number is the current index of its class
  // (and we increment the index).
  //
  long* renumber = new long[getNumNodes()];
  for (long i=0; i<getNumNodes(); i++) {
    renumber[i] = starts[ class_of_node[i] ]++;
  }

  //
  // Shift the sizes array to get the correct "starts" array
  //
  for (long i=getNumClasses(); i; i--) {
    starts[i] = starts[i-1];
  }
  starts[0] = 0;

  //
  // Package everything
  //
  replace_classifier(C, getNumClasses(), starts);
  return new array_renumberer(renumber);
}

// ******************************************************************
// *                                                                *
// *                      static_graph methods                      *
// *                                                                *
// ******************************************************************

GraphLib::static_graph::static_graph()
{
  num_nodes = 0;
  num_edges = 0;
  row_pointer = 0;
  column_index = 0;
  edge_bytes = 0;
  label = 0;
  is_by_rows = true;    // Arbitrary and useless
}

// ******************************************************************

GraphLib::static_graph::~static_graph()
{
  free(row_pointer);
  free(column_index);
  free(label);
}

// ******************************************************************

void GraphLib::static_graph::transposeFrom(const static_graph &m)
{
#ifdef DEBUG_TRANSPOSE_FROM
  printf("Inside transposeFrom\n");
  printf("  Input graph:\n");
  printf("\tis_by_rows: %s\n", m.is_by_rows ? "true" : "false");
  printf("\tnum_nodes: %ld\n", m.num_nodes);
  printf("\trow_pointer: ");
  ShowArray("\trow_pointer", m.row_pointer, m.num_nodes+1);
  ShowArray("\tcolumn_index: ", m.column_index, m.num_edges);
#endif

  is_by_rows = !m.is_by_rows;
  edge_bytes = m.edge_bytes;

  // resize arrays 
  allocate(m.num_nodes, m.num_edges);

  // Count entries in each transposed row
  for (long r=0; r<=num_nodes; r++) row_pointer[r] = 0;
  for (long e=0; e<m.row_pointer[m.num_nodes]; e++) {
    ++row_pointer[ m.column_index[e] ];
  }
#ifdef DEBUG_TRANSPOSE_FROM
  ShowArray("  index counts", row_pointer, num_nodes+1);
#endif

  // Accumulate, so row_pointer[r] gives #entries in rows 0..r
  for (long r=1; r<=num_nodes; r++) {
    row_pointer[r] += row_pointer[r-1];
  }
  // Shift.
  for (long r=num_nodes; r>0; r--) {
    row_pointer[r] = row_pointer[r-1];
  }
  row_pointer[0] = 0;
#ifdef DEBUG_TRANSPOSE_FROM
  ShowArray("  transposed row_pointer", row_pointer, num_nodes+1);
#endif

  //
  // Ok, right now row_pointer[r] is number of entries in rows 0..r-1.
  // In other words, it's the starting location for row r.
  // So we can start copying elements.
  long e = 0;
  for (long i=0; i<num_nodes; i++) {
    for (; e<m.row_pointer[i+1]; e++) {
      long j = m.column_index[e];

      // add element i,j,value[e]
#ifdef DEBUG_TRANSPOSE_FROM
      printf("  adding element [%ld, %ld]\n", i, j);
#endif

      column_index[ row_pointer[j] ] = i;
      if (edge_bytes) {
        memcpy(label + edge_bytes * row_pointer[j], m.label + edge_bytes * e, edge_bytes);
      }
      ++row_pointer[j];
    } // for e
  } // for i

  // Right now, row_pointer[r] gives #entries in rows 0..r,
  // so we need to shift again.
  for (long r=num_nodes; r>0; r--) {
    row_pointer[r] = row_pointer[r-1];
  }
  row_pointer[0] = 0;

#ifdef DEBUG_TRANSPOSE_FROM
  printf("  Output matrix:\n");
  printf("\tis_by_rows: %s\n", is_by_rows ? "true" : "false");
  printf("\tnum_nodes: %ld\n", num_nodes);
  ShowArray("\trow_pointer", row_pointer, num_nodes+1);
  ShowArray("\tcolumn_index", column_index, num_edges);
#endif
}

// ******************************************************************

void GraphLib::static_graph::emptyRows(intset &x) const
{
  x.removeAll();
  for (long s=0; s<num_nodes; s++) {
    if (row_pointer[s] == row_pointer[s+1]) {
      x.addElement(s);
    }
  }
}

// ******************************************************************

bool GraphLib::static_graph::traverse(BF_graph_traversal &t) const
{
  while (t.hasNodesToExplore()) {
      long s = t.getNextToExplore();

      if (s<0 || s>=num_nodes) {
        throw GraphLib::error(GraphLib::error::Bad_Index);
      }

      // explore edges to/from s
      for (long z=row_pointer[s]; z<row_pointer[s+1]; z++) {
        if (t.visit(s, column_index[z], label + z*edge_bytes)) return true;
      } // for z
  } // while
  return false;
}

// ******************************************************************

size_t GraphLib::static_graph::getMemTotal() const
{
  long mem = 0;
  if (row_pointer)  mem += (num_nodes+1) * sizeof(long);
  if (column_index) mem += num_edges * sizeof(long);
  if (label)        mem += num_edges * edge_bytes;
  return mem;
}

// ******************************************************************

void GraphLib::static_graph::allocate(long nodes, long edges)
{
  // row pointers
  // hopefully realloc is fast if we give the same size?
  long* nrp = (long*) realloc(row_pointer, (nodes+1)*sizeof(long));
  if (0==nrp) {
    throw error(error::Out_Of_Memory);
  }
  row_pointer = nrp;
  num_nodes = nodes;

  // column indexes
  long* nci = (long*) realloc(column_index, edges * sizeof(long)); 
  if (edges && 0==nci) {
    throw error(error::Out_Of_Memory);
  }
  column_index = nci;
  num_edges = edges;

  // labels
  unsigned char* nl = (unsigned char*) realloc(label, edges * edge_bytes);
  if ((edges * edge_bytes) && 0==nl) {
    throw error(error::Out_Of_Memory);
  }
  label = nl;
}



// ******************************************************************
// *                                                                *
// *                     dynamic_graph  methods                     *
// *                                                                *
// ******************************************************************

GraphLib::dynamic_graph::dynamic_graph(unsigned char es, bool ksl, bool md)
{
  edge_size = es;
  keep_self_loops = ksl;
  merge_duplicates = md;
  is_by_rows = true;
  nodes_alloc = 0;
  row_pointer = 0;
  edges_alloc = 0;
  column_index = 0;
  next = 0;
  label = 0;
  num_nodes = 0;
  num_edges = 0;
}

// ******************************************************************

GraphLib::dynamic_graph::~dynamic_graph()
{
  free(row_pointer);
  free(column_index);
  free(next);
  free(label);
}

// ******************************************************************

void 
GraphLib::dynamic_graph::addNodes(long count)
{
  const int MAX_NODE_ADD = 1024;

  long final_nodes = num_nodes+count;
  if (final_nodes > nodes_alloc) {
    long newnodes = nodes_alloc+1;
    while (newnodes <= final_nodes) {
      newnodes = MIN(2*newnodes, newnodes + MAX_NODE_ADD);
      newnodes = MAX(newnodes, 8L);
    }
    long* foo = (long*) realloc(row_pointer, newnodes*sizeof(long));
    if (0==foo) throw GraphLib::error(GraphLib::error::Out_Of_Memory);
    row_pointer = foo;
    nodes_alloc = newnodes-1;
  }
  for (; count; count--) {
    row_pointer[num_nodes] = -1;
    num_nodes++;
  }
}

// ******************************************************************

bool 
GraphLib::dynamic_graph::addEdge(long from, long to, const void* wt)
{
  const int MAX_EDGE_ADD = 1024;

  if (from == to && !keep_self_loops) return false;
  if (!is_by_rows) SWAP(from, to);
  if (from < 0 || from >= num_nodes || to < 0 || to >= num_nodes) {
    throw GraphLib::error(GraphLib::error::Bad_Index);
  }

  // get a new edge from "end"; allocate more space if necessary
  if (num_edges >= edges_alloc) {
    long newedges = MIN(2*edges_alloc, edges_alloc + MAX_EDGE_ADD);
    newedges = MAX(newedges, 16L);
    long* nci = (long *) realloc(column_index, newedges*sizeof(long));
    long* nn = (long *) realloc(next, newedges*sizeof(long));
    void* nl = 0;
    if (edge_size) {
      nl = realloc(label, newedges*edge_size);
      if (0==nl) throw GraphLib::error(GraphLib::error::Out_Of_Memory);
    }
    if ( (0==nci) || (0==nn) )
      throw GraphLib::error(GraphLib::error::Out_Of_Memory);
    column_index = nci;
    next = nn;
    label = (unsigned char*) nl;
    edges_alloc = newedges;
  }
  
  // fix the edge
  column_index[num_edges] = to;
  if (edge_size) memcpy(label+num_edges*edge_size, wt, edge_size);

  // add the new edge to the list.
  if (merge_duplicates) {
    if (!AddToMergedCircularList(row_pointer[from], num_edges))
      return true;
  } else {
    AddToUnmergedCircularList(row_pointer[from], num_edges);
  }
  num_edges++;
  return false;
}


// ******************************************************************

void 
GraphLib::dynamic_graph::removeEdges(BF_graph_traversal &t)
{
  while (t.hasNodesToExplore()) {
      long s = t.getNextToExplore();

      // Switch to linked lists, for simplicity
      if (row_pointer[s]<0) continue;  // it is empty
      long tail = row_pointer[s];
      row_pointer[s] = next[tail];
      next[tail] = -1;

      // traverse chain
      long prev = -1;
      long curr = row_pointer[s];
      while (curr >= 0) {
        if (t.visit(s, column_index[curr], label+curr*edge_size)) {
          curr = next[curr];
          if (prev>=0)   next[prev] = curr;
          else           row_pointer[s] = curr;
        } else {
          prev = curr;
          curr = next[curr];
        }
      } // while curr

      // switch back to circular lists
      if (row_pointer[s] < 0) continue;  // empty list
      if (prev < 0) {
        // list has one element, close the circle.
        next[row_pointer[s]] = row_pointer[s];
      } else {
        // prev is the tail, close the circle.
        next[prev] = row_pointer[s];
        row_pointer[s] = prev;
      }
  } // while
}

// ******************************************************************

void
GraphLib::dynamic_graph::renumberNodes(const node_renumberer &r)
{
  //
  // We need another row_pointer array; everything else is "in place"
  //
  long* tmp_row_lists = new long[num_nodes];
  for (long s=0; s<num_nodes; s++) tmp_row_lists[s] = -1;

  //
  // Do a transpose + renumber.
  // Convert old row lists into column lists, renumbering as we go.
  //
  for (long s=0; s<num_nodes; s++) {
    if (row_pointer[s]<0) continue; // empty row; skip
    // Convert current list from circular to linear
    long tail = row_pointer[s];
    row_pointer[s] = next[tail];
    next[tail] = -1;
    // Now, traverse the list, and do the transpose.
    // We won't bother to order the lists;
    // a second transpose will make that right.

    long nc = r.new_number(s);  // new column
    long ptrnext = -1;
    for (long ptr=row_pointer[s]; ptr>=0; ptr=ptrnext) {
      ptrnext = next[ptr];  // we're going to clobber next[ptr], so save it

      long nr = r.new_number(column_index[ptr]);  // new row
      //
      // Change edge ptr from (colindex, value) in list s
      // to (nc, value) in list nr
      //
      column_index[ptr] = nc;
      
      // Ok, add this to list tmp_row_lists[nr]

      next[ptr] = tmp_row_lists[nr];
      tmp_row_lists[nr] = ptr;
    }
    row_pointer[s] = -1;
  }

  //
  // Graph is renumbered, but transposed.
  // Transpose back, which will ensure the edges are in order.
  // Also, note that tmp_row_lists are (non-circular) linked lists.
  //
  long ptrnext = -1;
  for (long s=0; s<num_nodes; s++) {
    for (long ptr=tmp_row_lists[s]; ptr>=0; ptr=ptrnext) {
      ptrnext = next[ptr];  

      // Current edge is (colindex[ptr], value[ptr]) in list s;
      // Change it to (s, value[ptr]) in circular list colindex[ptr].

      long nr = column_index[ptr];
      column_index[ptr] = s;

      // Edge is good; add it to the appropriate list

      if (row_pointer[nr] < 0) {
        // Empty row; build one
        next[ptr] = ptr;
        row_pointer[nr] = ptr;
      } else {
        // Non-empty row; add past the tail
        long tail = row_pointer[nr];
        next[ptr] = next[tail];
        next[tail] = ptr;
        row_pointer[nr] = ptr;
      }
    }
  }

  // Cleanup
  delete[] tmp_row_lists;
}



// ******************************************************************

/*
void
GraphLib::dynamic_graph::transpose(timer_hook* sw)
{
  // we need another row_pointer array; everything else is "in place"
  long* old_row_pointer = (long *) malloc((num_nodes+1)*sizeof(long));
  if (0==old_row_pointer) 
    throw GraphLib::error(GraphLib::error::Out_Of_Memory);

  if (sw) sw->start("Transposing");

  long s;
  for (s=0; s<=num_nodes; s++) old_row_pointer[s] = -1;
  SWAP(row_pointer, old_row_pointer);
  nodes_alloc = num_nodes;

  // Convert old row lists into column lists
  for (s=0; s<num_nodes; s++) {
    // convert old list from circular to linear
    if (old_row_pointer[s]<0) continue;  // it is empty
    long tail = old_row_pointer[s];
    old_row_pointer[s] = next[tail];
    next[tail] = -1;
    // traverse linear list
    while (old_row_pointer[s]>=0) {
      long ptr = old_row_pointer[s];
      old_row_pointer[s] = next[ptr];
      // change column index to row
      long col = column_index[ptr];
      column_index[ptr] = s;
      AddToUnmergedCircularList(row_pointer[col], ptr);
    } // while
  } // for s

  // done with old matrix
  free(old_row_pointer);

  // toggle
  is_by_rows = !(is_by_rows);

  if (sw) sw->stop();
}
*/

// ******************************************************************

void
GraphLib::dynamic_graph::exportToStatic(static_graph &g, timer_hook *sw)
{
  if (sw) sw->start("Defragmenting graph");
  num_edges = Defragment(0);
  if (sw) sw->stop();

  if (sw) sw->start("Exporting graph");

  // Make space for g
  g.edge_bytes = edge_size;
  g.allocate(num_nodes, num_edges);

  // Copy everything over
  g.is_by_rows = is_by_rows;
  memcpy(g.row_pointer, row_pointer, (1+num_nodes) * sizeof(long));
  memcpy(g.column_index, column_index, num_edges * sizeof(long));
  if (edge_size) {
    memcpy(g.label, label, num_edges * edge_size);
  }

  if (sw) sw->stop();
}


// ******************************************************************

void
GraphLib::dynamic_graph::exportAndDestroy(static_graph &g, timer_hook *sw)
{
  if (sw) sw->start("Defragmenting graph");
  num_edges = Defragment(0);
  if (sw) sw->stop();

  if (sw) sw->start("Exporting graph");

  // Clean out any old g
  g.allocate(0, 0);

  // resize arrays
  if (nodes_alloc > num_nodes) {
    row_pointer = (long*) realloc(row_pointer, (1+num_nodes)*sizeof(long));
  } 
  if (edges_alloc > num_edges) {
    column_index = (long*) realloc(column_index, num_edges*sizeof(long));
    label = (unsigned char*) realloc(label, num_edges*edge_size);
  } 
  // Transfer things over
  g.num_nodes = num_nodes;
  g.num_edges = num_edges;
  g.edge_bytes = edge_size;
  g.is_by_rows = is_by_rows;
  g.row_pointer = row_pointer;
  g.column_index = column_index;
  g.label = label;

  // Destroy our pointers
  row_pointer = 0;
  column_index = 0;
  label = 0;
  free(next);
  next = 0;

  nodes_alloc = 0;
  edges_alloc = 0;
  num_nodes = 0;
  num_edges = 0;

  if (sw) sw->stop();
}

// ******************************************************************

void
GraphLib::dynamic_graph::splitAndExport(const static_classifier &C, bool ksl,
  static_graph &g_diag, static_graph &g_off, timer_hook *sw)
{
  if (sw) sw->start("Defragmenting graph");
  num_edges = Defragment(0);
  if (sw) sw->stop();

  if (sw) sw->start("Exporting graphs");

  //
  // We know the total number of edges.
  // Determine how many of these are "diagonal" edges
  // (i.e., are between nodes in the same class)
  // and how many are "off diagonal" edges.
  //
  // Do that by iterating over classes, then iterating
  // over states in the class, and examining the outgoing
  // edges for that state.
  //
  long diag_edges = 0;
  long off_edges = 0;
  for (long c=0; c<C.getNumClasses(); c++) {
    for (long s=C.firstNodeOfClass(c); s<=C.lastNodeOfClass(c); s++) {
      for (long edge=row_pointer[s]; edge<row_pointer[s+1]; edge++) {
        if (!ksl && s == column_index[edge]) continue;
        if (C.isNodeInClass(column_index[edge], c)) {
          diag_edges++;
        } else {
          off_edges++;
        }
      } // for edge
    } // for s
  } // for c
  DCASSERT(diag_edges + off_edges <= num_edges);

  //
  // Allocate space for g_diag and g_off
  //
  g_diag.is_by_rows = is_by_rows;
  g_diag.edge_bytes = edge_size;
  g_diag.allocate(num_nodes, diag_edges);

  g_off.is_by_rows = is_by_rows;
  g_off.edge_bytes = edge_size;
  g_off.allocate(num_nodes, off_edges);
  

  //
  // Copy edges over, using an iteration similar to the one where we
  // counted edges.
  //
  diag_edges = 0;
  off_edges = 0;
  for (long c=0; c<C.getNumClasses(); c++) {
    for (long s=C.firstNodeOfClass(c); s<=C.lastNodeOfClass(c); s++) {
      g_diag.row_pointer[s] = diag_edges;
      g_off.row_pointer[s] = off_edges;
      for (long edge=row_pointer[s]; edge<row_pointer[s+1]; edge++) {
        if (!ksl && s == column_index[edge]) continue;
        if (C.isNodeInClass(column_index[edge], c)) {
          // copy the edge over to g_diag
          g_diag.column_index[diag_edges] = column_index[edge];
          if (edge_size) {
            memcpy(g_diag.label+diag_edges*edge_size, 
                    label+edge*edge_size, edge_size);
          }
          // and count it
          diag_edges++;
        } else {
          // copy the edge over to g_off
          g_off.column_index[off_edges] = column_index[edge];
          if (edge_size) {
            memcpy(g_off.label+off_edges*edge_size, 
                    label+edge*edge_size, edge_size);
          }
          // and count it
          off_edges++;
        }
      } // for edge
    } // for s
  } // for c
  g_diag.row_pointer[num_nodes] = diag_edges;
  g_off.row_pointer[num_nodes] = off_edges;
  DCASSERT(g_diag.getNumEdges() == diag_edges);
  DCASSERT(g_off.getNumEdges() == off_edges);

  if (sw) sw->stop();
}

// ******************************************************************

size_t
GraphLib::dynamic_graph::getMemTotal() const
{
  long mem = 0;
  if (row_pointer)  mem += (nodes_alloc+1) * sizeof(long);
  if (column_index) mem += edges_alloc * sizeof(long);
  if (next)         mem += edges_alloc * sizeof(long);
  if (label)        mem += edges_alloc * edge_size;
  return mem;
}

// ******************************************************************

void 
GraphLib::dynamic_graph::clear()
{
  num_nodes = 0;
  num_edges = 0;
}

// ******************************************************************

bool
GraphLib::dynamic_graph::traverse(BF_graph_traversal &t) const
{
  while (t.hasNodesToExplore()) {
    long s = t.getNextToExplore();

    if (row_pointer[s] < 0)  continue;  // empty row
    long ptr = next[row_pointer[s]];
    long first = ptr;
    do {
      if (t.visit(s, column_index[ptr], label+ptr*edge_size)) return true;
      ptr = next[ptr];
    } while (ptr != first);

  } // while t
  return false;
}

// ******************************************************************

bool 
GraphLib::dynamic_graph::AddToMergedCircularList(long &list, long ptr)
{
  // Is the row empty?
  if (list < 0) {
    next[ptr] = ptr;  
    list = ptr;
    return true;
  } 
  long prev = list;
  // Is the new item past the tail?  (Most common case)
  if (column_index[ptr] > column_index[prev]) {
    next[ptr] = next[prev];
    next[prev] = ptr;
    list = ptr;
    return true;
  }
  // Is the new item equal to the tail?
  if (column_index[ptr] == column_index[prev]) {
    // Duplicate
    if (edge_size) merge_edges(label+prev*edge_size, label+ptr*edge_size);
    return false;
  }
  // Need to add somewhere in the middle.  Find the correct spot.
  while (1) {
    long curr = next[prev];
    if (column_index[ptr] < column_index[curr]) {
      // edge goes here!
      next[ptr] = curr;
      next[prev] = ptr;
      return true;
    }
    if (column_index[ptr] == column_index[curr]) {
      // Duplicate
      if (edge_size) merge_edges(label+curr*edge_size, label+ptr*edge_size);
      return false;
    }
    prev = curr;
  } // while 1
  // Never get here; keep compiler happy
  return false;
}

// ******************************************************************

void
GraphLib::dynamic_graph::AddToUnmergedCircularList(long &list, long ptr)
{
  // Is the row empty?
  if (list < 0) {
    next[ptr] = ptr;  
    list = ptr;
    return;
  } 
  long prev = list;
  // Is the new item past the tail?  (Most common case)
  if (column_index[ptr] >= column_index[prev]) {
    next[ptr] = next[prev];
    next[prev] = ptr;
    list = ptr;
    return;
  }
  // Need to add somewhere in the middle.  Find the correct spot.
  while (1) {
    long curr = next[prev];
    if (column_index[ptr] < column_index[curr]) {
      // edge goes here!
      next[ptr] = curr;
      next[prev] = ptr;
      break;
    }
    prev = curr;
  } // while 1
}

// ******************************************************************

long
GraphLib::dynamic_graph::Defragment(long first_slot)
{
  // make lists contiguous by swapping, forwarding pointers
  long i = first_slot; // everything before i is contiguous, after i is linked
  long s;

  unsigned char* tmp_label = (edge_size) ? (unsigned char*) malloc(edge_size) : 0;

  // convert everything from circular to linear
  for (s=0; s<num_nodes; s++) {
    long list = row_pointer[s];
    if (list<0) continue;
    row_pointer[s] = next[list];
    next[list] = -1;
  }
#ifdef DEBUG_DEFRAG
  printf("Pre-defrag lists:\n");
  ShowArray("row_pointer", row_pointer, num_nodes);
  ShowArray("col_index  ", column_index, num_edges);
  ShowArray("next       ", next, num_edges);
#endif
  for (s=0; s<num_nodes; s++) {
    long list = row_pointer[s];
    row_pointer[s] = i;
    // defragment the list
    while (list >= 0) {
      // traverse forwarding arcs if necessary...
      while (list<i) list = next[list];
      long nextlist = next[list];
      if (i!=list) {
        //
        // SWAP i and list
        //

        // Swap column indexes
        SWAP(column_index[i], column_index[list]);

        // Swap labels if any
        if (edge_size) {
          memcpy(tmp_label, label+i*edge_size, edge_size);
          memcpy(label+i*edge_size, label+list*edge_size, edge_size);
          memcpy(label+list*edge_size, tmp_label, edge_size);
        }

        // Set up forwarding information
        next[list] = next[i];
        next[i] = list;  // forwarding info

      } // if

      list = nextlist;
      i++;
    } // while list
  } // for s
  row_pointer[num_nodes] = i;
  free(tmp_label);
#ifdef DEBUG_DEFRAG
  printf("Post-defrag lists:\n");
  ShowArray("row_pointer", row_pointer, 1+num_nodes);
  ShowArray("col_index  ", column_index, num_edges);
  ShowArray("next       ", next, num_edges);
#endif
  return i;
}

// ******************************************************************
// *                                                                *
// *                    dynamic_digraph  methods                    *
// *                                                                *
// ******************************************************************

GraphLib::dynamic_digraph::dynamic_digraph(bool keep_self)
: dynamic_graph(0, keep_self, true)
{
  // nothing to do
}

void GraphLib::dynamic_digraph::merge_edges(void* ev, const void* nv) const
{
  // no edge labels
}

// ==========================================================================================================================================================================
// ==========================================================================================================================================================================
// ==========================================================================================================================================================================
// ==========================================================================================================================================================================
// ==========================================================================================================================================================================
// OLD INTERFACE BELOW, will eventually be discarded!
// ==========================================================================================================================================================================
// ==========================================================================================================================================================================
// ==========================================================================================================================================================================
// ==========================================================================================================================================================================
// ==========================================================================================================================================================================


#ifdef  ALLOW_OLD_GRAPH_INTERFACE

// ******************************************************************
// *                                                                *
// *                 generic_graph subclass methods                 *
// *                                                                *
// ******************************************************************


GraphLib::generic_graph::matrix::matrix()
{
  num_rows = 0;
  rowptr = 0;
  colindex = 0;
  value = 0;
  edge_size = 0;
}

void GraphLib::generic_graph::matrix::destroy()
{
  free(rowptr);
  free(colindex);
  free(value);
  num_rows = 0;
  rowptr = 0;
  colindex = 0;
  value = 0;
  edge_size = 0;
}

void GraphLib::generic_graph::matrix::copyFrom(const const_matrix &m)
{
  is_transposed = m.is_transposed;
  edge_size = m.edge_size;

  // resize arrays
  alloc(m.num_rows, m.rowptr[m.num_rows]);

  // copy arrays
  memcpy(rowptr, m.rowptr, (num_rows+1)*sizeof(long));
  memcpy(colindex, m.colindex, m.rowptr[m.num_rows] * sizeof(long));
  memcpy(value, m.value, m.rowptr[m.num_rows] * m.edge_size);
}


void GraphLib::generic_graph::matrix::transposeFrom(const const_matrix &m)
{
#ifdef DEBUG_TRANSPOSE_FROM
  printf("Inside transposeFrom\n");
  printf("  Input matrix:\n");
  printf("\tis_transposed: %s\n", m.is_transposed ? "true" : "false");
  printf("\tnum_rows: %ld\n", m.num_rows);
  printf("\trowptr: [%ld", m.rowptr[0]);
  for (long r=1; r<=m.num_rows; r++) {
    printf(", %ld", m.rowptr[r]);
  }
  printf("]\n");
  printf("\tcolindex: [%ld", m.colindex[0]);
  for (long e=1; e<m.rowptr[m.num_rows]; e++) {
    printf(", %ld", m.colindex[e]);
  }
  printf("]\n");
#endif

  // ASSUME matrix is square

  is_transposed = !m.is_transposed;
  edge_size = m.edge_size;

  // resize arrays 
  alloc(m.num_rows, m.rowptr[m.num_rows]);

  // Count entries in each transposed row
  for (long r=0; r<=num_rows; r++) rowptr[r] = 0;
  for (long e=0; e<m.rowptr[m.num_rows]; e++) {
    ++rowptr[ m.colindex[e] ];
  }
#ifdef DEBUG_TRANSPOSE_FROM
  printf("  index counts: [%ld", rowptr[0]);
  for (int r=1; r<=num_rows; r++) printf(", %ld", rowptr[r]);
  printf("]\n");
#endif

  // Accumulate, so rowptr[r] gives #entries in rows 0..r
  for (long r=1; r<=num_rows; r++) {
    rowptr[r] += rowptr[r-1];
  }
  // Shift.
  for (long r=num_rows; r>0; r--) {
    rowptr[r] = rowptr[r-1];
  }
  rowptr[0] = 0;
#ifdef DEBUG_TRANSPOSE_FROM
  printf("  transposed rowptr: [%ld", rowptr[0]);
  for (int r=1; r<=num_rows; r++) printf(", %ld", rowptr[r]);
  printf("]\n");
#endif

  //
  // Ok, right now rowptr[r] is number of entries in rows 0..r-1.
  // In other words, it's the starting location for row r.
  // So we can start copying elements.
  long e = 0;
  for (long i=0; i<num_rows; i++) {
    for (; e<m.rowptr[i+1]; e++) {
      long j = m.colindex[e];

      // add element i,j,value[e]
#ifdef DEBUG_TRANSPOSE_FROM
      printf("  adding element [%ld, %ld]\n", i, j);
#endif

      colindex[ rowptr[j] ] = i;
      if (edge_size) {
        memcpy((char*) value + edge_size * rowptr[j], (char*) m.value + edge_size * e, edge_size);
      }
      ++rowptr[j];
    } // for e
  } // for i

  // Right now, rowptr[r] gives #entries in rows 0..r,
  // so we need to shift again.
  for (long r=num_rows; r>0; r--) {
    rowptr[r] = rowptr[r-1];
  }
  rowptr[0] = 0;

#ifdef DEBUG_TRANSPOSE_FROM
  printf("  Output matrix:\n");
  printf("\tis_transposed: %s\n", is_transposed ? "true" : "false");
  printf("\tnum_rows: %ld\n", num_rows);
  printf("\trowptr: [%ld", rowptr[0]);
  for (long r=1; r<=num_rows; r++) {
    printf(", %ld", rowptr[r]);
  }
  printf("]\n");
  printf("\tcolindex: [%ld", colindex[0]);
  for (long e=1; e<rowptr[num_rows]; e++) {
    printf(", %ld", colindex[e]);
  }
  printf("]\n");
#endif
}


void GraphLib::generic_graph::matrix::alloc(long nr, long ne)
{
  // row pointers
  // hopefully realloc is fast if we give the same size?
  long* nrp = (long*) realloc(rowptr, (nr+1)*sizeof(long));
  if (0==nrp) {
    free(rowptr);
    throw error(error::Out_Of_Memory);
  }
  rowptr = nrp;
  num_rows = nr;

  // col indexes
  long* nci = (long*) realloc(colindex, ne * sizeof(long)); 
  if (rowptr[num_rows] && 0==nci) {
    free(colindex);
    throw error(error::Out_Of_Memory);
  }
  colindex = nci;

  // values
  void* nv = realloc(value, ne * edge_size);
  if ((ne * edge_size) && 0==nv) {
    free(value);
    throw error(error::Out_Of_Memory);
  }
  value = nv;
}


GraphLib::generic_graph::element_visitor::element_visitor()
{
}

GraphLib::generic_graph::element_visitor::~element_visitor()
{
}

// ******************************************************************
// *                                                                *
// *                     generic_graph  methods                     *
// *                                                                *
// ******************************************************************

GraphLib::generic_graph::generic_graph(int es, bool ksl, bool md)
{
  edge_size = es;
  keep_self_loops = ksl;
  merge_duplicates = md;
  finished = false;
  is_by_rows = true;
  nodes_alloc = 0;
  row_pointer = 0;
  edges_alloc = 0;
  column_index = 0;
  next = 0;
  label = 0;
  num_nodes = 0;
  num_edges = 0;
}

GraphLib::generic_graph::~generic_graph()
{
  free(row_pointer);
  free(column_index);
  free(next);
  free(label);
}

void 
GraphLib::generic_graph::addNodes(long count)
{
  const int MAX_NODE_ADD = 1024;

  if (finished) throw GraphLib::error(GraphLib::error::Finished_Mismatch);
  long final_nodes = num_nodes+count;
  if (final_nodes > nodes_alloc) {
    long newnodes = nodes_alloc+1;
    while (newnodes <= final_nodes) {
      newnodes = MIN(2*newnodes, newnodes + MAX_NODE_ADD);
      newnodes = MAX(newnodes, 8L);
    }
    long* foo = (long*) realloc(row_pointer, newnodes*sizeof(long));
    if (0==foo) throw GraphLib::error(GraphLib::error::Out_Of_Memory);
    row_pointer = foo;
    nodes_alloc = newnodes-1;
  }
  for (; count; count--) {
    row_pointer[num_nodes] = -1;
    num_nodes++;
  }
}

bool 
GraphLib::generic_graph::addEdge(long from, long to, const void* wt)
{
  const int MAX_EDGE_ADD = 1024;

  if (finished) throw GraphLib::error(GraphLib::error::Finished_Mismatch);
  if (from == to && !keep_self_loops) return false;
  if (!is_by_rows) SWAP(from, to);
  if (from < 0 || from >= num_nodes || to < 0 || to >= num_nodes) {
    throw GraphLib::error(GraphLib::error::Bad_Index);
  }

  // get a new edge from "end"; allocate more space if necessary
  if (num_edges >= edges_alloc) {
    long newedges = MIN(2*edges_alloc, edges_alloc + MAX_EDGE_ADD);
    newedges = MAX(newedges, 16L);
    long* nci = (long *) realloc(column_index, newedges*sizeof(long));
    long* nn = (long *) realloc(next, newedges*sizeof(long));
    void* nl = 0;
    if (edge_size) {
      nl = realloc(label, newedges*edge_size);
      if (0==nl) throw GraphLib::error(GraphLib::error::Out_Of_Memory);
    }
    if ( (0==nci) || (0==nn) )
      throw GraphLib::error(GraphLib::error::Out_Of_Memory);
    column_index = nci;
    next = nn;
    label = (char*) nl;
    edges_alloc = newedges;
  }
  
  // fix the edge
  column_index[num_edges] = to;
  if (edge_size) memcpy(label+num_edges*edge_size, wt, edge_size);

  // add the new edge to the list.
  if (merge_duplicates) {
    if (!AddToMergedCircularList(row_pointer[from], num_edges))
      return true;
  } else {
    AddToUnmergedCircularList(row_pointer[from], num_edges);
  }
  num_edges++;
  return false;
}

bool 
GraphLib::generic_graph::
haveSameEdges(long from, const generic_graph &g2, long from2) const
{
  if (finished) {
    if (g2.finished) {
      // COOL TRICK! We can compare memory directly!
      // check destination states
      long edges = row_pointer[from+1] - row_pointer[from];
      long edges2 = g2.row_pointer[from2+1] - g2.row_pointer[from];
      if (edges != edges2) return false;
      if (memcmp(
        column_index + row_pointer[from], 
        g2.column_index + g2.row_pointer[from2],
        edges * sizeof(long)
      )) return false;
      if (0==edge_size) return true;
      return 0==memcmp(
        label + row_pointer[from] * edge_size,
        g2.label + g2.row_pointer[from2] * edge_size,
        edges * edge_size
      );
    } else {
      long z = row_pointer[from];
      long zstop = row_pointer[from+1];
      if (g2.row_pointer[from2] < 0) {
        return (zstop == z);
      }
      long ptr = g2.next[g2.row_pointer[from2]];
      long first = ptr;
      do {
        if (z>=zstop) return false;
        if (column_index[z] != g2.column_index[ptr]) return false;
        if (edge_size) {
          if (memcmp(label + z*edge_size, g2.label + ptr*edge_size, edge_size))
            return false;
        }
        ptr = g2.next[ptr];
        z++;
      } while (ptr != first);
      return (z==zstop);
    } // if g2 finished
  } else {
    if (g2.finished) {
      long z = g2.row_pointer[from2];
      long zstop = g2.row_pointer[from2+1];
      if (row_pointer[from] < 0) {
        return (zstop == z);
      }
      long ptr = next[row_pointer[from]];
      long first = ptr;
      do {
        if (z>=zstop) return false;
        if (g2.column_index[z] != column_index[ptr]) return false;
        if (edge_size) {
          if (memcmp(g2.label + z*edge_size, label + ptr*edge_size, edge_size))
            return false;
        }
        ptr = next[ptr];
        z++;
      } while (ptr != first);
      return (z==zstop);
    } else {
      if (row_pointer[from] < 0) {
        return g2.row_pointer[from2] < 0;
      }
      if (g2.row_pointer[from2] < 0) return false;
      long ptr = next[row_pointer[from]];
      long ptr2 = g2.next[g2.row_pointer[from2]];
      long first = ptr;
      long first2 = ptr2;
      for (;;) {
        if (column_index[ptr] != g2.column_index[ptr2]) return false;
        if (edge_size) {
          if (memcmp(label+ptr*edge_size, g2.label+ptr2*edge_size, edge_size))
            return false;
        }
        ptr = next[ptr];
        ptr2 = g2.next[ptr2];
        if (ptr == first) return (ptr2 == first2);
        if (ptr2 == first2) return false;
      }
    } // if g2 finished
  } // if finished
}

bool
GraphLib::generic_graph::traverseFrom(long i, element_visitor &x) const
{
  if (is_by_rows) {
    if (i >= num_nodes || i < 0) return false;
    if (finished) {
      // static
      for (long z = row_pointer[i]; z<row_pointer[i+1]; z++) {
        if (x.visit(i, column_index[z], label+z*edge_size)) return true;
      } // for z
      return false;
    } 
    // dynamic
    if (row_pointer[i] < 0)  return false;  // empty row
    long ptr = next[row_pointer[i]];
    long first = ptr;
    do {
      if (x.visit(i, column_index[ptr], label+ptr*edge_size)) return true;
      ptr = next[ptr];
    } while (ptr != first);
    return false;
  } // if is_by_rows

  // still here?  by columns
  if (finished) {
    // static
    long z = row_pointer[0];
    for (long j=0; j<num_nodes; j++) for (; z<row_pointer[j+1]; z++) {
      if (i == column_index[z]) {
        if (x.visit(i, j, label+z*edge_size)) return true;
      } // if
    } // for j, z
    return false;
  }
  // dynamic
  for (long j=0; j<num_nodes; j++) {
    if (row_pointer[j]<0) continue;
    long ptr = row_pointer[j];
    long first = ptr;
    do {
      if (i == column_index[ptr]) {
        if (x.visit(i, j, label+ptr*edge_size)) return true;
      } // if
      ptr = next[ptr];
    } while (ptr != first);
  } // for j
  return false;
}

bool
GraphLib::generic_graph::traverseTo(long i, element_visitor &x) const
{
  if (!is_by_rows) {
    if (i >= num_nodes || i < 0) return false;
    if (finished) {
      // static
      for (long z = row_pointer[i]; z<row_pointer[i+1]; z++) {
        if (x.visit(column_index[z], i, label+z*edge_size)) return true;
      } // for z
      return false;
    } 
    // dynamic
    if (row_pointer[i] < 0)  return false;  // empty row
    long ptr = next[row_pointer[i]];
    long first = ptr;
    do {
      if (x.visit(column_index[ptr], i, label+ptr*edge_size)) return true;
      ptr = next[ptr];
    } while (ptr != first);
    return false;
  } // if is_by_rows

  // still here?  by columns
  if (finished) {
    // static
    long z = row_pointer[0];
    for (long j=0; j<num_nodes; j++) for (; z<row_pointer[j+1]; z++) {
      if (i == column_index[z]) {
        if (x.visit(j, i, label+z*edge_size)) return true;
      } // if
    } // for j, z
    return false; 
  }
  // dynamic
  for (long j=0; j<num_nodes; j++) {
    if (row_pointer[j]<0) continue;
    long ptr = row_pointer[j];
    long first = ptr;
    do {
      if (i == column_index[ptr]) {
        if (x.visit(j, i, label+ptr*edge_size)) return true;
      } // if
      ptr = next[ptr];
    } while (ptr != first);
  } // for j
  return false;
}

bool
GraphLib::generic_graph::traverseAll(element_visitor &x) const
{
  if (finished) {
    // static
    long z = row_pointer[0];
    if (is_by_rows) {
      for (long i=0; i<num_nodes; i++) for (; z<row_pointer[i+1]; z++) {
        if (x.visit(i, column_index[z], label+z*edge_size)) return true;
      } // for j, z
    } else {
      for (long j=0; j<num_nodes; j++) for (; z<row_pointer[j+1]; z++) {
        if (x.visit(column_index[z], j, label+z*edge_size)) return true;
      } // for j, z
    }
    return false;
  }
  // dynamic
  if (is_by_rows) {
    for (long i=0; i<num_nodes; i++) {
      if (row_pointer[i]<0) continue;
      long ptr = next[row_pointer[i]];
      long first = ptr;
      do {
        if (x.visit(i, column_index[ptr], label+ptr*edge_size)) return true;
        ptr = next[ptr];
      } while (ptr != first);
    } // for i
  } else {
    for (long j=0; j<num_nodes; j++) {
      if (row_pointer[j]<0) continue;
      long ptr = next[row_pointer[j]];
      long first = ptr;
      do {
        if (x.visit(column_index[ptr], j, label+ptr*edge_size)) return true;
        ptr = next[ptr];
      } while (ptr != first);
    } // for j
  }
  return false;
}

void 
GraphLib::generic_graph::removeEdges(element_visitor &x)
{
  if (finished) throw GraphLib::error(GraphLib::error::Finished_Mismatch);

  // For each row/column,
  for (long s=0; s<num_nodes; s++) {
    // Switch to linked lists, for simplicity (for now)
    if (row_pointer[s]<0) continue;  // it is empty
    long tail = row_pointer[s];
    row_pointer[s] = next[tail];
    next[tail] = -1;

    // traverse chain
    long prev = -1;
    long curr = row_pointer[s];
    while (curr >= 0) {
      bool remove;
      if (is_by_rows) 
        remove = x.visit(s, column_index[curr], label+curr*edge_size);
      else
        remove = x.visit(column_index[curr], s, label+curr*edge_size);
      if (remove) {
        curr = next[curr];
        if (prev>=0)   next[prev] = curr;
        else           row_pointer[s] = curr;
      } else {
        prev = curr;
        curr = next[curr];
      }
    } // while curr

    // switch back to circular lists
    if (row_pointer[s] < 0) continue;  // empty list
    if (prev < 0) {
      // list has one element, close the circle.
      next[row_pointer[s]] = row_pointer[s];
    } else {
      // prev is the tail, close the circle.
      next[prev] = row_pointer[s];
      row_pointer[s] = prev;
    }
  } // for s
}

void
GraphLib::generic_graph::renumber(const long* renum)
{
  if (finished) throw GraphLib::error(GraphLib::error::Finished_Mismatch);
  if (0==renum) throw GraphLib::error(GraphLib::error::Miscellaneous);
  
  bool* fixme = (bool*) malloc(num_nodes * sizeof(bool));
  if (0==fixme) throw GraphLib::error(GraphLib::error::Out_Of_Memory);

  long s;
  for (s=num_nodes-1; s>=0; s--) fixme[s] = (s!=renum[s]);

  // re-arrange row lists, by in-place swaps
  for (s=0; s<num_nodes; s++) if (fixme[s]) {
    long last = row_pointer[s];
    long p = s;
    while (1) {
      p = renum[p];
      SWAP(last, row_pointer[p]);
      fixme[p] = false;
      if (p==s) break;
    } // while 1
  } // for s's that need fixin'
  free(fixme);

  // re-arrange columns
  for (s=0; s<num_nodes; s++) {
    if (row_pointer[s]<0) continue;  // empty row
    long ptr = row_pointer[s];
    long first = ptr;
    do {
      column_index[ptr] = renum[column_index[ptr]];
      ptr = next[ptr];
    } while (ptr != first);
  } // for s

  // Column indexes may be out of order now.
  // A single transpose will put us right, though.
}

void
GraphLib::generic_graph::transpose(timer_hook* sw)
{
  if (finished) throw GraphLib::error(GraphLib::error::Finished_Mismatch);

  // we need another row_pointer array; everything else is "in place"
  long* old_row_pointer = (long *) malloc((num_nodes+1)*sizeof(long));
  if (0==old_row_pointer) 
    throw GraphLib::error(GraphLib::error::Out_Of_Memory);

  if (sw) sw->start("Transposing");

  long s;
  for (s=0; s<=num_nodes; s++) old_row_pointer[s] = -1;
  SWAP(row_pointer, old_row_pointer);
  nodes_alloc = num_nodes;

  // Convert old row lists into column lists
  for (s=0; s<num_nodes; s++) {
    // convert old list from circular to linear
    if (old_row_pointer[s]<0) continue;  // it is empty
    long tail = old_row_pointer[s];
    old_row_pointer[s] = next[tail];
    next[tail] = -1;
    // traverse linear list
    while (old_row_pointer[s]>=0) {
      long ptr = old_row_pointer[s];
      old_row_pointer[s] = next[ptr];
      // change column index to row
      long col = column_index[ptr];
      column_index[ptr] = s;
      AddToUnmergedCircularList(row_pointer[col], ptr);
    } // while
  } // for s

  // done with old matrix
  free(old_row_pointer);

  // toggle
  is_by_rows = !(is_by_rows);

  if (sw) sw->stop();
}

void
GraphLib::generic_graph::finish(const finish_options &o)
{
  if (finished) throw GraphLib::error(GraphLib::error::Finished_Mismatch);

  if (o.Store_By_Rows != is_by_rows) {
    transpose(o.report);
  }

  if (o.report) o.report->start("Converting to static");

  num_edges = Defragment(0);

  if (!o.Will_Clear) {
    // tighten up the memory usage
    free(next);
    next = 0;
    if (nodes_alloc > num_nodes) {
      row_pointer = (long*) realloc(row_pointer, (1+num_nodes)*sizeof(long));
      nodes_alloc = num_nodes;
    } // if nodes_alloc > num_nodes
    if (edges_alloc > num_edges) {
      column_index = (long*) realloc(column_index, num_edges*sizeof(long));
      edges_alloc = num_edges;
    } // if realloc the other arrays
  }
  finished = true;

  if (o.report) o.report->stop();
}

void
GraphLib::generic_graph::unfinish()
{
  if (!finished) throw GraphLib::error(GraphLib::error::Finished_Mismatch);

  if (0==next && edges_alloc>0) {
    next = (long*) malloc(edges_alloc * sizeof(long));
    if (0==next) throw GraphLib::error(GraphLib::error::Out_Of_Memory);
  }

  // first pass: get most of the next links correct
  for (long i=0; i<num_edges; i++) next[i] = i+1;

  // second pass: fix circular lists
  for (long i=0; i<num_nodes; i++) {
    if (row_pointer[i] == row_pointer[i+1]) {
      // empty row
      row_pointer[i] = -1;
      continue;
    }
    long tail = row_pointer[i+1]-1;
    next[tail] = row_pointer[i];  // close circle
    row_pointer[i] = tail;        // point to list tail
  }
  
  finished = false;
}

long 
GraphLib::generic_graph::computeTSCCs(timer_hook* sw, bool c, long* sccmap, long* aux) const
{
  if (0==sccmap || 0==aux) 
    throw GraphLib::error(GraphLib::error::Miscellaneous);

  // All these functions are implemented in sccs.cc
  if (sw) sw->start("Finding SCCs");
  long count = find_sccs(this, c, sccmap, aux);
  if (sw) sw->stop();

  if (sw) sw->start("Finding terminal SCCs");
  find_tsccs(this, sccmap, aux, count);
  if (sw) sw->stop();

  if (sw) sw->start("Renumbering SCCs");
  count = compact(this, sccmap, aux, count);
  if (sw) sw->stop();
  return count;
}

bool 
GraphLib::generic_graph::exportFinished(const_matrix &m) const
{
  if (!finished) return false;
  m.is_transposed = !is_by_rows;
  m.num_rows = num_nodes;
  m.rowptr = row_pointer;
  m.colindex = column_index;
  m.value = label;
  m.edge_size = edge_size;
  return true;
}

long 
GraphLib::generic_graph::ReportMemTotal() const
{
  long mem = 0;
  if (row_pointer)  mem += (nodes_alloc+1) * sizeof(long);
  if (column_index) mem += (edges_alloc) * sizeof(long);
  if (next)         mem += (edges_alloc) * sizeof(long);
  if (label)        mem += edges_alloc * edge_size;
  return mem;
}

void 
GraphLib::generic_graph::clear()
{
  if (0==next && nodes_alloc) {
    next = (long*) malloc(nodes_alloc * sizeof(long));
  }
  num_nodes = 0;
  num_edges = 0;
  finished = false;
}

//
// protected methods
//


// Return true if the edge was added; false if it was a duplicate.
bool 
GraphLib::generic_graph::AddToMergedCircularList(long &list, long ptr)
{
  // Is the row empty?
  if (list < 0) {
    next[ptr] = ptr;  
    list = ptr;
    return true;
  } 
  long prev = list;
  // Is the new item past the tail?  (Most common case)
  if (column_index[ptr] > column_index[prev]) {
    next[ptr] = next[prev];
    next[prev] = ptr;
    list = ptr;
    return true;
  }
  // Is the new item equal to the tail?
  if (column_index[ptr] == column_index[prev]) {
    // Duplicate
    if (edge_size) merge_edges(label+prev*edge_size, label+ptr*edge_size);
    return false;
  }
  // Need to add somewhere in the middle.  Find the correct spot.
  while (1) {
    long curr = next[prev];
    if (column_index[ptr] < column_index[curr]) {
      // edge goes here!
      next[ptr] = curr;
      next[prev] = ptr;
      return true;
    }
    if (column_index[ptr] == column_index[curr]) {
      // Duplicate
      if (edge_size) merge_edges(label+curr*edge_size, label+ptr*edge_size);
      return false;
    }
    prev = curr;
  } // while 1
  // Never get here; keep compiler happy
  return false;
}

void
GraphLib::generic_graph::AddToUnmergedCircularList(long &list, long ptr)
{
  // Is the row empty?
  if (list < 0) {
    next[ptr] = ptr;  
    list = ptr;
    return;
  } 
  long prev = list;
  // Is the new item past the tail?  (Most common case)
  if (column_index[ptr] >= column_index[prev]) {
    next[ptr] = next[prev];
    next[prev] = ptr;
    list = ptr;
    return;
  }
  // Need to add somewhere in the middle.  Find the correct spot.
  while (1) {
    long curr = next[prev];
    if (column_index[ptr] < column_index[curr]) {
      // edge goes here!
      next[ptr] = curr;
      next[prev] = ptr;
      break;
    }
    prev = curr;
  } // while 1
}

inline void ShowArray(const char* name, long* ptr, long N)
{
  printf("%s: [%ld", name, ptr[0]);
  for (long i=1; i<N; i++) printf(", %ld", ptr[i]);
  printf("]\n");
}

long
GraphLib::generic_graph::Defragment(long first_slot)
{
  // make lists contiguous by swapping, forwarding pointers
  long i = first_slot; // everything before i is contiguous, after i is linked
  long s;
  // convert everything from circular to linear
  for (s=0; s<num_nodes; s++) {
    long list = row_pointer[s];
    if (list<0) continue;
    row_pointer[s] = next[list];
    next[list] = -1;
  }
#ifdef DEBUG_DEFRAG
  printf("Pre-defrag lists:\n");
  ShowArray("row_pointer", row_pointer, num_nodes);
  ShowArray("col_index  ", column_index, num_edges);
  ShowArray("next       ", next, num_edges);
#endif
  for (s=0; s<num_nodes; s++) {
    long list = row_pointer[s];
    row_pointer[s] = i;
    // defragment the list
    while (list >= 0) {
      // traverse forwarding arcs if necessary...
      while (list<i) list = next[list];
      long nextlist = next[list];
      if (i!=list) DefragSwap(i, list);
      list = nextlist;
      i++;
    } // while list
  } // for s
  row_pointer[num_nodes] = i;
#ifdef DEBUG_DEFRAG
  printf("Post-defrag lists:\n");
  ShowArray("row_pointer", row_pointer, 1+num_nodes);
  ShowArray("col_index  ", column_index, num_edges);
  ShowArray("next       ", next, num_edges);
#endif
  return i;
}

bool
GraphLib::generic_graph::rowMult(const intset& rows, intset& y) const
{
  bool changed = false;
  long r = -1;
  if (finished) {
    for (r=rows.getSmallestAfter(r); r>=0; r=rows.getSmallestAfter(r)) {
      for (long z = row_pointer[r]; z<row_pointer[r+1]; z++) 
        if (!y.testAndAdd(column_index[z])) changed = true;
    } // for r
  } else {
    for (r=rows.getSmallestAfter(r); r>=0; r=rows.getSmallestAfter(r)) {
      long ptr = row_pointer[r];
      if (ptr < 0) continue;
      long first = ptr;
      do {
        if (!y.testAndAdd(column_index[ptr])) changed = true;
        ptr = next[ptr];
      } while (ptr != first);
    } // for r
  }
  return changed;
}

bool
GraphLib::generic_graph::colMult(const intset& cols, intset& y) const
{
  bool changed = false;
  long r;
  if (finished) {
    for (r=0; r<num_nodes; r++) {
      if (y.contains(r)) continue;
      for (long z = row_pointer[r]; z<row_pointer[r+1]; z++) {
        if (cols.contains(column_index[z])) {
          changed = true;
          y.addElement(r);
          break;
        }
      }
    } // for r
  } else {
    for (r=0; r<num_nodes; r++) {
      long ptr = row_pointer[r];
      if (ptr < 0 || y.contains(r)) continue;
      long first = ptr;
      do {
        if (cols.contains(column_index[ptr])) {
          changed = true;
          y.addElement(r);
          break;
        }
      } while (ptr != first);
    } // for r
  }
  return changed;
}

void
GraphLib::generic_graph::rp_empty(intset& x) const
{
  x.removeAll();
  if (finished) {
    for (long r=0; r<num_nodes; r++) {
      if (row_pointer[r] == row_pointer[r+1])
        x.addElement(r);
    }
  } else {
    // actually, just as fast
    for (long r=0; r<num_nodes; r++) {
      if (row_pointer[r]<0)
        x.addElement(r);
    }
  }
}

void
GraphLib::generic_graph::ci_empty(intset& x) const
{
  x.addAll();
  if (finished) {
    for (long z = row_pointer[num_nodes]-1; z>=0; z--)
      x.removeElement(column_index[z]);
  } else {
    // can't be sure that all column index slots are in use, 
    // so we need to do a proper traversal.
    for (long r=0; r<num_nodes; r++) {
      long ptr = row_pointer[r];
      if (ptr < 0) continue;
      long first = ptr;
      do {
        x.removeElement(column_index[ptr]);
      } while (ptr != first);
    }
  }
}

long
GraphLib::generic_graph::getFinishedReachable(long s, bool* reached, long* queue) const
{
  if (0==reached) return 0;
  if (reached[s]) return 0;
  queue[0] = s;
  reached[s] = true;
  long head = 1;
  long tail = 0;
  while (tail < head) {
    long i = queue[tail];
    tail++;
    for (long z=row_pointer[i]; z<row_pointer[i+1]; z++) {
      long j = column_index[z];
      if (!reached[j]) {
        reached[j] = true;
        queue[head] = j;
        head++;
      }
    } // for z
  } // while tail < head
  return head;
}

long
GraphLib::generic_graph::getUnfinishedReachable(long s, bool* reached, long* queue) const
{
  if (0==reached) return 0;
  if (reached[s]) return 0;
  queue[0] = s;
  reached[s] = true;
  long head = 1;
  long tail = 0;
  while (tail < head) {
    long i = queue[tail];
    tail++;
    long a = row_pointer[i];
    if (a<0) continue;
    long first = next[a];
    long ptr = first;
    do {
      long j = column_index[ptr];
      if (!reached[j]) {
        reached[j] = true;
        queue[head] = j;
        head++;
      }
      ptr = next[ptr];
    } while (ptr != first);
  } // while tail < head
  return head;
}


long
GraphLib::generic_graph::getFinishedReachable(long s, intset& reached, long* queue) const
{
  if (reached.contains(s)) return 0;
  queue[0] = s;
  reached.addElement(s);
  long head = 1;
  long tail = 0;
  while (tail < head) {
    long i = queue[tail];
    tail++;
    for (long z=row_pointer[i]; z<row_pointer[i+1]; z++) {
      long j = column_index[z];
      if (!reached.testAndAdd(j)) {
        queue[head] = j;
        head++;
      }
    } // for z
  } // while tail < head
  return head;
}

long
GraphLib::generic_graph::getUnfinishedReachable(long s, intset& reached, long* queue) const
{
  if (reached.contains(s)) return 0;
  queue[0] = s;
  reached.addElement(s);
  long head = 1;
  long tail = 0;
  while (tail < head) {
    long i = queue[tail];
    tail++;
    long a = row_pointer[i];
    if (a<0) continue;
    long first = next[a];
    long ptr = first;
    do {
      long j = column_index[ptr];
      if (!reached.testAndAdd(j)) {
        queue[head] = j;
        head++;
      }
      ptr = next[ptr];
    } while (ptr != first);
  } // while tail < head
  return head;
}


// ******************************************************************
// *                                                                *
// *                         other  methods                         *
// *                                                                *
// ******************************************************************

void GraphLib::digraph::DefragSwap(long i, long j) 
{
  // swap i and j, set up forward
  SWAP(column_index[i], column_index[j]);
  next[j] = next[i];
  next[i] = j;  // forwarding info
}

#endif // ALLOW_OLD_GRAPH_INTERFACE

// ******************************************************************
// *                                                                *
// *                                                                *
// *                      Front-end  functions                      *
// *                                                                *
// *                                                                *
// ******************************************************************



const char* GraphLib::Version()
{
  static char buffer[100];
  snprintf(buffer, sizeof(buffer), "Large (sparse) graph Library, version %d.%d",
     MAJOR_VERSION, MINOR_VERSION);
  return buffer;

  // TBD - revision number?
}

