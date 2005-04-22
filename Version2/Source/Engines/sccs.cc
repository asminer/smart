
// $Id$

#include "sccs.h"

#include "../Templates/stack.h"

//#define DEBUG_SCCS


// globals
const digraph* scc_graph;
int* visit_stack;
int visit_stack_top;
int visit_id;
int scc_count;
int* scc_val;

Stack <int>* callstack;

inline void VisitsPush(int k) 
{
  DCASSERT(visit_stack_top<scc_graph->NumNodes());
  visit_stack[visit_stack_top++] = k;
  // number the state, too
  visit_id++;
  scc_val[k] = visit_id;
}

inline int VisitsPop()
{
  DCASSERT(visit_stack_top>0);
  return visit_stack[--visit_stack_top];
}

/* Stack-based (not recursive!) SCC algorithm.
   Yanked from Sedgewick's Algorithms book (from undergrad!)
   and skillfully rewritten to eliminate recursion.
   For use with dynamic graphs.
*/
void dynamic_visit(int i)
{
#ifdef DEBUG_SCCS
  Output << "Visiting node " << i << "\n";
  Output.flush();
#endif
  DCASSERT(scc_graph->IsDynamic());
  int edge = scc_graph->row_pointer[i];
  VisitsPush(i);
  int min = scc_val[i];
  while (1) {
    if (edge>=0) {
      // arc i-->j
      int j = scc_graph->column_index[edge];
      edge = scc_graph->next[edge];
      if (edge==scc_graph->row_pointer[i]) // we've cycled around
	edge = -1;  
      if (0==scc_val[j]) {
#ifdef DEBUG_SCCS
        Output << "Visiting node " << j << "\n";
        Output.flush();
#endif
 	// recurse, "by hand"
        callstack->Push(i);
	callstack->Push(edge);
        callstack->Push(min);
        VisitsPush(j);
	i = j;
	edge = scc_graph->row_pointer[j];
        min = scc_val[j];
      } else {
        min = MIN(min, scc_val[j]);
      }
      continue;
    } // if edge>=0
    DCASSERT(edge<0);
    // All states reachable from i have been visited

    // Determine if i was first state entered in its scc.
    // This is true iff its value is less than all neighbors.

#ifdef DEBUG_SCCS
    Output << "Done visiting node " << i << "; min=" << min << "\n"; 
    Output.flush();
#endif

    // Is this node the minimum? 
    if (min == scc_val[i]) {
      // yes, pop until we get i, those states are a SCC
#ifdef DEBUG_SCCS
      Output << "SCC #";
      Output << scc_count-scc_graph->NumNodes();
      Output << " contains:\t";
#endif
      int mbr;
      do {
        mbr = VisitsPop();
        scc_val[mbr] = scc_count;
#ifdef DEBUG_SCCS
	Output << " " << mbr;
#endif
      } while (mbr!=i);
      scc_count++;
#ifdef DEBUG_SCCS
      Output << "\n";
      Output.flush();
#endif
    } // if min

    if (callstack->Empty()) return;
    min = MIN(min, callstack->Pop());
    edge = callstack->Pop();
    i = callstack->Pop();
  } // while 1
}





/* Stack-based (not recursive!) SCC algorithm.
   Yanked from Sedgewick's Algorithms book (from undergrad!)
   and skillfully rewritten to eliminate recursion.
   For use with static graphs.
*/
void static_visit(int i)
{
#ifdef DEBUG_SCCS
  Output << "Visiting node " << i << "\n";
  Output.flush();
#endif
  DCASSERT(scc_graph->IsStatic());
  int edge = scc_graph->row_pointer[i];
  VisitsPush(i);
  int min = scc_val[i];
  while (1) {
    if (edge<scc_graph->row_pointer[i+1]) {
      // arc i-->j
      int j = scc_graph->column_index[edge];
      edge++;
      if (0==scc_val[j]) {
#ifdef DEBUG_SCCS
        Output << "Visiting node " << j << "\n";
        Output.flush();
#endif
 	// recurse, "by hand"
        callstack->Push(i);
	callstack->Push(edge);
        callstack->Push(min);
        VisitsPush(j);
	i = j;
	edge = scc_graph->row_pointer[j];
        min = scc_val[j];
      } else {
        min = MIN(min, scc_val[j]);
      }
      continue;
    } // if edge<
    DCASSERT(edge>=scc_graph->row_pointer[i+1]);
    // All states reachable from i have been visited

    // Determine if i was first state entered in its scc.
    // This is true iff its value is less than all neighbors.

#ifdef DEBUG_SCCS
    Output << "Done visiting node " << i << "; min=" << min << "\n"; 
    Output.flush();
#endif

    // Is this node the minimum? 
    if (min == scc_val[i]) {
      // yes, pop until we get i, those states are a SCC
#ifdef DEBUG_SCCS
      Output << "SCC #";
      Output << scc_count-scc_graph->NumNodes();
      Output << " contains:\t";
#endif
      int mbr;
      do {
        mbr = VisitsPop();
        scc_val[mbr] = scc_count;
#ifdef DEBUG_SCCS
	Output << " " << mbr;
#endif
      } while (mbr!=i);
      scc_count++;
#ifdef DEBUG_SCCS
      Output << "\n";
      Output.flush();
#endif
    } // if min

    if (callstack->Empty()) return;
    min = MIN(min, callstack->Pop());
    edge = callstack->Pop();
    i = callstack->Pop();
  } // while 1
}





