
// $Id$

#include "rss_meddly.h"
#include "../ExprLib/mod_vars.h"
#include "../Modules/meddly_ssets.h"


// ******************************************************************
// *                                                                *
// *                    meddly_reachset  methods                    *
// *                                                                *
// ******************************************************************

meddly_reachset::meddly_reachset()
{
  vars = 0;
  mdd_wrap = 0;
  initial = 0;
  states = 0;
  natorder = 0;
}

meddly_reachset::~meddly_reachset()
{
  // TBD!
  delete natorder;
  Delete(vars);
}

void meddly_reachset::getNumStates(long &ns) const
{
  DCASSERT(mdd_wrap);
  mdd_wrap->getCardinality(states, ns);
}

void meddly_reachset::getNumStates(result &ns) const
{
  DCASSERT(mdd_wrap);
  mdd_wrap->getCardinality(states, ns);
}

void meddly_reachset::showInternal(OutputStream &os) const
{
  os << "Internal state space representation (using MEDDLY):\n";
  mdd_wrap->showNodeGraph(os, states);
  os.flush();
}

void meddly_reachset::showState(OutputStream &os, const shared_state* st) const
{
  st->Print(os, 0);
}

graph_lldsm::reachset::iterator& meddly_reachset::iteratorForOrder(int)
{
  DCASSERT(natorder);
  return *natorder;
}

graph_lldsm::reachset::iterator& meddly_reachset::easiestIterator() const
{
  DCASSERT(natorder);
  return *natorder;
}

stateset* meddly_reachset::getReachable() const
{
  if (0==vars) return 0;
  if (0==mdd_wrap) return 0;

#ifdef NEW_STATESETS
  return new meddly_stateset(getParent(), Share(vars), Share(mdd_wrap), Share(states));
#else
  return 0;
#endif
}

// ******************************************************************
// *                                                                *
// *             meddly_reachset::lexical_iter  methods             *
// *                                                                *
// ******************************************************************

meddly_reachset::lexical_iter::lexical_iter(const meddly_encoder &w, shared_ddedge &s)
 : states(s), wrapper(w)
{
  iter = new MEDDLY::enumerator(MEDDLY::enumerator::FULL, states.getForest());
}

meddly_reachset::lexical_iter::~lexical_iter()
{
  delete iter;
}

void meddly_reachset::lexical_iter::start()
{
  DCASSERT(iter);
  iter->start(states.E);
  i = 0;
}

void meddly_reachset::lexical_iter::operator++(int)
{
  DCASSERT(iter);
  ++(*iter);
  ++i;
}

meddly_reachset::lexical_iter::operator bool() const
{
  DCASSERT(iter);
  return (*iter);
}

long meddly_reachset::lexical_iter::index() const
{
  return i;
}

void meddly_reachset::lexical_iter::copyState(shared_state* st) const
{
  DCASSERT(iter);
  const int* minterm = iter->getAssignments();
  wrapper.minterm2state(minterm, st);
}

