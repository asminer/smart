
// $Id$

#ifndef RGR_GRLIB_H
#define RGR_GRLIB_H

#include "rgr_ectl.h"
#include "rss_indx.h"

// external libraries
#include "graphlib.h"
#include "intset.h"

/*
  TO DO:
    (1) When we attach to parent, make static "by rows" 
        and "by columns" matrices, and destroy edges.

    (2) Adjust destructor

    (3) Re-implement showArcs

    (4) Re-implement countPaths
*/

class grlib_reachgraph : public ectl_reachgraph {

  public:
    grlib_reachgraph(GraphLib::digraph* g);

  protected:
    virtual ~grlib_reachgraph();
    virtual const char* getClassName() const { return "grlib_reachgraph"; }
    virtual void attachToParent(graph_lldsm* p, state_lldsm::reachset*);

  public:
    virtual void getNumArcs(long &na) const;
    virtual void showInternal(OutputStream &os) const;
    virtual void showArcs(OutputStream &os, const show_options& opt, 
      state_lldsm::reachset* RSS, shared_state* st) const;

    // Hold initial until we can give it to RSS.
    void setInitial(LS_Vector &init);

    // This method is needed for the fsm formalism
    inline bool isDeadlocked(long st) const {
      return deadlocks.contains(st);
    }

    virtual void countPaths(const stateset* src, const stateset* dest, result& count);

  protected:
    virtual void getDeadlocked(intset &r) const;
    virtual void need_reverse_time();

    virtual void count_edges(bool rt, traverse_helper &TH) const;
      
    virtual void traverse(bool rt, bool one_step, traverse_helper &TH) const; 

  private:
    template <class EDGES>
    inline void _traverse(bool one_step, const EDGES &E, traverse_helper &TH) const {
        while (TH.queue_nonempty()) {
            long s = TH.queue_pop();
            // explore edges to/from s
            for (long z=E.rowptr[s]; z<E.rowptr[s+1]; z++) {
                long t = E.colindex[z];
                if (TH.num_obligations(t) <= 0) continue;
                TH.remove_obligation(t);
                if (one_step) continue;
                if (0==TH.num_obligations(t)) {
                    TH.queue_push(t);
                }
            } // for z
        } // while queue not empty
    }

  
// OLD FROM HERE

    /*

    virtual stateset* unfairAEF(bool revTime, const stateset* p, const stateset* q);

  protected:
    virtual bool forward(const intset& p, intset &r) const;
    virtual bool backward(const intset& p, intset &r) const;

    */
  private:
    bool transposeEdges(const named_msg* rep, bool byrows);
    
  private:
    GraphLib::digraph* edges;
    LS_Vector initial;    // hold until we can pass it to RSS
    intset deadlocks;     // set of states with no outgoing edges

    // matrix after finishing, stored by incoming edges
    GraphLib::digraph::const_matrix InEdges; 

    // matrix after finishing, stored by outgoing edges, for reverse time
    GraphLib::digraph::matrix OutEdges;

  // ----------------------------------------------------------------------
  private:
    // for displaying edges
    class sparse_row_elems : public GraphLib::generic_graph::element_visitor {
      int alloc;
      const indexed_reachset::indexed_iterator &I;
      bool incoming;
      bool overflow;
    public:
      int last;
      long* index;
    public:
      sparse_row_elems(const indexed_reachset::indexed_iterator &i);
      virtual ~sparse_row_elems();

    protected:
      bool Enlarge(int ns);
    public:
      bool buildIncomingUnsorted(GraphLib::digraph* g, int i);
      bool buildIncoming(GraphLib::digraph* g, int i);
      bool buildOutgoing(GraphLib::digraph* g, int i);

    // for element_visitor
      virtual bool visit(long from, long to, void*);    

    // for heapsort
      inline int Compare(long i, long j) const {
        CHECK_RANGE(0, i, last);
        CHECK_RANGE(0, j, last);
        return SIGN(index[i] - index[j]);
      }

      inline void Swap(long i, long j) {
        CHECK_RANGE(0, i, last);
        CHECK_RANGE(0, j, last);
        SWAP(index[i], index[j]);
      }
    };
  // ----------------------------------------------------------------------
  /*
  private:
    class outgoingCounter : public GraphLib::generic_graph::element_visitor {
        long* count;
      public:
        outgoingCounter(long* c);
        virtual ~outgoingCounter();
        virtual bool visit(long from, long to, void*);
    };
    */
  // ----------------------------------------------------------------------
  /*
  private:
    class incomingEdges : public GraphLib::generic_graph::element_visitor {
        int alloc;
        int last;
        long* index;
        bool overflow;
      public:
        incomingEdges();
        virtual ~incomingEdges();
        virtual bool visit(long from, long to, void*);

        inline int Length() const { 
          return last; 
        }
        inline long Item(int z) const {
          CHECK_RANGE(0, z, last);
          return index[z];
        }
        inline bool overflowed() const {
          return overflow;
        }
        inline void Clear() {
          last = 0;
          overflow = false;
        }

      protected:
        bool Enlarge(int ns);
    };
*/
};

#endif

