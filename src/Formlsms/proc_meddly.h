
// $Id$

#ifndef PROC_MEDDLY_H
#define PROC_MEDDLY_H

#include "proc_markov.h"
#include "../Modules/glue_meddly.h"

// ******************************************************************
// *                                                                *
// *                      meddly_process  class                      *
// *                                                                *
// ******************************************************************

class meddly_process : public markov_process {

  public:
    meddly_process(meddly_encoder* wrap);
    // TBD - potential vs actual?

  protected:
    virtual ~meddly_process();
    virtual const char* getClassName() const { return "meddly_process"; }
    virtual void attachToParent(stochastic_lldsm* p, LS_Vector &init, state_lldsm::reachset* rss);

  public:
    inline void setActual(shared_ddedge* p) {
      // TBD?
      proc = p;
    }

    inline meddly_encoder& useMxdWrapper() {
      DCASSERT(mxd_wrap);
      return *mxd_wrap;
    }

  //
  // Required
  //
  public:
    virtual long getNumStates() const;
    virtual void showProc(OutputStream &os, 
      const graph_lldsm::reachgraph::show_options &opt, 
      state_lldsm::reachset* RSS, shared_state* st) const;
    virtual void showInternal(OutputStream &os) const;
    virtual void getNumClasses(long &count) const;
    virtual void showClasses(OutputStream &os, state_lldsm::reachset* rss, 
      shared_state* st) const;
    virtual bool isTransient(long st) const;
    virtual statedist* getInitialDistribution() const;

  private:
    meddly_encoder* mxd_wrap;
    shared_ddedge* proc;

    long num_states;
};


#endif

