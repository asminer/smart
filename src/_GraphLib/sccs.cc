
#include "sccs.h"
#include "graphlib.h"

#include <stdlib.h>

// #define DEBUG_SCCS

#ifdef DEBUG_SCCS
#include <stdio.h>
#endif

// ==================================================================
// |                                                                |
// |                           Front  end                           |
// |                                                                |
// ==================================================================

GraphLib::abstract_classifier*
GraphLib::dynamic_graph::determineSCCs(long nonterminal, long sinks, 
  bool cons, timer_hook* sw) const
{
  //
  // Sanity checks
  //

  if (getNumNodes() < 1) return 0;

  //
  // Build scc map and auxiliary memory
  //
  long* sccmap;
  long* aux;

  sccmap = new long[getNumNodes()];
  aux = new long[getNumNodes()];

  for (long i=0; i<getNumNodes(); i++) {
    sccmap[i] = 0;
  }

  //
  // First and hardest step - determine "raw" SCCs
  //

  if (sw) sw->start("Finding SCCs");

  scc_traversal foo(*this, sccmap, aux);

  if (cons) {
    for (long i=0; i<getNumNodes(); i++) 
      if (0==sccmap[i]) foo.Visit1(i);
  } else {
    for (long i=0; i<getNumNodes(); i++) 
      if (0==sccmap[i]) foo.Visit2(i);
  }
  const long scc_count = foo.NumSCCs();

  if (sw) sw->stop();


  //
  // Handy constants to make code readable :^)
  //

  const long TSCC = -1;

  //
  // Now, use aux array to renumber and to
  // further classify the SCCs.
  // Initially we'll set everything to TSCC
  // until we learn otherwise.

  for (long i=0; i<scc_count; i++) aux[i] = TSCC;


  //
  // If we're merging nonterminals, then we need to determine
  // which SCCs are terminal and which are not.
  //

  if (nonterminal>=0 && scc_count>1) {
    if (sw) sw->start("Finding terminal SCCs");

    //
    // Figure out which SCCs can reach other SCCs.
    // Mark those as "nonterminal" in array aux.
    //
    for (long i=0; i<getNumNodes(); i++) {
      //
      // Get the class for state i
      //
      const long c = sccmap[i] - getNumNodes();
      DCASSERT(c>=0);
      DCASSERT(c<getNumNodes());

      // If we already know this class is non-terminal,
      // then we can skip this state
      if (nonterminal == aux[c]) continue;

      //
      // Loop over all outgoing edges from this state,
      // and check if we reach a state in a different SCC.
      // If so, mark this SCC as non-terminal.
      //
      long edge = row_pointer[i];
      if (edge<0) continue;   // no outgoing edges
      do {
        const long j = column_index[edge];
        if (sccmap[j] != sccmap[i]) {
          aux[c] = nonterminal;
          break;  // no point continuing in the loop
        }
        edge = next[edge];
      } while (edge != row_pointer[i]);
    } // for i

    if (sw) sw->stop();
  }

  //
  // If we're merging sink/absorbing states, then we need to
  // determine which states cannot reach other states.
  //

  if (sinks>=0) {
    if (sw) sw->start("Finding sink nodes");

    for (long i=0; i<getNumNodes(); i++) {
      //
      // Check outgoing edges for state i,
      // if we have an edge not to stae i
      // then this is not an absorbing state.
      //
      bool absorbing = true;

      long edge = row_pointer[i];
      if (edge>=0) do {
        if (column_index[edge] != i) {
          absorbing = false;
          break;
        }
        edge = next[edge];
      } while (edge != row_pointer[i]);

      //
      // If we're absorbing, mark the SCC as such
      //
      if (absorbing) {
        const long c = sccmap[i] - getNumNodes();
        DCASSERT(c>=0);
        DCASSERT(c<getNumNodes());
        aux[c] = sinks;
      }
    } // for i

    if (sw) sw->stop();
  }


  //
  // Compact the numbering of the SCCs.
  // We'll use array aux to remember
  // the new number for each class.
  //

  if (sw) sw->start("Renumbering SCCs");

  //
  // Initialize SCC number.
  // The three lines ensure that termcount skips
  // over the special values "nonterminal" and "sink".
  // It takes three lines because we don't know if
  // sinks < nonterminal or nonterminal < sinks.
  //
  long termcount = 0;
  if (nonterminal == termcount) termcount++;
  if (sinks == termcount) termcount++;
  if (nonterminal == termcount) termcount++;
  
  for (long i=0; i<getNumNodes(); i++) {
    const long c = sccmap[i] - getNumNodes();
    
    if (aux[c] != TSCC) {
      // i's class is already renumbered
      sccmap[i] = aux[c];
      continue;
    }

    //
    // Still here?  Need to renumber this SCC.
    //

    aux[c] = termcount;
    sccmap[i] = aux[c];

    //
    // Increment SCC number, skipping over special values
    //
    termcount++;
    if (nonterminal == termcount) termcount++;
    if (sinks == termcount) termcount++;
    if (nonterminal == termcount) termcount++;
  } // for state i

  //
  // Determine number of classes, including special ones
  // so that all classes are within the range 
  //      0, 1, 2, ..., numclasses-1
  //
  long num_classes = MAX(termcount, nonterminal+1);
  num_classes = MAX(num_classes, sinks+1);

  if (sw) sw->stop();

  //
  // Clean up and package up results
  //
  delete[] aux;
  return new general_classifier(sccmap, getNumNodes(), num_classes);
}


