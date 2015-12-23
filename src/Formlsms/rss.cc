
// $Id$

#include "../ExprLib/mod_vars.h"
#include "../ExprLib/exprman.h"
#include "rss.h"

reachset::reachset(const lldsm* p)
{
  parent = p;
}

reachset::~reachset()
{
}

void reachset::getNumStates(result &ns) const
{
  long lns;
  getNumStates(lns);
  if (lns>=0) {
    ns.setInt(lns);
  } else {
    ns.setNull();
  }
}

void reachset::showStates(OutputStream &os, int display_order, shared_state* st)
{
  long num_states;
  getNumStates(num_states);

  if (num_states<=0) return;
  if (parent->tooManyStates(num_states, true)) return;

  iterator& I = iteratorForOrder(display_order);

  long i = 0;
  for (I.start(); I; I++, i++) {
    os << "State " << i << ": ";
    I.copyState(st);
    showState(os, st);
    os << "\n";
    os.flush();
  }
}

void reachset::visitStates(lldsm::state_visitor &v, int visit_order)
{
  DCASSERT(v.state());

  iterator& I = iteratorForOrder(visit_order);

  for (I.start(); I; I++) {
    v.index() = I.index();
    if (v.canSkipIndex()) continue;
    I.copyState(v.state());
    if (v.visit()) return;
  }
}

