
#ifndef PROC_MCLIB_H
#define PROC_MCLIB_H

#include "proc_markov.h"
#include "rss_indx.h"
#include "rgr_ectl.h"

#include "../_MCLib/mclib.h"
#include "../_IntSets/intset.h"

// ******************************************************************
// *                                                                *
// *                      mclib_process  class                      *
// *                                                                *
// ******************************************************************

class mclib_process : public markov_process {

  public:
    mclib_process(bool discrete, GraphLib::dynamic_graph *G);
    mclib_process(MCLib::vanishing_chain* vc);
    

  protected:
    virtual ~mclib_process();
    virtual const char* getClassName() const { return "mclib_process"; }
    virtual void attachToParent(stochastic_lldsm* p, LS_Vector &init, state_lldsm::reachset* rss);

  private:
    // Helper, used by attachToParent.
    GraphLib::node_renumberer* initChain(GraphLib::dynamic_graph *g);

  public:
    virtual long getNumStates() const;
    virtual void getNumClasses(long &count) const;
    virtual void showClasses(OutputStream &os, state_lldsm::reachset* rss, 
        shared_state* st) const;
    virtual bool isTransient(long st) const;
    virtual statedist* getInitialDistribution() const;
    virtual long getOutgoingWeights(long from, long* to, double* w, long n) const;
    virtual bool computeTransient(double t, double* probs, 
        double* aux, double* aux2) const;
    virtual bool computeAccumulated(double t, const double* p0, double* n,
        double* aux, double* aux2) const;
    virtual bool computeSteadyState(double* probs) const;
    virtual bool computeTimeInStates(const double* p0, double* x) const;
    virtual bool computeClassProbs(const double* p0, double* x) const;
    virtual bool randomTTA(rng_stream &st, long &state, const stateset* final,
        long maxt, long &elapsed);
    virtual bool randomTTA(rng_stream &st, long &state, const stateset* final,
        double maxt, double &elapsed);

  public:

    inline void setTrapState(long t) {
      trap = t;
    }
    inline void setAcceptState(long a) {
      accept = a;
    }

    virtual long getTrapState() const { return trap; }
    virtual long getAcceptingState() const { return accept; }

    virtual bool computeDiscreteTTA(double epsilon, long maxsize, 
      discrete_pdf &dist) const;

    virtual bool computeContinuousTTA(double dt, double epsilon, 
      long maxsize, discrete_pdf &dist) const;

    // virtual bool reachesAccept(double* x) const;

    virtual bool reachesAcceptBy(double t, double* x) const;

    virtual void showInternal(OutputStream &os) const;

    virtual void showProc(OutputStream &os, 
      const graph_lldsm::reachgraph::show_options& opt, 
      state_lldsm::reachset* RSS, shared_state* st) const;


  // For reachgraphs hooked into this
  public:
    inline void getNumArcs(long &na) const {
      DCASSERT(chain);
      na = chain->getNumEdges();
    }

    void getDeadlocked(intset &r) const;
    void getTSCCsSatisfying(intset &p) const;

#ifdef USE_OLD_TRAVERSE_HELPER
    void count_edges(bool, traverse_helper&) const;
    void traverse(bool rt, bool one_step, traverse_helper &TH) const;
#else
    void count_edges(bool, ectl_reachgraph::CTL_traversal&) const;
    void traverse(bool rt, GraphLib::BF_graph_traversal &T) const;
#endif
  private:
    bool is_discrete;

    /// We hold this until attachToParent() is called.
    GraphLib::dynamic_graph* G;

    /// We hold this until attachToParent() is called.
    MCLib::vanishing_chain* VC;

    /// This is set up in attachToParent().
    MCLib::Markov_chain* chain;

    statedist* initial;
    long trap;
    long accept;

  /*
  private:
    class sparse_row_elems : public GraphLib::generic_graph::element_visitor {
        int alloc;
        const indexed_reachset::indexed_iterator &I;
        bool incoming;
        bool overflow;
      public:
        int last;
        long* index;
        double* value;
      public:
        sparse_row_elems(const indexed_reachset::indexed_iterator &i);
        virtual ~sparse_row_elems();

      protected:
        bool Enlarge(int ns);
      public:
        bool buildIncoming(Old_MCLib::Markov_chain* chain, int i);
        bool buildOutgoing(Old_MCLib::Markov_chain* chain, int i);

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
          SWAP(value[i], value[j]);
        }
    };  // inner class sparse_row_elems
    // ----------------------------------------------------------------------
  */
  /*
  private:
    class simple_outedges : public GraphLib::generic_graph::element_visitor {
      public:
          long edges;
          long* to;
          double* weights;
          long edge_alloc;
      public:
          simple_outedges(long* t, double* w, long n) {
            edges = 0;
            to = t;
            weights = w;
            edge_alloc = n;
          }
          virtual bool visit(long f, long t, void* label) {
            if (edges < edge_alloc) {
              to[edges] = t;
              weights[edges] = ((float*)label)[0];
            }
            edges++;
            return false;
          }
    };  // inner class simple_outedges
    // ----------------------------------------------------------------------
  */


};


// ******************************************************************
// *                                                                *
// *                     mclib_reachgraph class                     *
// *                                                                *
// ******************************************************************

/**
  Adapter, so we don't have to use the dreaded diamond.

  All of these methods simply call a method with the same name 
  in class mclib_process.
*/
class mclib_reachgraph : public ectl_reachgraph {
  public:
    mclib_reachgraph(mclib_process* MC);

  protected:
    virtual ~mclib_reachgraph();
    virtual const char* getClassName() const { return "mclib_reachgraph"; }

  public:
    virtual void getNumArcs(long &na) const;
    virtual void showInternal(OutputStream &os) const;
    virtual void showArcs(OutputStream &os, const show_options& opt, 
      state_lldsm::reachset* RSS, shared_state* st) const;

  protected:

#ifdef USE_OLD_TRAVERSE_HELPER
    virtual void count_edges(bool, traverse_helper&) const;
    virtual void traverse(bool rt, bool one_step, traverse_helper &TH) const;
#else
    virtual void count_edges(bool, CTL_traversal&) const;
    virtual void traverse(bool rt, GraphLib::BF_graph_traversal &T) const;
#endif

    virtual void getDeadlocked(intset &r) const;
    virtual void getTSCCsSatisfying(intset &p) const;

  private:
    mclib_process* chain;
};

#endif
