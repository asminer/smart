
// $Id$

#include "sccs.h"

#include "../Templates/stack.h"

#define DEBUG_SCCS

// globals
digraph* scc_graph;
Stack <int> *visit_stack;
unsigned long visit_id;
unsigned long scc_count;
unsigned long* scc_val;

/* Based on Tarjan's SCC algorithm (apparently).
   Yanked from Sedgewick's Algorithms book (from undergrad!)
*/
unsigned long scc_visit(int k)
{
#ifdef DEBUG_SCCS
  Output << "Visiting node " << k << "\n";
  Output.flush();
#endif
  unsigned long min;
  visit_id++;
  scc_val[k] = visit_id;
  min = visit_id;
  visit_stack->Push(k);
   
  // visit "outgoing" edges from node k
  if (scc_graph->IsDynamic()) {
    // Dynamic graph, traverse circular list
    int edge = scc_graph->row_pointer[k];
    if (edge>=0) do {
      int j = scc_graph->column_index[edge];
#ifdef DEBUG_SCCS
      Output << "\tnode " << k << " --> node " << j << "\n"; 
#endif
      if (j!=k) {
        unsigned long m = (scc_val[j]) ? scc_val[j] : scc_visit(j);
        min = MIN(min, m);
      }
      edge = scc_graph->next[edge];
    } while (edge!=scc_graph->row_pointer[k]);
  } else {
    // Static graph
    int edge = scc_graph->row_pointer[k];
    for (; edge<scc_graph->row_pointer[k+1]; edge++) {
      int j = scc_graph->column_index[edge];
#ifdef DEBUG_SCCS
      Output << "\tnode " << k << " --> node " << j << "\n"; 
#endif
      if (j!=k) {
        unsigned long m = (scc_val[j]) ? scc_val[j] : scc_visit(j);
        min = MIN(min, m);
      }
    }
  } // if dynamic graph

#ifdef DEBUG_SCCS
  Output << "Done visiting node " << k << "; min=" << min << "\n"; 
  Output.flush();
#endif
  // done, now check if we were the first state entered in this scc
  // and if so, set up the mappings
  if (min == scc_val[k]) {
    int i;
#ifdef DEBUG_SCCS
  Output << "We have a SCC, numbered " << scc_count << "\n";
  Output.flush();
  Output << "SCC:\t";
#endif
    do {
      i = visit_stack->Pop();
      scc_val[i] = scc_count;
#ifdef DEBUG_SCCS
      Output << i << ", ";
#endif
    } while (i!=k);
    scc_count++;
#ifdef DEBUG_SCCS
    Output.Put('\n');
    Output.flush();
#endif
  }
  return min;
}


int 	ComputeSCCs(digraph *g, unsigned long* sccmap)
{
#ifdef DEBUG_SCCS
  Output << "Computing strongly connected components\n";
#endif
  Stack <int> foo(16, 1+g->NumNodes());
  scc_graph = g;
  visit_stack = &foo;
  visit_id = 0;
  scc_val = sccmap;
  scc_count = g->NumNodes()+1;
  int i;
  for (i=0; i<g->NumNodes(); i++) 
    if (0==sccmap[i]) scc_visit(i);

  scc_count -= g->NumNodes()+1;
  return scc_count;
}

