
// $Id$

#ifndef RSS_MEDDLY_H
#define RSS_MEDDLY_H

#include "state_llm.h"
#include "../Modules/glue_meddly.h"

class dsde_hlm;

class meddly_reachset : public state_lldsm::reachset {
  public:
    meddly_reachset();

  protected:
    virtual ~meddly_reachset();
    virtual const char* getClassName() const { return "meddly_reachset"; }

  //
  // Helpers
  //
  public:

    /**
        Initialize the domain.
    */
    bool createVars(MEDDLY::variable** v, int nv);

    shared_domain* shareVars() const {
      return Share(vars);
    }

    inline MEDDLY::forest* createForest(bool rel, MEDDLY::forest::range_type t,
      MEDDLY::forest::edge_labeling ev)
    {
      DCASSERT(vars);
      return vars->createForest(rel, t, ev);
    }

    inline MEDDLY::forest* createForest(bool rel, MEDDLY::forest::range_type t,
      MEDDLY::forest::edge_labeling ev, const MEDDLY::forest::policies &p)
    {
      DCASSERT(vars);
      return vars->createForest(rel, t, ev, p);
    }

    inline int getNumLevels() const {
      DCASSERT(vars);
      // Includes terminal level...
      return vars->getNumLevels();
    }

    inline void MddState2Minterm(const shared_state* s, int* mt) const {
      DCASSERT(mdd_wrap);
      mdd_wrap->state2minterm(s, mt);
    }

    inline void MddMinterm2State(const int* mt, shared_state* s) const {
      DCASSERT(mdd_wrap);
      mdd_wrap->minterm2state(mt, s);
    }

    inline MEDDLY::forest* getMddForest() const {
      DCASSERT(mdd_wrap);
      return mdd_wrap->getForest();
    }

    inline shared_ddedge* newMddEdge() const {
      return new shared_ddedge(getMddForest());
    }

    inline shared_ddedge* newMddConst(bool v) const {
      shared_ddedge* ans = newMddEdge();
      mdd_wrap->buildSymbolicConst(v, ans);
      return ans;
    }

    inline void createMinterms(const int* const* mts, int n, shared_object* ans) {
      DCASSERT(mdd_wrap);
      mdd_wrap->createMinterms(mts, n, ans);
    }
  
    inline void setInitial(shared_ddedge* I) {
      DCASSERT(0==initial);
      initial = I;
    }

    inline const MEDDLY::dd_edge& getInitial() const {
      DCASSERT(initial);
      return initial->E;
    }
    
    void setMddWrap(meddly_encoder* w);

    void reportStats(OutputStream &out) const;

    void setStates(shared_ddedge* S);

    inline shared_ddedge* copyStates() const {
      return Share(states);
    }

    inline bool hasStates() const {
      return states;
    }

    inline const MEDDLY::dd_edge& getStates() const {
      DCASSERT(states);
      return states->E;
    }

  //
  // Required for reachsets
  //
  public:
    virtual void getNumStates(long &ns) const;
    virtual void getNumStates(result &ns) const;  
    virtual void showInternal(OutputStream &os) const;
    virtual void showState(OutputStream &os, const shared_state* st) const;
    virtual iterator& iteratorForOrder(state_lldsm::display_order ord);
    virtual iterator& easiestIterator() const;

    virtual stateset* getReachable() const;
    virtual stateset* getInitialStates() const;
    virtual stateset* getPotential(expr* p) const;


  // 
  // Bonus features
  //
  public:
    //
    // Build the index set, so we can quickly determine the
    // index of a given state.
    void buildIndexSet();

    //
    // Get the index of the specified minterm
    //
    inline long getMintermIndex(const int* mt) const {
      DCASSERT(index_wrap);
      DCASSERT(state_indexes);
      int index;
      index_wrap->getForest()->evaluate(state_indexes->E, mt, index);
      return index;
    }

    //
    // Remember the mxd wrapper for later (explicit only)
    //
    inline void saveMxdWrapper(meddly_encoder* mw) {
      DCASSERT(0==mxd_wrap);
      mxd_wrap = mw;
    }

    inline meddly_encoder* grabMxdWrapper() {
      meddly_encoder* foo = mxd_wrap;
      mxd_wrap = 0;
      return foo;
    }
    

  public: 

    // TBD - we may want an abstract base class for this
    class lexical_iter : public reachset::iterator {
      public:
        lexical_iter(const meddly_encoder &w, shared_ddedge &s);
        virtual ~lexical_iter();

        virtual void start();
        virtual void operator++(int);
        virtual operator bool() const;
        virtual long index() const;
        virtual void copyState(shared_state* st) const;

        inline const int* getCurrentMinterm() const {
          DCASSERT(iter);
          return iter->getAssignments();
        }

      private:
        shared_ddedge &states;
        const meddly_encoder &wrapper;
        MEDDLY::enumerator* iter;
        long i;
    };

  private:
    shared_domain* vars;

    meddly_encoder* mdd_wrap;
    shared_ddedge* initial;
    shared_ddedge* states;

    reachset::iterator* natorder;

    // Scratch space for getPotential()
    meddly_encoder* mtmdd_wrap;

    // for indexing states
    meddly_encoder* index_wrap;
    shared_ddedge* state_indexes;

    // Total kludge for 2-phase explicit generation
    // Remember the mxd wrapper for phase 2
    meddly_encoder* mxd_wrap;
};

#endif

