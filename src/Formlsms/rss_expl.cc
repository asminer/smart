
// $Id$

#include <cstdio>
#include "rss_expl.h"
#include "../ExprLib/exprman.h"
#include "../ExprLib/mod_vars.h"

// For ordering states
#include "../include/heap.h"
#include "../Modules/expl_states.h"

// ******************************************************************
// *                                                                *
// *                     expl_reachset  methods                     *
// *                                                                *
// ******************************************************************

expl_reachset::expl_reachset(StateLib::state_db* ss)
{
  state_dictionary = ss;
  state_dictionary->ConvertToStatic(true);
  state_handle = 0;
  state_collection = 0;

  DCASSERT(state_dictionary);
  natorder = new natural_db_iter(*state_dictionary);
  needs_discorder = false;
  discorder = 0;
  lexorder = 0;
}

expl_reachset::~expl_reachset()
{
  delete state_dictionary;
  delete[] state_handle;
  delete state_collection;

  delete natorder;
  delete discorder;
  delete lexorder;
}

StateLib::state_db* expl_reachset::getStateDatabase() const
{
  return state_dictionary;
}

void expl_reachset::getNumStates(long &ns) const
{
  if (state_dictionary) {
    ns = state_dictionary->Size();
  } else {
    DCASSERT(state_collection);
    ns = state_collection->Size();
  }
}

void expl_reachset::showInternal(OutputStream &os) const
{
  long ns;
  getNumStates(ns);
  for (long i=0; i<ns; i++) {
    long bytes = 0;
    const unsigned char* ptr = 0;
    os << "State " << i << " internal:";
    if (state_dictionary) {
      ptr = state_dictionary->GetRawState(i, bytes);
    } else {
      DCASSERT(state_collection);
      DCASSERT(state_handle);
      ptr = state_collection->GetRawState(state_handle[i], bytes);
    }
    for (long b=0; b<bytes; b++) {
      os.Put(' ');
      os.PutHex(ptr[b]);
    }
    os.Put('\n');
    os.flush();
  }
}

void expl_reachset::showState(OutputStream &os, const shared_state* st) const
{
  st->Print(os, 0);
}

state_lldsm::reachset::iterator& expl_reachset
::iteratorForOrder(state_lldsm::display_order ord)
{
  DCASSERT(state_dictionary || (state_collection && state_handle));

  switch (ord) {
    case state_lldsm::DISCOVERY:
      if (needs_discorder) {
        if (0==discorder) {
          discorder = new discovery_coll_iter(*state_collection, state_handle);
        }
        return *discorder;
      }
      DCASSERT(natorder);
      return *natorder;

    case state_lldsm::LEXICAL:
      if (0==lexorder) {
        if (state_dictionary) {
          lexorder = new lexical_db_iter(getGrandParent(), *state_dictionary);
        } else {
          lexorder = new lexical_coll_iter(getGrandParent(), *state_collection, state_handle);
        }
      }
      return *lexorder;
      
    case state_lldsm::NATURAL:
    default:
      DCASSERT(natorder);
      return *natorder;
  };
}

state_lldsm::reachset::iterator& expl_reachset::easiestIterator() const
{
  DCASSERT(natorder);
  return *natorder;
}

void expl_reachset::Finish()
{
  // Compact states
  state_collection = state_dictionary->TakeStateCollection();
  delete state_dictionary;
  state_dictionary = 0;
  state_handle = state_collection->RemoveIndexHandles();

  // TBD - renumbering?
  // TBD - needs_discorder becomes true if state_handle array is not monotone increasing

  // Update iterators
  DCASSERT(state_collection);
  DCASSERT(state_handle);
  delete natorder;
  natorder = new natural_coll_iter(*state_collection, state_handle);

  delete lexorder;
  lexorder = 0;
  delete discorder;
  discorder = 0;
}

// ******************************************************************
// *                                                                *
// *               expl_reachset::db_iterator methods               *
// *                                                                *
// ******************************************************************

expl_reachset::db_iterator::db_iterator(const StateLib::state_db &s)
 : indexed_iterator(s.Size()), states(s)
{
}

expl_reachset::db_iterator::~db_iterator()
{
}

void expl_reachset::db_iterator::copyState(shared_state* st, long o) const
{
  states.GetStateKnown(ord2index(o), st->writeState(), st->getStateSize());
}

// ******************************************************************
// *                                                                *
// *              expl_reachset::coll_iterator methods              *
// *                                                                *
// ******************************************************************

expl_reachset::coll_iterator::coll_iterator(const StateLib::state_coll &SC,
  const long* SH) : indexed_iterator(SC.Size()), states(SC), state_handle(SH)
{
  DCASSERT(state_handle);
}

expl_reachset::coll_iterator::~coll_iterator()
{
}

void expl_reachset::coll_iterator::copyState(shared_state* st, long o) const
{
  states.GetStateKnown(state_handle[ord2index(o)], st->writeState(), st->getStateSize());
}

// ******************************************************************
// *                                                                *
// *             expl_reachset::natural_db_iter methods             *
// *                                                                *
// ******************************************************************

expl_reachset::natural_db_iter::natural_db_iter(const StateLib::state_db &s)
 : db_iterator(s)
{
  // nothing!
}

// ******************************************************************
// *                                                                *
// *            expl_reachset::natural_coll_iter methods            *
// *                                                                *
// ******************************************************************

expl_reachset::natural_coll_iter::natural_coll_iter(const StateLib::state_coll &SC,
  const long* SH) : coll_iterator(SC, SH)
{
  // nothing!
}

// ******************************************************************
// *                                                                *
// *           expl_reachset::discovery_coll_iter methods           *
// *                                                                *
// ******************************************************************

expl_reachset::discovery_coll_iter::discovery_coll_iter(const StateLib::state_coll &SC,
  const long* SH) : coll_iterator(SC, SH)
{
  long* M = new long[SC.Size()];
  DCASSERT(SH);
  HeapSort(SH, M, SC.Size());
  setMap(M);
}

// ******************************************************************
// *                                                                *
// *             expl_reachset::lexical_db_iter methods             *
// *                                                                *
// ******************************************************************

expl_reachset::lexical_db_iter::lexical_db_iter(const hldsm* hm, 
  const StateLib::state_db &s) : db_iterator(s)
{
  long* M = new long[s.Size()];
  const StateLib::state_coll* coll = s.GetStateCollection();
  LexicalSort(hm, coll, M);
  setMap(M);
}

// ******************************************************************
// *                                                                *
// *            expl_reachset::lexical_coll_iter methods            *
// *                                                                *
// ******************************************************************

expl_reachset::lexical_coll_iter::lexical_coll_iter(const hldsm* hm, 
  const StateLib::state_coll &SC, const long* SH) : coll_iterator(SC, SH)
{
  long* M = new long[SC.Size()];
  DCASSERT(SH);
  LexicalSort(hm, &SC, SH, M);
  setMap(M);
}

