
#ifndef RGR_GRLIB_H
#define RGR_GRLIB_H

#include "rgr_ectl.h"
#include "rss_indx.h"

// external libraries
#include "../_GraphLib/graphlib.h"
#include "../_IntSets/intset.h"


class grlib_reachgraph : public ectl_reachgraph {

  public:
    grlib_reachgraph(GraphLib::dynamic_digraph* g);

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
    virtual void count_edges(bool rt, CTL_traversal &CTL) const;
    virtual void traverse(bool rt, GraphLib::BF_graph_traversal &T) const; 

  private:
    static void showRawMatrix(OutputStream &os, const GraphLib::static_graph &E);

  private:
    GraphLib::dynamic_digraph* edges;  // hold until attachToParent
    LS_Vector initial;    // hold until we can pass it to RSS
    intset deadlocks;     // set of states with no outgoing edges

    // Graph after finishing, stored by incoming edges
    GraphLib::static_graph InEdges; 

    // Graph after finishing, stored by outgoing edges, for reverse time
    GraphLib::static_graph OutEdges;

};

#endif

