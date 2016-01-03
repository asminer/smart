
// $Id$

#ifndef RSS_MDD_H
#define RSS_MDD_H

#include "graph_llm.h"
#include "../Modules/glue_meddly.h"

// ******************************************************************
// *                                                                *
// *                      meddly_states  class                      *
// *                                                                *
// ******************************************************************

class dsde_hlm;

/*
    TBD - converting this into a proper class
          rather than a struct as it is currently

    TBD - use class hierarchy for the pieces?
          E.g., "final" nsf stored in various ways (monolithic,
          by levels, by events).  Virtual functions for
            CTL model checking support

          E.g., final process (nsf is a subclass of this).
            counting arcs
            displaying arcs

          Inside - proc_wrap and necessary edges

          Ideally - this class hierarchy can be independent of meddly

          What we need:
            "state space" class with supporting functions?
            "graph" class with supporting functions?

          fsm_llm and fsm_mdd merge, implementation details pushed to 
            ss/graph classes.

          mc_llm and mc_mdd merge, implementation details pushed to 
            ss/graph classes.

          How do we keep states & graphs "compatible"?
          
*/
class meddly_states : public shared_object {
  // Domain 
  MEDDLY::domain* vars;

  // State encoder, and sets of states
  meddly_encoder* mdd_wrap;
  shared_ddedge* initial;

public:
  shared_ddedge* states;
  // Optional: state indexes
  meddly_encoder* index_wrap;
  shared_ddedge* state_indexes;
  // For the "next-state function".
  meddly_encoder* mxd_wrap;
  shared_ddedge* nsf;

  // TBD - nsf should be an array of edges;
  // monolithic with dimension 1, or
  // by events or other partitioning with dimension >1.
  
  // OR, should we just use the MEDDLY mechanism for 
  // partitioning nsfs on the fly?

// protected:
  // For the "process"
  meddly_encoder* proc_wrap;
  shared_ddedge* proc;
  bool proc_uses_actual;

  // TBD - proc should be an array of edges;
  // monolithic with dimension 1, or 
  // by events or other partitioning with dimension >1.

  // Also we should keep "potential" and "actual" copies,
  // with actual built only when needed.
  // Then eliminate "proc_uses_actual" and just switch between them
  // outside of here.

public:
  meddly_states();
protected:
  virtual ~meddly_states();

// Methods involving the domain
public:
//  bool createVarsBottomUp(const dsde_hlm &m, int* b, int nl);
  bool createVars(const dsde_hlm &m, MEDDLY::variable** v, int nv);

  inline int getNumLevels() const {
    DCASSERT(vars);
    // Includes terminal level...
    return vars->getNumVariables() + 1;
  }

