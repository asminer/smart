
// $Id$

#include "rgr_meddly.h"
#include "rss_meddly.h"

// ******************************************************************
// *                                                                *
// *                  meddly_monolithic_rg methods                  *
// *                                                                *
// ******************************************************************

meddly_monolithic_rg::meddly_monolithic_rg(meddly_reachset &rss)
{
  vars = rss.shareVars(); 
  mxd_wrap = 0;
  edges = 0;
}

meddly_monolithic_rg::~meddly_monolithic_rg()
{
  Delete(vars);
  Delete(edges);
  Delete(mxd_wrap);
}

