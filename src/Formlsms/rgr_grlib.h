
#ifndef RGR_GRLIB_H
#define RGR_GRLIB_H

#include "rgr_ectl.h"
#include "rss_indx.h"

// external libraries
#include "../_GraphLib/graphlib.h"
#include "../_IntSets/intset.h"

// #define USE_OLD_GRAPH_INTERFACE

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
#ifdef USE_OLD_GRAPH_INTERFACE
    grlib_reachgraph(GraphLib::digraph* g);
#else
    grlib_reachgraph(GraphLib::dynamic_digraph* g);
#endif

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
#ifdef USE_OLD_GRAPH_INTERFACE
    static void showRawMatrix(OutputStream &os, const GraphLib::digraph::matrix &E);
    static void _traverse(bool one_step, const GraphLib::digraph::matrix &E, traverse_helper &TH);
#else
    static void showRawMatrix(OutputStream &os, const GraphLib::static_graph &E);
    static void _traverse(bool one_step, const GraphLib::static_graph &E, traverse_helper &TH);
#endif


  private:
#ifdef USE_OLD_GRAPH_INTERFACE
    GraphLib::digraph* edges; // hold until attachToParent
#else
    GraphLib::dynamic_digraph* edges;  // hold until attachToParent
#endif
    LS_Vector initial;    // hold until we can pass it to RSS
    intset deadlocks;     // set of states with no outgoing edges

#ifdef USE_OLD_GRAPH_INTERFACE
    // matrix after finishing, stored by incoming edges
    GraphLib::digraph::matrix InEdges; 

    // matrix after finishing, stored by outgoing edges, for reverse time
    GraphLib::digraph::matrix OutEdges;
#else
    // Graph after finishing, stored by incoming edges
    GraphLib::static_graph InEdges; 

    // Graph after finishing, stored by outgoing edges, for reverse time
    GraphLib::static_graph OutEdges;
#endif

// New traversal stuff
#ifndef USE_OLD_GRAPH_INTERFACE
    template <bool ONESTEP>
    class mygraphtraverse : public GraphLib::BF_graph_traversal {
      public:
        mygraphtraverse(traverse_helper &_th);

        virtual bool hasStatesToExplore();
        virtual long getNextToExplore();
        virtual bool visit(long, long dest, void*);
      private:
        traverse_helper &TH;
    };
    /*
    class onestep_bfs : public GraphLib::BF_graph_traversal {
      public:
        onestep_bfs(traverse_helper &_th);

        virtual bool hasStatesToExplore();
        virtual long getNextToExplore();
        virtual bool visit(long, long dest, void*);
      private:
        traverse_helper &TH;
    };
    class allstep_bfs : public GraphLib::BF_graph_traversal {
      public:
        allstep_bfs(traverse_helper &_th);

        virtual bool hasStatesToExplore();
        virtual long getNextToExplore();
        virtual bool visit(long, long dest, void*);
      private:
        traverse_helper &TH;
    };
    */
#endif

};

#endif

