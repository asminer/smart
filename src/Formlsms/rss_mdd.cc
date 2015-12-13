
// $Id$

#include "rss_mdd.h"

#define DSDE_HLM_DETAILS
#include "dsde_hlm.h"

// #define DEBUG_INDEXSET

// #define DUMP_PROCESS

// ******************************************************************
// *                                                                *
// *                     meddly_states  methods                     *
// *                                                                *
// ******************************************************************

meddly_states::meddly_states() : shared_object()
{
  vars = 0;
  mdd_wrap = 0;
  initial = 0;
  states = 0;
  index_wrap = 0;
  state_indexes = 0;
  mxd_wrap = 0;
  nsf = 0;
  proc_wrap = 0;
  proc = 0;
  proc_uses_actual = 0;
}

meddly_states::~meddly_states()
{
  // These will most likely already be null
  Delete(initial);
  Delete(states);
  Delete(state_indexes);
  Delete(nsf);
  Delete(proc);
  Delete(mdd_wrap);
  Delete(index_wrap);
  Delete(mxd_wrap);
  Delete(proc_wrap);
  MEDDLY::destroyDomain(vars);
}

/*
bool meddly_states::createVarsBottomUp(const dsde_hlm &m, int* b, int nl)
{
  try {
    vars = MEDDLY::createDomainBottomUp(b, nl);
    return true;
  }
  catch (MEDDLY::error de) {
    if (m.StartError(0)) {
      m.SendError("Error creating domain: ");
      m.SendError(de.getName());
      m.DoneError();
    }
    return false;
  }
}
*/

bool meddly_states::createVars(const dsde_hlm &m, MEDDLY::variable** v, int nv)
{
  try {
    vars = MEDDLY::createDomain(v, nv);
    return true;
  }
  catch (MEDDLY::error de) {
    if (m.StartError(0)) {
      m.SendError("Error creating domain: ");
      m.SendError(de.getName());
      m.DoneError();
    }
    return false;
  }
}

bool meddly_states::Print(OutputStream &s, int) const
{
  // not sure if we need this
  s << "{meddly states struct}";
  return true;
}

bool meddly_states::Equals(const shared_object* o) const
{
  DCASSERT(0);
  return o == this;
}

void meddly_states::showStates(const lldsm* m, OutputStream &cout, bool internal)
{

  DCASSERT(m);
  DCASSERT(mdd_wrap);
  DCASSERT(states);

  if (internal) {
    cout << "Internal state space representation (using MEDDLY):\n";
    mdd_wrap->showNodeGraph(cout, states);
    cout.flush();
    return;
  }

  long ns;
  mdd_wrap->getCardinality(states, ns);
  if (m->tooManyStates(ns, true)) return;


  // Natural order
  // TBD: Lexical order (use index set, build index mapping)
  
  shared_state* st = new shared_state(m->GetParent());
  long c;
  states->startIterator();
  for (c = 0; !states->isIterDone(); states->incIter(), ++c) {
    const int* minterm = states->getIterMinterm();
    mdd_wrap->minterm2state(minterm, st); // TBD: check error code
    cout << "State " << c << ": ";
    st->Print(cout, 0);
    cout << "\n";
    cout.flush();
  }
  states->freeIterator();
  Delete(st);
}

void meddly_states
::showArcs(const graph_lldsm* m, OutputStream &cout, bool internal, bool node_names)
{
  if (internal) {
    DCASSERT(proc_wrap);
    DCASSERT(proc);

    cout << "Internal process representation (using MEDDLY):\n";
    proc_wrap->showNodeGraph(cout, proc);
    cout.flush();

    return;
  }
  long ns;
  getNumStates(ns);
  if (ns<0) return;

  long na;
  getNumArcs(na);
  if (na<0) return;

  if (m->tooManyStates(ns, true))  return;
  if (m->tooManyArcs(na, true))    return;

  if (!node_names) {
    buildIndexSet();
    DCASSERT(index_wrap);
    DCASSERT(state_indexes);
  }

  long count_na = 0;
  shared_state* fst = new shared_state(m->GetParent());
  shared_state* tst = new shared_state(m->GetParent());
  long fc;
  states->startIterator();
  for (fc = 0; 
        !states->isIterDone(); 
        states->incIter(), ++fc) 
  {
    const int* fmt = states->getIterMinterm();
    cout << "From state ";
    if (node_names) {
      MddMinterm2State(fmt, fst);
      fst->Print(cout, 0);
    } else {
      cout << fc;
    }
    cout << ":\n";
    cout.flush();

    //
    // Iterate directly over the selected row
    //

    proc->startIteratorRow(fmt);
    for (; !proc->isIterDone(); proc->incIter() ) {
      const int* tmt = proc->getIterPrimedMinterm();
      cout << "\tTo state ";
      if (node_names) {
        MddMinterm2State(tmt, tst);
        tst->Print(cout, 0);
      } else {
        int index;
        index_wrap->getForest()->evaluate(
            state_indexes->E, tmt, index
        );
        cout << index;
      }

      if (proc_wrap->isTypeReal()) {
        float rate;
        proc->getForest()->evaluate(proc->E, fmt, tmt, rate);
        cout << " : " << rate;
      }

      cout << "\n";
      cout.flush();
      count_na++;
    }

  } // for fc

  proc->freeIterator();
  states->freeIterator();
  Delete(fst);
  Delete(tst);
  DCASSERT(na == count_na);
  cout.flush();
}

void meddly_states::buildIndexSet()
{
  if (state_indexes)  return;
  if (0==states)      return;

  DCASSERT(0==index_wrap);
  DCASSERT(vars);

  MEDDLY::forest* evF = vars->createForest(
    false, MEDDLY::forest::INTEGER, MEDDLY::forest::INDEX_SET
  );
  DCASSERT(evF);

  index_wrap = mdd_wrap->copyWithDifferentForest("EV+MDD", evF);
  DCASSERT(index_wrap);

  state_indexes = new shared_ddedge(evF);

  MEDDLY::apply(
    MEDDLY::CONVERT_TO_INDEX_SET, states->E, state_indexes->E
  );

#ifdef DEBUG_INDEXSET
  printf("Built index set:\n");
  state_indexes->E.show(stdout, 2);
#endif
}

void meddly_states::visitStates(lldsm::state_visitor &x) const 
{
  states->startIterator();
  for (x.index()=0; !states->isIterDone(); states->incIter(), x.index()++) {
    if (x.canSkipIndex()) continue;
    const int* minterm = states->getIterMinterm();
    mdd_wrap->minterm2state(minterm, x.state());
    if (x.visit()) break;
  }
  states->freeIterator();
}

void meddly_states::reportStats(OutputStream &out) const
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
  if (nsf) {
    long nc = nsf->E.getNodeCount();
    out << "\t    Edge relation  requires " << nc << " nodes\n";
  }
  if (proc) {
    long nc = proc->E.getNodeCount();
    out << "\t    Stoch. process requires " << nc << " nodes\n";
#ifdef DUMP_PROCESS
    DisplayStream &ds = dynamic_cast<DisplayStream&>(out);
    ds.flush();
    proc->E.show(ds.getDisplay(), 2);
#endif
  }
  if (mdd_wrap)   mdd_wrap->reportStats(out);
  if (index_wrap) index_wrap->reportStats(out);
  if (mxd_wrap)   mxd_wrap->reportStats(out);
  if (proc_wrap)  proc_wrap->reportStats(out);
}

