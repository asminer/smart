
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
    static void showRawMatrix(OutputStream &os, const GraphLib::digraph::matrix &E);
    static void _traverse(bool one_step, const GraphLib::digraph::matrix &E, traverse_helper &TH);


  private:
    // Kill this!  TBD
    bool transposeEdges(const named_msg* rep, bool byrows);
    
  private:
    GraphLib::digraph* edges; // hold until attachToParent
    LS_Vector initial;    // hold until we can pass it to RSS
    intset deadlocks;     // set of states with no outgoing edges

    // matrix after finishing, stored by incoming edges
    GraphLib::digraph::matrix InEdges; 

    // matrix after finishing, stored by outgoing edges, for reverse time
    GraphLib::digraph::matrix OutEdges;

};

#endif