  inline int getNumVars() const {
    DCASSERT(mdd_wrap);
    return mdd_wrap->getNumDDVars();
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

// Methods involving the states or mdd wrapper
public:
  inline meddly_encoder* 
  copyMddWrapperWithDifferentForest(const char* what, MEDDLY::forest* f) const
  {
    DCASSERT(mdd_wrap);
    return mdd_wrap->copyWithDifferentForest(what, f);
  }

  inline MEDDLY::forest* getMddForest() {
    DCASSERT(mdd_wrap);
    return mdd_wrap->getForest();
  }

  inline bool hasMddWrapper() const {
    return mdd_wrap;
  }

  inline bool hasStates() const {
    return states;
  }

  inline bool hasInitial() const {
    return initial;
  }

  inline meddly_encoder* shareMddWrap() {
    return Share(mdd_wrap);
  }

  inline shared_ddedge* shareStates() {
    return Share(states);
  }

  inline shared_ddedge* shareInitial() {
    return Share(initial);
  }

  inline void setMddWrap(meddly_encoder* w) {
    DCASSERT(0==mdd_wrap);
    mdd_wrap = w;
  }

  inline void setStates(shared_ddedge* S) {
    DCASSERT(0==states);
    states = S;
  }

  inline void setInitial(shared_ddedge* I) {
    DCASSERT(0==initial);
    initial = I;
  }

  inline const MEDDLY::dd_edge& getInitial() const {
    DCASSERT(initial);
    return initial->E;
  }

  inline shared_ddedge* newMddEdge() {
    DCASSERT(mdd_wrap);
    return new shared_ddedge(mdd_wrap->getForest());
  }

  inline void MddState2Minterm(const shared_state* s, int* mt) const {
    DCASSERT(mdd_wrap);
    mdd_wrap->state2minterm(s, mt);
  }
  inline void MddMinterm2State(const int* mt, shared_state* s) const {
    DCASSERT(mdd_wrap);
    mdd_wrap->minterm2state(mt, s);
  }

  inline void createMddMinterms(const int* const* mts, int n, shared_object* ans)
  {
    DCASSERT(mdd_wrap);
    mdd_wrap->createMinterms(mts, n, ans);
  }
  
  inline meddly_encoder& useMddWrap() {
    DCASSERT(mdd_wrap);
    return *mdd_wrap;
  }

  template <class INT>
  inline void getNumStates(INT &count) const {
    DCASSERT(mdd_wrap);
    mdd_wrap->getCardinality(states, count);
  }

  // TBD: use an enum for show, e.g., what order?
  void showStates(const lldsm* p, OutputStream &os, bool internal);

  void visitStates(lldsm::state_visitor &x) const;


// Methods involving the next-state function or mxd wrapper
public:
  inline shared_ddedge* buildActualNSF() const {
    // TBD - this will become more complex when NSF isn't monolithic
    DCASSERT(mxd_wrap);
    DCASSERT(nsf);
    DCASSERT(states);
    shared_ddedge* actual = new shared_ddedge(mxd_wrap->getForest());
    mxd_wrap->selectRows(nsf, states, actual);
    return actual;
  }

  inline meddly_encoder* shareMxdWrap() {
    return Share(mxd_wrap);
  }

  inline shared_ddedge* shareNSF() {
    return Share(nsf);
  }

  inline void setMxdWrap(meddly_encoder* w) {
    DCASSERT(0==mxd_wrap);
    mxd_wrap = w;
  }


  inline const MEDDLY::dd_edge& getNSF() const {
    DCASSERT(nsf);
    return nsf->E;
  }

  inline meddly_encoder& useMxdWrap() {
    DCASSERT(mxd_wrap);
    return *mxd_wrap;
  }

  template <class INT>
  void getNumArcs(INT &count) const;

  // TBD: use an enum for show, e.g., what order?
  void showArcs(const checkable_lldsm* p, OutputStream &os, bool internal, bool node_names);

// Methods involving the process 
public:
  inline shared_ddedge* buildActualProcess() const {
    // TBD - this will become more complex when the process isn't monolithic
    DCASSERT(proc_wrap);
    DCASSERT(proc);
    DCASSERT(states);
    shared_ddedge* actual = new shared_ddedge(proc_wrap->getForest());
    proc_wrap->selectRows(proc, states, actual);
    return actual;
  }


public:
  // handy functions

  // required for shared_object
  virtual bool Print(OutputStream &s, int) const;
  virtual bool Equals(const shared_object *o) const;

  /*
  inline const MEDDLY::dd_edge& getInit() const { 
    DCASSERT(initial);
    return initial->E;
  };
  inline const MEDDLY::dd_edge& getStates() const {
    DCASSERT(states);
    return states->E;
  }
  inline MEDDLY::dd_edge& useStates() {
    DCASSERT(states);
    return states->E;
  }
*/

  void buildIndexSet();

  // TBD - void getNumArcs(result &count) const
  // TBD - long getNumArcs() const
  // TBD - showArcs
  // TBD - buildActual

  void reportStats(OutputStream &out) const;
};

template <class INT>
void meddly_states::getNumArcs(INT &count) const
{
  DCASSERT(proc_wrap);
  if (proc_uses_actual) {
    proc_wrap->getCardinality(proc, count);
  } else {
    // TBD - this should be the "process" not the "nsf"
    shared_object* actual = buildActualNSF();
    proc_wrap->getCardinality(actual, count);
    Delete(actual);
  }
}


#endif
