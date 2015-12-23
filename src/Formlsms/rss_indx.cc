
// $Id$

#include "rss_indx.h"
#include "check_llm.h"
#include "../Modules/statesets.h"

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

void indexed_reachset::getReachable(result &rs) const
{
  long num_states;
  getNumStates(num_states);
  if (0==num_states) {
    rs.setNull();
    return;
  }
  intset* all = new intset(num_states);
  all->addAll();
  rs.setPtr(new stateset(getParent(), all));
}

void indexed_reachset::getPotential(expr* p, result &x) const
{
  if (0==p) {
    x.setNull();
    return;
  }
  long num_states;
  getNumStates(num_states);
  if (0==num_states) {
    x.setNull();
    return;
  }

  intset* pset = new intset(num_states);
  const checkable_lldsm* LM = getParent();
  const hldsm* HM = LM ? LM->GetParent() : 0;
  pot_visit pv(HM, p, *pset);
  visitStates(pv);
  if (pv.isOK()) {
    x.setPtr(new stateset(LM, pset));
  } else {
    delete pset;
    x.setNull();
  }
}

void indexed_reachset::getInitialStates(result &x) const
{
  long num_states;
  getNumStates(num_states);
  if (0==num_states) {
    x.setNull();
    return;
  }
  intset* initss = new intset(num_states);
  initss->removeAll();
  
  if (initial.index) {
    for (long z=0; z<initial.size; z++)
      initss->addElement(initial.index[z]);
  } 

  x.setPtr(new stateset(getParent(), initss));
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

