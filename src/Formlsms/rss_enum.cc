
// $Id$

#include "rss_enum.h"
#include "../ExprLib/exprman.h"
#include "../ExprLib/mod_vars.h"

// ******************************************************************
// *                                                                *
// *                     enum_reachset  methods                     *
// *                                                                *
// ******************************************************************

enum_reachset::enum_reachset(model_enum* ss)
{
  states = ss;
  DCASSERT(states);
  natorder = new natural_iter(*states);
  lexorder = 0;
  discorder = 0;

  // keep track of which state has each index
  state_handle = new long[states->NumValues()];
  for (long j=0; j<states->NumValues(); j++) {
    const model_enum_value* st = states->ReadValue(j);
    CHECK_RANGE(0, st->GetIndex(), states->NumValues());
    state_handle[st->GetIndex()] = j;
  }
}

enum_reachset::~enum_reachset()
{
  Delete(states);
  delete natorder;
  delete lexorder;
  delete discorder;
  delete[] state_handle;
}

void enum_reachset::getNumStates(long &ns) const
{
  ns = states->NumValues();
}

void enum_reachset::showInternal(OutputStream &os) const
{
  for (long i=0; i<states->NumValues(); i++) {
    const model_enum_value* st = states->ReadValue(i);
    os << "State " << i << " internal: (index " << st->GetIndex() << ") ";
    os.Put(st->Name());
    os << "\n";
    os.flush();
  } // for i
}

void enum_reachset::showState(OutputStream &os, const shared_state* st) const
{
  long i = st->get(states->GetIndex());
  const model_enum_value* mev = states->ReadValue(state_handle[i]);
  os.Put(mev->Name());
}

state_lldsm::reachset::iterator& enum_reachset
::iteratorForOrder(state_lldsm::display_order ord)
{
  DCASSERT(states);
  switch (ord) {

    case state_lldsm::LEXICAL:
        if (0==lexorder) lexorder = new lexical_iter(*states);
        return *lexorder;

    case state_lldsm::DISCOVERY:
        if (0==discorder) discorder = new discovery_iter(*states);
        return *discorder;

    case state_lldsm::NATURAL:
    default:
        if (0==natorder) natorder = new natural_iter(*states);
        return *natorder;
  }
}

state_lldsm::reachset::iterator& enum_reachset::easiestIterator() const
{
  DCASSERT(natorder);
  return *natorder;
}

shared_object* enum_reachset::getEnumeratedState(long i) const 
{
  DCASSERT(state_handle);
  return Share(states->GetValue(state_handle[i]));
}

void enum_reachset::Renumber(const long* ren)
{
  if (0==ren) return;
  for (long i=0; i<states->NumValues(); i++) {
    model_enum_value* st = states->GetValue(i);
    st->SetIndex(ren[i]);
  }
}

// ******************************************************************
// *                                                                *
// *              enum_reachset::natural_iter  methods              *
// *                                                                *
// ******************************************************************

enum_reachset::natural_iter::natural_iter(const model_enum &ss) 
 : indexed_iterator(ss.NumValues()), states(ss)
{
}

enum_reachset::natural_iter::~natural_iter()
{
}

void enum_reachset::natural_iter::copyState(shared_state* st, long o) const
{
  st->set(states.GetIndex(), ord2index(o));
}

// ******************************************************************
// *                                                                *
// *              enum_reachset::lexical_iter  methods              *
// *                                                                *
// ******************************************************************

enum_reachset::lexical_iter::lexical_iter(const model_enum &ss) 
 : natural_iter(ss)
{
  long* M = new long[ss.NumValues()];
  ss.MakeSortedMap(M);
  for (long i=0; i<ss.NumValues(); i++) {
    const model_enum_value* which = ss.GetValue(M[i]);
    M[i] = which->GetIndex();
  }
  setMap(M);
}

enum_reachset::lexical_iter::~lexical_iter()
{
}

// ******************************************************************
// *                                                                *
// *             enum_reachset::discovery_iter  methods             *
// *                                                                *
// ******************************************************************

enum_reachset::discovery_iter::discovery_iter(const model_enum &ss) 
 : natural_iter(ss)
{
  long* M = new long[ss.NumValues()];
  for (long i=0; i<ss.NumValues(); i++) {
    const model_enum_value* which = ss.GetValue(i);
    M[i] = which->GetIndex();
  }
  setMap(M);
}

enum_reachset::discovery_iter::~discovery_iter()
{
}