/* Based on Tarjan's SCC algorithm (apparently).
   Yanked from Sedgewick's Algorithms book (from undergrad!)
   Recursive version
*/
/*
int dynamic_scc_visit(int k)
{
  DCASSERT(scc_graph->IsDynamic());
#ifdef DEBUG_SCCS
  Output << "Visiting node " << k << "\n";
  Output.flush();
#endif
  int min;
  visit_id++;
  scc_val[k] = visit_id;
  min = visit_id;
  // Push k
  DCASSERT(visit_stack_top<scc_graph->NumNodes());
  visit_stack[visit_stack_top++] = k;
   
  // visit "outgoing" edges from node k
  // Dynamic graph, traverse circular list
  int edge = scc_graph->row_pointer[k];
  if (edge>=0) do {
    int j = scc_graph->column_index[edge];
    if (j!=k) {
        int m = (scc_val[j]) ? scc_val[j] : dynamic_scc_visit(j);
        min = MIN(min, m);
#ifdef DEBUG_SCCS
        Output << "\tnode " << k << " --> node " << j << " (m=" << m << "\n"; 
#endif
    }
    edge = scc_graph->next[edge];
  } while (edge!=scc_graph->row_pointer[k]);

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
*/

/* Based on Tarjan's SCC algorithm (apparently).
   Yanked from Sedgewick's Algorithms book (from undergrad!)
   Recursive version.
*/
/*
int static_scc_visit(int k)
{
  DCASSERT(scc_graph->IsStatic());
#ifdef DEBUG_SCCS
  Output << "Visiting node " << k << "\n";
  Output.flush();
#endif
  int min;
  visit_id++;
  scc_val[k] = visit_id;
  min = visit_id;
  // Push k
  DCASSERT(visit_stack_top<scc_graph->NumNodes());
  visit_stack[visit_stack_top++] = k;
   
  // visit "outgoing" edges from node k
  // Static graph
  int edge = scc_graph->row_pointer[k];
  for (; edge<scc_graph->row_pointer[k+1]; edge++) {
    int j = scc_graph->column_index[edge];
    if (j!=k) {
      int m = (scc_val[j]) ? scc_val[j] : static_scc_visit(j);
      min = MIN(min, m);
#ifdef DEBUG_SCCS
      Output << "\tnode " << k << " --> node " << j << " (m=" << m << "\n"; 
#endif
    }
  }

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
*/


int 	ComputeSCCs(const digraph *g, int* sccmap)
{
#ifdef DEBUG_SCCS
  Output << "Computing strongly connected components\n";
#endif
  scc_graph = g;
  visit_stack = new int[g->NumNodes()];
  visit_id = 0;
  scc_val = sccmap;
  scc_count = g->NumNodes();
  callstack = new Stack<int> (1024, 2*g->NumNodes());
  int i;
  if (g->IsDynamic()) {
    for (i=0; i<g->NumNodes(); i++) 
      if (0==sccmap[i]) dynamic_visit(i);
  } else {
    for (i=0; i<g->NumNodes(); i++) 
      if (0==sccmap[i]) static_visit(i);
  }
  delete callstack;
  delete[] visit_stack;

  for (i=0; i<g->NumNodes(); i++) sccmap[i] -= g->NumNodes();
  scc_count -= g->NumNodes();
  return scc_count;
}




