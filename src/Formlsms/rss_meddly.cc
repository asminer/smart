
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
  mtmdd_wrap = 0;
}

meddly_reachset::~meddly_reachset()
{
  delete natorder;
  Delete(vars);
  Delete(initial);
  Delete(states);
  Delete(mdd_wrap);
  Delete(mtmdd_wrap);
}

bool meddly_reachset::createVars(MEDDLY::variable** v, int nv)
{
  try {
    vars = new shared_domain(v, nv);
    return true;
  }
  catch (MEDDLY::error de) {
    if (em->startError()) {
      em->noCause();
      em->cerr() << "Error creating domain: " << de.getName();
      em->stopIO();
    }
    return false;
  }
}

void meddly_reachset::setMddWrap(meddly_encoder* w)
{
  DCASSERT(0==mdd_wrap);
  mdd_wrap = w;
  DCASSERT(0==mtmdd_wrap);
  MEDDLY::forest* foo = vars->createForest(
    false, MEDDLY::forest::INTEGER, MEDDLY::forest::MULTI_TERMINAL
  );
  mtmdd_wrap = mdd_wrap->copyWithDifferentForest("MTMDD", foo);
}

void meddly_reachset::reportStats(OutputStream &out) const
{
  if (0==states) return;
  double card;
  mdd_wrap->getCardinality(states, card);
  out << "\tApproximately " << card << " states\n";
  out << "\tDD Sizes for various structures:\n";
  if (initial) {
    long nc = initial->E.getNodeCount();
    out << "\t    Initial set    requires " << nc << " nodes\n";
  }
  if (states) {
    long nc = states->E.getNodeCount();
    out << "\t    Reachable set  requires " << nc << " nodes\n";
  }
  if (mdd_wrap)     mdd_wrap->reportStats(out);
  if (mtmdd_wrap)   mtmdd_wrap->reportStats(out);
  //  if (index_wrap) index_wrap->reportStats(out);
}

void meddly_reachset::setStates(shared_ddedge* S) 
{
  if (states == S) return;  // could happen?

  DCASSERT(0==states);
  states = S;

  // fix iterators
  delete natorder;

  if (states) {
    DCASSERT(mdd_wrap);
    natorder = new lexical_iter(*mdd_wrap, *states);
  } else {
    natorder = 0;
  }
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

state_lldsm::reachset::iterator& 
meddly_reachset::iteratorForOrder(state_lldsm::display_order ord)
{
  DCASSERT(natorder);
  return *natorder;
}

state_lldsm::reachset::iterator& meddly_reachset::easiestIterator() const
{
  DCASSERT(natorder);
  return *natorder;
}

stateset* meddly_reachset::getReachable() const
{
  if (0==vars) return 0;
  if (0==mdd_wrap) return 0;
  if (0==states) return 0;

  return new meddly_stateset(getParent(), Share(vars), Share(mdd_wrap), Share(states));
}

stateset* meddly_reachset::getInitialStates() const
{
  if (0==vars) return 0;
  if (0==mdd_wrap) return 0;
  if (0==initial) return 0;

  return new meddly_stateset(getParent(), Share(vars), Share(mdd_wrap), Share(initial));
}

stateset* meddly_reachset::getPotential(expr* p) const
{
  if (0==vars) return 0;
  if (0==mdd_wrap) return 0;
  if (0==initial) return 0;

  //
  // Build expression as MTMDD
  //
  traverse_data x(traverse_data::BuildDD);
  result answer;
  x.answer = &answer;
  x.ddlib = mtmdd_wrap;
  p->Traverse(x);
  if (!answer.isNormal()) {
    // anything else?  Make noise?
    return 0;
  }

  shared_ddedge* ans = smart_cast <shared_ddedge*> (answer.getPtr());
  DCASSERT(ans);
  return new meddly_stateset(getParent(), Share(vars), Share(mdd_wrap), Share(ans));
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

