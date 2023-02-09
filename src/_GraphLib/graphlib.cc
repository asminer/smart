
#include "graphlib.h"

#include <stdio.h> 
#include <stdlib.h>
#include <string.h>

#include "sccs.h"

// External libraries

#include "../_IntSets/intset.h"

const int MAJOR_VERSION = 3;
const int MINOR_VERSION = 0;

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
// *                  nochange_renumberer  methods                  *
// *                                                                *
// ******************************************************************

GraphLib::nochange_renumberer::nochange_renumberer()
{
}

GraphLib::nochange_renumberer::~nochange_renumberer()
{
}

long GraphLib::nochange_renumberer::new_number(long s) const
{
  return s;
}

bool GraphLib::nochange_renumberer::changes_something() const
{
  return false;
}

// ******************************************************************
// *                                                                *
// *                    array_renumberer methods                    *
// *                                                                *
// ******************************************************************

GraphLib::array_renumberer::array_renumberer(long* nn, long L)
{
  newnumber = nn;
  length = L;

  not_identity = false;
  for (long i=0; i<L; i++) {
    if (nn[i] != i) {
      not_identity = true;
      break;
    }
  }
}

GraphLib::array_renumberer::~array_renumberer()
{
  delete[] newnumber;
}

long GraphLib::array_renumberer::new_number(long s) const
{
  CHECK_RANGE(0, s, length);
  return newnumber[s];
}

bool GraphLib::array_renumberer::changes_something() const
{
  return not_identity;
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
  node_renumberer* R = new array_renumberer(renumber, getNumNodes());
  if (R->changes_something()) return R;
  // Clever bit here!
  delete R;
  return new nochange_renumberer;
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
  long* nci = 0;
  if (0==edges) {
    free(column_index);
  } else {
    nci = (long*) realloc(column_index, edges * sizeof(long)); 
    if (0==nci) {
      throw error(error::Out_Of_Memory);
    }
  }
  column_index = nci;
  num_edges = edges;

  // labels
  unsigned char* nl = 0;
  if (0==edges*edge_bytes) {
    free(label);
  } else {
    nl = (unsigned char*) realloc(label, edges * edge_bytes);
    if (0==nl) {
      throw error(error::Out_Of_Memory);
    }
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
  // Important special case
  //
  if (!r.changes_something()) {
    // we don't actually renumber anything
    return;
  }

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

void GraphLib::dynamic_graph::transpose(timer_hook* sw)
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

