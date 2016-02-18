
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
    meddly_process(meddly_encoder* wrap, shared_ddedge* mc);
    // TBD - potential vs actual?

  protected:
    virtual ~meddly_process();
    virtual const char* getClassName() const { return "meddly_process"; }
    virtual void attachToParent(stochastic_lldsm* p, LS_Vector &init, state_lldsm::reachset* rss);

  private:
    meddly_encoder* mxd_wrap;
    shared_ddedge* proc;
};


#endif

