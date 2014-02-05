
// $Id$

#ifndef RSS_MDD_H
#define RSS_MDD_H

#include "../ExprLib/mod_inst.h"
#include "../Modules/glue_meddly.h"

// ******************************************************************
// *                                                                *
// *                      meddly_states  class                      *
// *                                                                *
// ******************************************************************

class dsde_hlm;

class meddly_states : public shared_object {
public:
  MEDDLY::domain* vars;
  meddly_encoder* mdd_wrap;
  shared_ddedge* initial;
  shared_ddedge* states;
  // Optional: state indexes
  meddly_encoder* index_wrap;
  shared_ddedge* state_indexes;
  // For the "next-state function".
  meddly_encoder* mxd_wrap;
  shared_ddedge* nsf;
  // For the "process"
  meddly_encoder* proc_wrap;
  shared_ddedge* proc;
  bool proc_uses_actual;
public:
  meddly_states();
protected:
  virtual ~meddly_states();
public:
  // handy functions
//  bool createVarsBottomUp(const dsde_hlm &m, int* b, int nl);
  bool createVars(const dsde_hlm &m, MEDDLY::variable** v, int nv);

  inline int getNumLevels() const {
    DCASSERT(vars);
    // Includes terminal level...
    return vars->getNumVariables() + 1;
  }

  // required for shared_object
  virtual bool Print(OutputStream &s, int) const;
  virtual bool Equals(const shared_object *o) const;

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
  inline const MEDDLY::dd_edge& getNSF() const {
    DCASSERT(nsf);
    return nsf->E;
  }

  inline int getNumVars() const {
    DCASSERT(mdd_wrap);
    return mdd_wrap->getNumDDVars();
  }

  // TBD: use an enum for show, e.g., what order?
  long getNumStates(const lldsm* p, OutputStream &os, bool show);

  void buildIndexSet();

  void visitStates(lldsm::state_visitor &x) const;

  inline void getNumStates(result &count) const {
    DCASSERT(mdd_wrap);
    mdd_wrap->getCardinality(states, count);
  }

  void reportStats(OutputStream &out) const;
};

#endif
