
// $Id$

#include "../ExprLib/mod_vars.h"
#include "../ExprLib/exprman.h"
#include "rss.h"
#include "check_llm.h"

reachset::reachset()
{
  parent = 0;
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
  DCASSERT(parent);

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

void reachset::visitStates(lldsm::state_visitor &v) const
{
  DCASSERT(v.state());

  iterator& I = easiestIterator();

  for (I.start(); I; I++) {
    v.index() = I.index();
    if (v.canSkipIndex()) continue;
    I.copyState(v.state());
    if (v.visit()) return;
  }
}

bool reachset::Print(OutputStream &s, int width) const
{
  // Required for shared object, but will we ever call it?
  s << "reachset (why is it printing?)";
  return true;
}

bool reachset::Equals(const shared_object* o) const
{
  // Required for shared object, but will we ever call it?
  fprintf(stderr, "Inside reachset::Equals, why?\n");
  return (this == o);
}


reachset::iterator::iterator()
{
}

reachset::iterator::~iterator()
{
}

