
// $Id$ 

#include "proc_meddly.h"
#include "rss_meddly.h"

// ******************************************************************
// *                                                                *
// *                     mclib_process  methods                     *
// *                                                                *
// ******************************************************************

meddly_process::meddly_process(meddly_encoder* wrap, shared_ddedge* mc)
{
  mxd_wrap = wrap;
  proc = mc;
}

meddly_process::~meddly_process()
{
  Delete(mxd_wrap);
  Delete(proc);
}

void meddly_process::attachToParent(stochastic_lldsm* p, LS_Vector &init, state_lldsm::reachset* rss)
{
  process::attachToParent(p, init, rss);

  // TBD
}

