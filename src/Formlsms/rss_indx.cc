
#include "rss_indx.h"
#include "../Modules/expl_ssets.h"

// External libs
#include "../_LSLib/lslib.h"    // for LS_Vector

// ******************************************************************
// *                                                                *
// *                    indexed_reachset methods                    *
// *                                                                *
// ******************************************************************

indexed_reachset::indexed_reachset()
{
}

indexed_reachset::~indexed_reachset()
{
}

stateset* indexed_reachset::getReachable() const
{
  long num_states;
  getNumStates(num_states);
  intset* all = new intset(num_states);
  all->addAll();
  return new expl_stateset(getParent(), all);
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
  return new expl_stateset(getParent(), pset);
}

stateset* indexed_reachset::getInitialStates() const
{
  long num_states;
  getNumStates(num_states);
  intset* initss = new intset(getInitial());
  return new expl_stateset(getParent(), initss);
}

void indexed_reachset::setInitial(const LS_Vector &init)
{
  long num_states;
  getNumStates(num_states);
  initial.resetSize(num_states);
  initial.removeAll();
  if (init.index) {
    for (long z=0; z<init.size; z++) {
      CHECK_RANGE(0, init.index[z], num_states);
      initial.addElement(init.index[z]);
    }
  } else {
    if (init.d_value) {
      for (long i=0; i<init.size; i++) {
        if (init.d_value[i]) {
          initial.addElement(i);
        }
      }
    } else if (init.f_value) {
      for (long i=0; i<init.size; i++) {
        if (init.f_value[i]) {
          initial.addElement(i);
        }
      }
    }
  }
}

void indexed_reachset::Finish()
{
  // Do-nothing default
}

void indexed_reachset::Renumber(const GraphLib::node_renumberer* Ren)
{
  // Do-nothing default
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