void FwdArcsFindTerminal(const digraph *g, int* sccmap, int* isterm)
{
  DCASSERT(g->isTransposed == false);
  
  int largest = 2*g->NumNodes();
  // For each state that we care about
  for (int i=0; i<g->NumNodes(); i++) {
    if (sccmap[i] > largest) continue;
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
	    // no sense checking remaining arcs
            break;
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
	    // no sense checking remaining arcs
            break;
          }
        }
        // end of static graph portion
      } // if dynamic graph
    } // if isterm[c]
  } // for i
}

void BackArcsFindTerminal(const digraph *g, int* sccmap, int* isterm)
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

int 	ComputeTSCCs(const digraph *g, int* sccmap)
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
  callstack = new Stack<int> (1024, 3*g->NumNodes());
  if (g->IsDynamic()) {
    for (i=0; i<g->NumNodes(); i++) 
      if (0==sccmap[i]) dynamic_visit(i);
  } else {
    for (i=0; i<g->NumNodes(); i++) 
       if (0==sccmap[i]) static_visit(i);
  }
  delete callstack;
  scc_count -= g->NumNodes();

  // sccs are numbered #states ... 2*#states-1 (at most)
  // figure out which sccs are terminal.  If there is an arc out of
  // one, then it is not terminal.
  // We'll store the results in the stack array.

  for (i=0; i<int(scc_count); i++) visit_stack[i] = 1;

  if (g->isTransposed) {
    BackArcsFindTerminal(g, sccmap, visit_stack);
  } else {
    FwdArcsFindTerminal(g, sccmap, visit_stack);
  }
  
  // renumber classes.  
  // first, compact the terminal sccs to have numbers 1,2,3,...
  int termcount = 0;
  for (i=0; i<int(scc_count); i++) if (visit_stack[i]) {
    termcount++;
    visit_stack[i] = termcount;
  }
  // next, renumber the scc mapping array.
  int largest = 2*g->NumNodes();
  for (i=0; i<g->NumNodes(); i++) {
    if (sccmap[i] > largest) continue;
    sccmap[i] = visit_stack[sccmap[i]-g->NumNodes()];
  }

  // done!
  delete[] visit_stack;
#ifdef DEBUG_SCCS
  Output << "Done, there are " << termcount << " terminal sccs\n";
#endif
  return termcount;
}




void FindLoops(const digraph *g, int* sccmap, int* hasloop)
{
  // For each state
  for (int i=0; i<g->NumNodes(); i++) {
    // Get the class for this state
    int c = sccmap[i] - g->NumNodes();
    // Already known to have a loop?
    if (hasloop[c]) continue; 
    // check all outgoing arcs from this state
    if (g->IsDynamic()) {
        // Dynamic graph, traverse circular list
        int edge = g->row_pointer[i];
        if (edge>=0) do {
          int j = g->column_index[edge];
	  // there is an arc from i to j
	  if (sccmap[j] == sccmap[i]) {
	    // loop within this class
	    hasloop[c] = 1;
            break; // no sense checking the rest of the arcs
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
	  if (sccmap[j] == sccmap[i]) {
	    // loop within this class
	    hasloop[c] = 1;
            break; // no sense checking the rest of the arcs
          }
        }
        // end of static graph portion
    } // if dynamic graph
  } // for i
}

int ComputeLoopedSCCs(const digraph *g, int* sccmap)
{
#ifdef DEBUG_SCCS
  Output << "Computing looped strongly connected components\n";
#endif
  scc_graph = g;
  visit_stack = new int[g->NumNodes()];
  visit_id = 0;
  scc_val = sccmap;
  scc_count = g->NumNodes();
  int i;
  callstack = new Stack<int> (1024, 2*g->NumNodes());
  if (g->IsDynamic()) {
    for (i=0; i<g->NumNodes(); i++) 
      if (0==sccmap[i]) dynamic_visit(i);
  } else {
    for (i=0; i<g->NumNodes(); i++) 
      if (0==sccmap[i]) static_visit(i);
  }
  delete callstack;

  // sccs are numbered #states ... 2*#states-1 (at most)
  // figure out which sccs have arcs to their own scc.
  // We'll store the results in the stack array: 1 means this 
  // class has an arc to itself.

  for (i=0; i<g->NumNodes(); i++) visit_stack[i] = 0;

  FindLoops(g, sccmap, visit_stack);
  
  // renumber classes.  
  // first, compact the looped sccs to have numbers 1,2,3,...
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


