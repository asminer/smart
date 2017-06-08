
#ifndef SCCS_H
#define SCCS_H

#include "graphlib.h"

/*
  Functions for strongly-connected components
  and variations.
*/


// ======================================================================
// |                                                                    |
// |                 dynamic_graph::scc_traversal class                 |
// |                                                                    |
// ======================================================================

namespace GraphLib {

  class dynamic_graph::scc_traversal {
    public:
      scc_traversal(const dynamic_graph &graph, long* sccs, long* vstack);
      ~scc_traversal();


    public:
      /**
        Stack-based (not recursive) SCC algorithm.
        Yanked from Sedgewick's Algorithms book (from undergrad)
        and skillfully rewritten to eliminate recursion.

        This version pushes state, edge, and minimum to the call stack.
      */
      void Visit3(long k);

      /** 
        Stack-based (not recursive) SCC algorithm.
        Yanked from Sedgewick's Algorithms book (from undergrad)
        and skillfully rewritten to eliminate recursion.

        Modified slightly: we can compute the minimum after visiting
        all of our "children".  As such, this version pushes state
        and edge (only) to the call stack.
      */
      void Visit2(long k);

      /** 
        Stack-based (not recursive) SCC algorithm.
        Yanked from Sedgewick's Algorithms book (from undergrad)
        and skillfully rewritten to eliminate recursion.

        Modified significantly, added time in favor of memory reduction: 
        We don't remember which children we've visited.   On "return" we have 
        to re-visit all children, but this is fast for children who are already 
        visited.  This version pushes only the state to the call stack.
      */
      void Visit1(long k);

      inline long NumSCCs() const { return scc_count - G.getNumNodes(); }

      inline long CallAlloc() const { return call_stack_size; }

    private:
      void EnlargeCallStack(long newsize);
      inline void VisitPush(long k) {
        DCASSERT(visit_stack_top < G.getNumNodes());
        visit_stack[visit_stack_top++] = k;
        visit_id++;
        scc_val[k] = visit_id;
      }
      inline long VisitPop() {
        DCASSERT(visit_stack_top>0);
        return visit_stack[--visit_stack_top];
      }

      // For 3 vars on call stack

      inline void CallPush(long state, long edge, long min) {
        if (call_stack_top+3>call_stack_size) {
          EnlargeCallStack(call_stack_size+1024);
        }
        call_stack[call_stack_top++] = state;
        call_stack[call_stack_top++] = edge;
        call_stack[call_stack_top++] = min;
      }
      inline bool CallPop(long& state, long& edge, long& min) {
        if (0==call_stack_top) return false;
        DCASSERT(call_stack_top>2);
        min = call_stack[--call_stack_top];
        edge = call_stack[--call_stack_top];
        state = call_stack[--call_stack_top];
        return true;
      }

      // For 2 vars on call stack

      inline void CallPush(long state, long edge) {
        if (call_stack_top+2>call_stack_size) {
          EnlargeCallStack(call_stack_size+1024);
        }
        call_stack[call_stack_top++] = state;
        call_stack[call_stack_top++] = edge;
      }
      inline bool CallPop(long& state, long& edge) {
        if (0==call_stack_top) return false;
        DCASSERT(call_stack_top>1);
        edge = call_stack[--call_stack_top];
        state = call_stack[--call_stack_top];
        return true;
      }

      // For 1 var on call stack!

      inline void CallPush(long state) {
        if (call_stack_top+1>call_stack_size) {
          EnlargeCallStack(call_stack_size+1024);
        }
        call_stack[call_stack_top++] = state;
      }
      inline bool CallPop(long& state) {
        if (0==call_stack_top) return false;
        state = call_stack[--call_stack_top];
        return true;
      }

    private:
      const dynamic_graph &G;
      long* scc_val;
      long* visit_stack;
      long* call_stack;
      long scc_count;
      long visit_stack_top;
      long visit_id;
      long call_stack_top;
      long call_stack_size;
  };

};


/**
    Determine all SCCs in a graph.

      @param  g       The graph 

      @param  cons    If true, conserve memory, at a cost of
                      (usually only slightly) increased CPU time.

      @param  sccmap  An array of dimension at least g.numNodes().
                      Completely ignored on input.  On output,
                      sccmap[i] is an integer between g.numNodes()
                      and 2*g.numNodes(), such that 
                      sccmap[i] equals sccmap[j] if and only if
                      nodes i and j belong to the same SCC.

      @param  aux     Auxiliary array of dimension at least g.numNodes().
                      Ignored on input, undefined elements on output, used
                      during the computation.
*/
// long find_sccs(const GraphLib::dynamic_graph &g, bool cons, long* sccmap, long* aux);

/**
    Given the SCCs of a graph, decide which ones are terminal SCCs.
    An SCC is "terminal" if it cannot be escaped, i.e., all outgoing
    edges are to states in the same SCC.

      @param  g           The graph 

      @param  sm          An array of dimension at least g.numNodes(),
                          where sm[i] gives an integer between g.numNodes()
                          and 2*g.numNodes(), which is the SCC number for 
                          node i.

      @param  aux         An array of dimension at least scc_count.
                          On output, aux[c-g.numNodes()] is zero
                          if SCC c (numbered between g.numNodes() and 
                          2*g.numNodes()) can reach another SCC,
                          and negative otherwise.

      @param  scc_count   Number of SCCs.
*/
// void find_tsccs(const GraphLib::dynamic_graph &g, const long* sm, long* aux, long scc_count);


/**
    Renumber SCCs.

      @param  g           The graph 

      @param  sm          An array of dimension at least g.numNodes().
                          On input, sm[i] gives an integer between 
                          g.numNodes() and 2*g.numNodes(), which is the 
                          SCC number for node i.
                          On output, sm[i] gives an integer between
                          0 and scc_count, where
                            0 is for any class with aux[c] = 0,
                            1 is for all sink states, and
                            2..scc_count is for all other TSCCs.

      @param  aux         An array of dimension at least scc_count.
                          Used as a renumbering map for SCCs.
                          On input, aux[c] should be 0 for any SCCs
                          that should be grouped together as SCC 0,
                          and negative otherwise.

      @param  scc_count   (Upper bound on) mumber of SCCs.
      
*/
// long compact(const GraphLib::dynamic_graph &g, long* sm, long* aux, long scc_count);

#ifdef ALLOW_OLD_GRAPH_INTERFACE

long find_sccs(const GraphLib::generic_graph* g, bool cons, long* sccmap, long* aux);
void find_tsccs(const GraphLib::generic_graph* g, const long* sm, long* aux, long scc_count);
long compact(const GraphLib::generic_graph* g, long* sm, long* aux, long scc_count);

#endif

#endif