// ======================================================================
// |                                                                    |
// |                dynamic_graph::scc_traversal methods                |
// |                                                                    |
// ======================================================================

GraphLib::dynamic_graph::scc_traversal
  ::scc_traversal(const dynamic_graph &graph, long* sccs, long* vstack)
  : G(graph)
{
  if (!G.isByRows()) throw error(error::Format_Mismatch);

  scc_val = sccs;
  scc_count = G.getNumNodes();

  visit_stack = vstack;
  visit_stack_top = 0;
  visit_id = 0;

  call_stack = 0;
  call_stack_top = 0;
  call_stack_size = 0;
}

// ******************************************************************

GraphLib::dynamic_graph::scc_traversal::~scc_traversal()
{
  free(call_stack);

  // DON'T delete scc_val or visit_stack
}

// ******************************************************************

void GraphLib::dynamic_graph::scc_traversal::Visit3(long i)
{
#ifdef DEBUG_SCCS
  fprintf(stderr, "Visiting node %d\n", i);
#endif

  long edge = G.row_pointer[i];
  VisitPush(i);
  long min = scc_val[i];
  while (1) {
    if (edge>=0) {
      // arc i-->j
      long j = G.column_index[edge];
      edge = G.next[edge];
      if (edge==G.row_pointer[i]) // we've cycled around
        edge = -1;  
      if (0==scc_val[j]) {
#ifdef DEBUG_SCCS
        fprintf(stderr, "Visiting node %d\n", j);
#endif
        // "by hand" recursion
        CallPush(i, edge, min);
        VisitPush(j);
        i = j;
        edge = G.row_pointer[j];
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
    fprintf(stderr, "Done visiting node %d; min=%d\n", i, min);
#endif

    // Is this node the minimum? 
    if (min == scc_val[i]) {
      // yes, pop until we get i, those states are a SCC
#ifdef DEBUG_SCCS
      fprintf(stderr, "SCC #%d contains:\t", scc_count-g->getNumNodes());
#endif
      long mbr;
      do {
        mbr = VisitPop();
        scc_val[mbr] = scc_count;
#ifdef DEBUG_SCCS
        fprintf(stderr, " %d", mbr);
#endif
      } while (mbr!=i);
      scc_count++;
#ifdef DEBUG_SCCS
      fprintf(stderr, "\n");
#endif
    } // if min

    long m;
    if (!CallPop(i, edge, m)) return;  // empty stack = done!
    min = MIN(min, m);
  } // while 1
}


// ******************************************************************

void GraphLib::dynamic_graph::scc_traversal::Visit2(long i)
{
#ifdef DEBUG_SCCS
  fprintf(stderr, "Visiting node %d\n", i);
#endif

  long edge = G.row_pointer[i];
  VisitPush(i);
  while (1) {
    if (edge>=0) {
      // arc i-->j
      long j = G.column_index[edge];
      edge = G.next[edge];
      if (edge==G.row_pointer[i]) // we've cycled around
        edge = -1;  
      if (0==scc_val[j]) {
#ifdef DEBUG_SCCS
        fprintf(stderr, "Visiting node %d\n", j);
#endif
        // "by hand" recursion
        CallPush(i, edge);
        VisitPush(j);
        i = j;
        edge = G.row_pointer[j];
      } 
      continue;
    } // if edge>=0
    DCASSERT(edge<0);
    // All states reachable from i have been visited
    // Now, compute the minimum value.
    long min = scc_val[i];
    edge = G.row_pointer[i];
    while (edge >= 0) {
      long j = G.column_index[edge];
      min = MIN(min, scc_val[j]);
      edge = G.next[edge];
      if (edge == G.row_pointer[i]) break;
    } 
    
    // Determine if i was first state entered in its scc.
    // This is true iff its value is less than all neighbors.

#ifdef DEBUG_SCCS
    fprintf(stderr, "Done visiting node %d; min=%d\n", i, min);
#endif

    // Is this node the minimum? 
    if (min == scc_val[i]) {
      // yes, pop until we get i, those states are a SCC
#ifdef DEBUG_SCCS
      fprintf(stderr, "SCC #%d contains:\t", scc_count-g->getNumNodes());
#endif
      long mbr;
      do {
        mbr = VisitPop();
        scc_val[mbr] = scc_count;
#ifdef DEBUG_SCCS
        fprintf(stderr, " %d", mbr);
#endif
      } while (mbr!=i);
      scc_count++;
#ifdef DEBUG_SCCS
      fprintf(stderr, "\n");
#endif
    } else {
      // not minimum, save it for later
      scc_val[i] = min;
    }

    if (!CallPop(i, edge)) return;  // empty stack = done!
  } // while 1
}


// ******************************************************************

void GraphLib::dynamic_graph::scc_traversal::Visit1(long i)
{
#ifdef DEBUG_SCCS
  fprintf(stderr, "Visiting node %d\n", i);
#endif

  VisitPush(i);
  while (1) {
    // Try to get through all our children
    // optimistically compute minimum as we go
    long min = scc_val[i];
    long edge = G.row_pointer[i];
    while (edge >= 0) {
      long j = G.column_index[edge];
      min = MIN(min, scc_val[j]);
      if (0==min) {
        // we've never visited j, "by hand" recursion
        CallPush(i);  // out of memory
#ifdef DEBUG_SCCS
        fprintf(stderr, "Visiting node %d\n", j);
#endif
        VisitPush(j);
        i = j;
        break;
      }
      edge = G.next[edge];
      if (edge == G.row_pointer[i]) break;
    } // while edge >= 0
    if (0==min) continue;  // start on one of our children
  
    // All states reachable from i have been visited
    // and the minimum value we've computed is valid.
    
    // Determine if i was first state entered in its scc.
    // This is true iff its value is less than all neighbors.

#ifdef DEBUG_SCCS
    fprintf(stderr, "Done visiting node %d; min=%d\n", i, min);
#endif

    // Is this node the minimum? 
    if (min == scc_val[i]) {
      // yes, pop until we get i, those states are a SCC
#ifdef DEBUG_SCCS
      fprintf(stderr, "SCC #%d contains:\t", scc_count-g->getNumNodes());
#endif
      long mbr;
      do {
        mbr = VisitPop();
        scc_val[mbr] = scc_count;
#ifdef DEBUG_SCCS
        fprintf(stderr, " %d", mbr);
#endif
      } while (mbr!=i);
      scc_count++;
#ifdef DEBUG_SCCS
      fprintf(stderr, "\n");
#endif
    } else {
      // not minimum, save it for later
      scc_val[i] = min;
    }

    if (!CallPop(i)) return;  // empty stack = done!
  } // while 1
}

// ******************************************************************

void GraphLib::dynamic_graph::scc_traversal::EnlargeCallStack(long newsize)
{
  DCASSERT(newsize > call_stack_size);
  long* foo = (long*) realloc(call_stack, newsize*sizeof(long));
  if (0==foo) throw error(error::Out_Of_Memory);
  call_stack = foo;
  call_stack_size = newsize;
}



