
// $Id$

#ifndef PROC_MCLIB_H
#define PROC_MCLIB_H

#include "proc_markov.h"
#include "rss_indx.h"
#include "rgr_ectl.h"

#include "mclib.h"
#include "intset.h"

// ******************************************************************
// *                                                                *
// *                      mclib_process  class                      *
// *                                                                *
// ******************************************************************

class mclib_process : public markov_process {

  public:
    mclib_process(MCLib::Markov_chain* mc);

  protected:
    virtual ~mclib_process();
    virtual const char* getClassName() const { return "mclib_process"; }
    virtual void attachToParent(stochastic_lldsm* p, state_lldsm::reachset* rss);

  public:
    virtual long getNumStates() const;
    virtual void getNumClasses(long &count) const;
    virtual void showClasses(OutputStream &os, shared_state* st) const;
    virtual bool isTransient(long st) const;
    virtual statedist* getInitialDistribution() const;

    void setInitial(LS_Vector init);

    inline void setTrapState(long t) {
      trap = t;
    }
    inline void setAcceptState(long a) {
      accept = a;
    }

    virtual long getTrapState() const { return trap; }
    virtual long getAcceptingState() const { return accept; }

  // For reachgraphs hooked into this
  public:
    inline void getNumArcs(long &na) const {
      DCASSERT(chain);
      na = chain->getNumArcs();
    }
    void showInternal(OutputStream &os) const;
    void showArcs(OutputStream &os, state_lldsm::reachset* RSS, 
      state_lldsm::display_order ord, shared_state* st) const;

    inline bool forward(const intset& p, intset &r) const {
      DCASSERT(chain);
      return chain->getForward(p, r);
    }

    inline bool backward(const intset& p, intset &r) const {
      DCASSERT(chain);
      return chain->getBackward(p, r);
    }

    inline void absorbing(intset &r) const {
      //
      // Determine set of states with NO outgoing edges
      //
      DCASSERT(chain);
      r.removeAll();
      if (chain->isDiscrete()) return; 
      long fa = chain->getFirstAbsorbing();
      if (fa < 0) return; // no absorbing states
      long la = chain->getNumStates();
      r.addRange(fa, la-1);
    }

  private:
    MCLib::Markov_chain* chain;
    LS_Vector initial;
    long trap;
    long accept;

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
      bool buildIncoming(MCLib::Markov_chain* chain, int i);
      bool buildOutgoing(MCLib::Markov_chain* chain, int i);

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
    };
};


// ******************************************************************
// *                                                                *
// *                     mclib_reachgraph class                     *
// *                                                                *
// ******************************************************************

// Adapter, so we don't have to use the dreaded diamond.
class mclib_reachgraph : public ectl_reachgraph {
  public:
    mclib_reachgraph(mclib_process* MC);

  protected:
    virtual ~mclib_reachgraph();
    virtual const char* getClassName() const { return "mclib_reachgraph"; }

  public:
    virtual void getNumArcs(long &na) const;
    virtual void showInternal(OutputStream &os) const;
    virtual void showArcs(OutputStream &os, state_lldsm::reachset* RSS, 
      state_lldsm::display_order ord, shared_state* st) const;

  protected:
    virtual bool forward(const intset& p, intset &r) const;
    virtual bool backward(const intset& p, intset &r) const;
    virtual void absorbing(intset &r) const;
    
  private:
    mclib_process* chain;
};

#endif
