
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




#endif
