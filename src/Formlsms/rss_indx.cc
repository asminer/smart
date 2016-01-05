
// $Id$

#include "rss_indx.h"
#include "../Modules/expl_ssets.h"

// External libs
#include "lslib.h"    // for LS_Vector
#include "intset.h"   // for intset


indexed_reachset::indexed_reachset()
{
  // clear the initial vector
  initial.size = 0;
  initial.index = 0;
  initial.d_value = 0;
  initial.f_value = 0;
}

indexed_reachset::~indexed_reachset()
{
  delete[] initial.index;
}

stateset* indexed_reachset::getReachable() const
{
  long num_states;
  getNumStates(num_states);
  intset* all = new intset(num_states);
  all->addAll();
#ifdef NEW_STATESETS
  return new expl_stateset(getParent(), all);
#else
  return new stateset(getParent(), all);
#endif
}

stateset* indexed_reachset::getPotential(expr* p) const
{
  long num_states;
  getNumStates(num_states);
  intset* pset = new intset(num_states);
  if (p) {
    const hldsm* HM = getGrandParent();
    pot_visit pv(HM, p, *pset);
    visitStates(pv);
    if (!pv.isOK()) {
      delete pset;
      return 0;
    }
  } else {
    pset->removeAll();
  }
#ifdef NEW_STATESETS
  return new expl_stateset(getParent(), pset);
#else
  return new stateset(getParent(), pset);
#endif
}

stateset* indexed_reachset::getInitialStates() const
{
  long num_states;
  getNumStates(num_states);
  intset* initss = new intset(num_states);
  initss->removeAll();
  if (initial.index) {
    for (long z=0; z<initial.size; z++)
      initss->addElement(initial.index[z]);
  } 
#ifdef NEW_STATESETS
  return new expl_stateset(getParent(), initss);
#else
  return new stateset(getParent(), initss);
#endif
}

void indexed_reachset::setInitial(LS_Vector &init)
{
  DCASSERT(0==init.d_value);
  DCASSERT(0==init.f_value);
  initial = init;
}

// ******************************************************************
// *              indexed_reachset::pot_visit  methods              *
// ******************************************************************

indexed_reachset::pot_visit::pot_visit(const hldsm* mdl, expr* _p, intset &ps)
 : state_visitor(mdl), pset(ps)
{
  p = _p;
  p->PreCompute();
  x.answer = &tmp;
  pset.removeAll();
  ok = true;
}

bool indexed_reachset::pot_visit::visit()
{
  p->Compute(x);
  if (!tmp.isNormal()) {
    ok = false;
    return true;
  }
  if (tmp.getBool()) pset.addElement(x.current_state_index);
  return false;
}

