
// $Id$

#include "sccs.h"

#include "../Templates/stack.h"

// globals
digraph* scc_graph;
Stack <int> *visit_stack;
unsigned int visit_id;
unsigned int scc_count;
unsigned int* scc_val;

/* Based on Tarjan's SCC algorithm (apparently).
   Yanked from Sedgewick's Algorithms book (from undergrad!)
*/
unsigned int scc_dynamic_visit(int k)
{
  DCASSERT(scc_graph->IsDynamic());
  unsigned int min;
  visit_id++;
  scc_val[k] = visit_id;
  min = visit_id;
  visit_stack->Push(k);
   
  // visit "outgoing" edges from node k
  int edge = scc_graph->row_pointer[k];
  if (edge>=0) do {
    int j = scc_graph->column_index[edge];
    if (j!=k) {
      unsigned int m = (scc_val[j]) ? scc_val[j] : scc_dynamic_visit(j);
      min = MIN(min, m);
    }
    edge = scc_graph->next[edge];
  } while (edge!=scc_graph->row_pointer[k]);

  // done, now check if we were the first state entered in this scc
  // and if so, set up the mappings
  if (min == scc_val[k]) {
    int i;
    do {
      i = visit_stack->Pop();
      scc_val[i] = scc_count;
    } while (i!=k);
    scc_count++;
  }
  return min;
}

unsigned int scc_static_visit(int k)
{
  DCASSERT(scc_graph->IsStatic());
  unsigned int min;
  visit_id++;
  scc_val[k] = visit_id;
  min = visit_id;
  visit_stack->Push(k);
   
  // visit "outgoing" edges from node k
  int edge = scc_graph->row_pointer[k];
  for (; edge<scc_graph->row_pointer[k+1]; edge++) {
    int j = scc_graph->column_index[edge];
    if (j!=k) {
      unsigned int m = (scc_val[j]) ? scc_val[j] : scc_static_visit(j);
      min = MIN(min, m);
    }
  }

  // done, now check if we were the first state entered in this scc
  // and if so, set up the mappings
  if (min == scc_val[k]) {
    int i;
    do {
      i = visit_stack->Pop();
      scc_val[i] = scc_count;
    } while (i!=k);
    scc_count++;
  }
  return min;
}

int 	ComputeSCCs(digraph *g, unsigned int* sccmap)
{
  Stack <int> foo(16, 1+g->NumNodes());
  scc_graph = g;
  visit_stack = &foo;
  visit_id = 0;
  scc_val = sccmap;
  scc_count = g->NumNodes()+1;
  int i;
  if (g->IsDynamic())
    for (i=0; i<g->NumNodes(); i++) 
      if (0==sccmap[i]) scc_dynamic_visit(i);
  else
    for (i=0; i<g->NumNodes(); i++) 
      if (0==sccmap[i]) scc_static_visit(i);

  scc_count -= g->NumNodes()+1;
  return scc_count;
}

