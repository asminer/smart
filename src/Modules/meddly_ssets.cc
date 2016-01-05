
// $Id$

#include "../ExprLib/exprman.h"
#include "../ExprLib/mod_vars.h"
#include "../Formlsms/graph_llm.h"
#include "glue_meddly.h"
#include "meddly_ssets.h"

#ifdef NEW_STATESETS

// ******************************************************************
// *                                                                *
// *                     meddly_stateset  class                     *
// *                                                                *
// ******************************************************************

meddly_stateset::meddly_stateset(const graph_lldsm* p, shared_domain* sd, 
  meddly_encoder* me, shared_ddedge* E) : stateset(p)
{
  vars = sd;
  mdd_wrap = me;
  states = E;
}

meddly_stateset::~meddly_stateset()
{
  Delete(vars);
  Delete(mdd_wrap);
  Delete(states);
}

stateset* meddly_stateset::DeepCopy() const
{
  return new meddly_stateset(getParent(), Share(vars), Share(mdd_wrap), Share(states));
}

bool meddly_stateset::Complement()
{
  shared_ddedge* ans = new shared_ddedge(mdd_wrap->getForest());
  mdd_wrap->buildUnary(exprman::uop_not, states, ans);
  Delete(states);
  states = ans;
  return true;
  // TBD - use a try/catch block to check for errors
}

bool meddly_stateset::Union(const expr* c, const char* op, const stateset* x)
{
  const meddly_stateset* mx = dynamic_cast <const meddly_stateset*> (x);
  if (0==mx) {
    storageMismatchError(c, op);
    return false;
  }
  DCASSERT(vars == mx->vars);
  DCASSERT(mdd_wrap == mx->mdd_wrap);

  // can this happen?
  if (0==mx->states) {
    return true;
  }

  // can this happen?
  if (0==states) {
    states = Share(mx->states);
    return true;
  }

  shared_ddedge* ans = new shared_ddedge(mdd_wrap->getForest());
  mdd_wrap->buildAssoc(states, false, exprman::aop_or, mx->states, ans);
  Delete(states);
  states = ans;
  return true;
  // TBD - use a try/catch block to check for errors
}

bool meddly_stateset::Intersect(const expr* c, const char* op, const stateset* x)
{
  const meddly_stateset* mx = dynamic_cast <const meddly_stateset*> (x);
  if (0==mx) {
    storageMismatchError(c, op);
    return false;
  }
  DCASSERT(vars == mx->vars);
  DCASSERT(mdd_wrap == mx->mdd_wrap);

  // can this happen?
  if (0==mx->states) {
    return true;
  }

  // can this happen?
  if (0==states) {
    states = Share(mx->states);
    return true;
  }

  shared_ddedge* ans = new shared_ddedge(mdd_wrap->getForest());
  mdd_wrap->buildAssoc(states, false, exprman::aop_and, mx->states, ans);
  Delete(states);
  states = ans;
  return true;
  // TBD - use a try/catch block to check for errors
}

void meddly_stateset::getCardinality(long &card) const
{
  mdd_wrap->getCardinality(states, card);
}

void meddly_stateset::getCardinality(result &x) const
{
  mdd_wrap->getCardinality(states, x);
}

bool meddly_stateset::Print(OutputStream &s, int) const
{
  // TBD - option for printing indexes instead?

  const hldsm* hm = getGrandparent();
  DCASSERT(hm);

  shared_state* st = new shared_state(hm);
  const int* mt = mdd_wrap->firstMinterm(states);
  s.Put('{');
  bool comma = false;
  while (mt) {
    if (comma)  s << ", ";
    else        comma = true;
    mdd_wrap->minterm2state(mt, st);
    hm->showState(s, st);
    mt = mdd_wrap->nextMinterm(states);
  }
  s.Put('}');
  Delete(st);
  return true;
}

bool meddly_stateset::Equals(const shared_object *o) const
{
  const meddly_stateset* ms = dynamic_cast <const meddly_stateset*> (o);
  if (0==ms) return false;

  DCASSERT(vars);
  if (!vars->Equals(ms->vars)) return false;

  DCASSERT(mdd_wrap);
  if (!mdd_wrap->Equals(ms->mdd_wrap)) return false;

  if (states) {
    return states->Equals(ms->states);
  } else {
    return 0==ms->states;
  }
}

#endif

// **************************************************************************
// *                                                                        *
// *                               Front  end                               *
// *                                                                        *
// **************************************************************************

void InitMeddlyStatesets(exprman* em)
{
  // Nothing to do actually
}

