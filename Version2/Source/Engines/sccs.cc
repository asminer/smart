
// $Id$

#include "sccs.h"

#include "../Templates/stack.h"

#define DEBUG_SCCS

// globals
digraph* scc_graph;
int* visit_stack;
int visit_stack_top;
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
  // Push k
  DCASSERT(visit_stack_top<scc_graph->NumNodes());
  visit_stack[visit_stack_top++] = k;
   
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
  Output << "We have a SCC, numbered ";
  Output << scc_count-scc_graph->NumNodes() << "\t(actual " << scc_count;
  Output << ")\n";
  Output.flush();
  Output << "SCC:\t";
#endif
    do {
      // Pop i 
      DCASSERT(visit_stack_top>0);
      i = visit_stack[--visit_stack_top];
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
  scc_graph = g;
  visit_stack = new int[g->NumNodes()];
  visit_id = 0;
  scc_val = sccmap;
  scc_count = g->NumNodes();
  int i;
  for (i=0; i<g->NumNodes(); i++) 
    if (0==sccmap[i]) scc_visit(i);
  delete[] visit_stack;

  for (i=0; i<g->NumNodes(); i++) sccmap[i] -= g->NumNodes();
  scc_count -= g->NumNodes();
  return scc_count;
}




void FwdArcsFindTerminal(digraph *g, unsigned long* sccmap, int* isterm)
{
  DCASSERT(g->isTransposed == false);
  
  // For each state
  for (int i=0; i<g->NumNodes(); i++) {
    // Get the class for this state
    int c = sccmap[i] - g->NumNodes();
    // If the class is still thought to be terminal
    if (isterm[c]) {
      // check all outgoing arcs from this state
      if (g->IsDynamic()) {
        // Dynamic graph, traverse circular list
        int edge = g->row_pointer[i];
        if (edge>=0) do {
          int j = g->column_index[edge];
	  // there is an arc from i to j
	  if (sccmap[j] != sccmap[i]) {
	    // arc to another class, we cannot be a terminal scc
	    isterm[c] = 0;
#ifdef DEBUG_SCCS
	    Output << "SCC " << c << " is not terminal, due to arc from ";
	    Output << i << " to " << j << "\n";
	    Output.flush();
#endif
          }
          edge = g->next[edge];
        } while (edge!=g->row_pointer[i]);
        // end of dynamic graph portion
      } else { 
        // Static graph
        int edge = scc_graph->row_pointer[i];
        for (; edge<scc_graph->row_pointer[i+1]; edge++) {
          int j = scc_graph->column_index[edge];
	  // there is an arc from i to j
	  if (sccmap[j] != sccmap[i]) {
	    // arc to another class, we cannot be a terminal scc
	    isterm[c] = 0;
#ifdef DEBUG_SCCS
	    Output << "SCC " << c << " is not terminal, due to arc from ";
	    Output << i << " to " << j << "\n";
	    Output.flush();
#endif
          }
        }
        // end of static graph portion
      } // if dynamic graph
    } // if isterm[c]
  } // for i
}

void BackArcsFindTerminal(digraph *g, unsigned long* sccmap, int* isterm)
{
  DCASSERT(g->isTransposed);
  
  // For each state
  for (int j=0; j<g->NumNodes(); j++) {
    // check all incoming arcs to this state
    if (g->IsDynamic()) {
      // Dynamic graph, traverse circular list
      int edge = g->row_pointer[j];
      if (edge>=0) do {
        int i = g->column_index[edge];
	// there is an arc from i to j
	if (sccmap[j] != sccmap[i]) {
	  // arc to another class, node i cannot be in a terminal scc
          int c = sccmap[i] - g->NumNodes();
	  isterm[c] = 0;
#ifdef DEBUG_SCCS
	  Output << "SCC " << c << " is not terminal, due to arc from ";
	  Output << i << " to " << j << "\n";
	  Output.flush();
#endif
        }
        edge = g->next[edge];
      } while (edge!=g->row_pointer[j]);
      // end of dynamic graph portion
    } else { 
      // Static graph
      int edge = scc_graph->row_pointer[j];
      for (; edge<scc_graph->row_pointer[j+1]; edge++) {
        int i = scc_graph->column_index[edge];
	// there is an arc from i to j
	if (sccmap[j] != sccmap[i]) {
	  // arc to another class, node i cannot be in a terminal scc
          int c = sccmap[i] - g->NumNodes();
	  isterm[c] = 0;
#ifdef DEBUG_SCCS
          Output << "SCC " << c << " is not terminal, due to arc from ";
	  Output << i << " to " << j << "\n";
	  Output.flush();
#endif
        }
      }
      // end of static graph portion
    } // if dynamic graph
  } // for j
}

int 	ComputeTSCCs(digraph *g, unsigned long* sccmap)
{
#ifdef DEBUG_SCCS
  Output << "Computing terminal strongly connected components\n";
#endif
  scc_graph = g;
  visit_stack = new int[g->NumNodes()];
  visit_id = 0;
  scc_val = sccmap;
  scc_count = g->NumNodes();
  int i;
  for (i=0; i<g->NumNodes(); i++) 
    if (0==sccmap[i]) scc_visit(i);

  // sccs are numbered #states ... 2*#states-1 (at most)
  // figure out which sccs are terminal.  If there is an arc out of
  // one, then it is not terminal.
  // We'll store the results in the stack array.

  for (i=0; i<g->NumNodes(); i++) visit_stack[i] = 1;

  if (g->isTransposed) {
    BackArcsFindTerminal(g, sccmap, visit_stack);
  } else {
    FwdArcsFindTerminal(g, sccmap, visit_stack);
  }
  
  // renumber classes.  
  // first, compact the terminal sccs to have numbers 1,2,3,...
  int termcount = 0;
  for (i=0; i<g->NumNodes(); i++) if (visit_stack[i]) {
    termcount++;
    visit_stack[i] = termcount;
  }
  // next, renumber the scc mapping array.
  for (i=0; i<g->NumNodes(); i++) 
	sccmap[i] = visit_stack[sccmap[i]-g->NumNodes()];

  // done!
  delete[] visit_stack;
  return termcount;
}

