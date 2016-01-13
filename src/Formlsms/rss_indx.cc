
// $Id$

#include "rss_indx.h"
#include "../Modules/expl_ssets.h"

// External libs
#include "lslib.h"    // for LS_Vector
#include "intset.h"   // for intset

// ******************************************************************
// *                                                                *
// *                    indexed_reachset methods                    *
// *                                                                *
// ******************************************************************

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
  getInitial(*initss);
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

void indexed_reachset::getInitial(intset& init) const
{
  init.removeAll();
  if (initial.index) {
    for (long z=0; z<initial.size; z++)
      init.addElement(initial.index[z]);
  } 
}

void indexed_reachset::Finish()
{
}

void indexed_reachset::Renumber(const long* ren)
{
}

// ******************************************************************
// *           indexed_reachset::indexed_iterator methods           *
// ******************************************************************

indexed_reachset::indexed_iterator::indexed_iterator(long ns)
{
  num_states = ns;
  map = 0;
  invmap = 0;
  I = ns;
}

indexed_reachset::indexed_iterator::~indexed_iterator()
{
  delete[] map;
  delete[] invmap;
}

void indexed_reachset::indexed_iterator::start()
{
  I = 0;
}

void indexed_reachset::indexed_iterator::operator++(int)
{
  I++;
}

indexed_reachset::indexed_iterator::operator bool() const
{
  return I < num_states;
}

long indexed_reachset::indexed_iterator::index() const
{
  return getIndex();
}

void indexed_reachset::indexed_iterator::copyState(shared_state* st) const
{
  copyState(st, I);
}

void indexed_reachset::indexed_iterator::setMap(long* m)
{
  DCASSERT(0==map);
  DCASSERT(0==invmap);
  if (0==m) return;

  map = m;
  invmap = new long[num_states];
  for (long i=0; i<num_states; i++) {
    CHECK_RANGE(0, map[i], num_states);
    invmap[map[i]] = i;
  }
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

