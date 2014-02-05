
// $Id$

#include <limits.h>

#include "check_llm.h"
#include "rss_mdd.h"
#include "../ExprLib/mod_vars.h"

#include "../Modules/glue_meddly.h"
#include "../Modules/statesets.h"

#include "fsm_mdd.h"

// #define NO_ROW_ITERATORS

// ******************************************************************
// *                                                                *
// *                        meddly_fsm class                        *
// *                                                                *
// ******************************************************************

class meddly_fsm : public checkable_lldsm {
  bool is_finished;
  // For getPotential():
  meddly_encoder* mtmdd_wrap;
protected:
  meddly_states* process;
public:
  meddly_fsm(meddly_states* ss);
protected:
  virtual ~meddly_fsm();
  const char* getClassName() const { return "meddly_fsm"; }
public:
  inline meddly_states*     grabStates() { return process; }

  inline shared_ddedge* buildActualNSF(sv_encoder::error &e) const {
    DCASSERT(process);
    DCASSERT(process->mxd_wrap);
    DCASSERT(process->nsf);
    DCASSERT(process->states);
    shared_ddedge* actual = new shared_ddedge(process->mxd_wrap->getForest());
    e = process->mxd_wrap->selectRows(process->nsf, process->states, actual);
    return actual;
  };

  inline void finish() {
    DCASSERT(false==is_finished);
    is_finished = true;
  }

  virtual void visitStates(state_visitor &x) const;

  // Required for a useful "checkable_lldsm":
  virtual long getNumStates(bool show) const;
  virtual void getNumStates(result& count) const;
  virtual void getReachable(result &ss) const;
  virtual void getPotential(expr* p, result &ss) const;
  virtual void getInitialStates(result &x) const;

  virtual void findDeadlockedStates(stateset &) const;

  virtual long getNumArcs(bool show) const;
  virtual void getNumArcs(result& count) const;
};

// ******************************************************************
// *                       meddly_fsm methods                       *
// ******************************************************************

meddly_fsm::meddly_fsm(meddly_states* ss)
: checkable_lldsm(FSM)
{
  process = ss;
  is_finished = false;

  // Build scratch space
  DCASSERT(process->vars);
  DCASSERT(process->mdd_wrap);
  MEDDLY::forest* f = process->vars->createForest(
    false, MEDDLY::forest::INTEGER, MEDDLY::forest::MULTI_TERMINAL
  );
  mtmdd_wrap = process->mdd_wrap->copyWithDifferentForest("MTMDD", f);
  DCASSERT(mtmdd_wrap);
}

meddly_fsm::~meddly_fsm()
{
  Delete(mtmdd_wrap);
  Delete(process);
}

void meddly_fsm::visitStates(state_visitor &x) const
{
  DCASSERT(process);
  process->visitStates(x);
}

long meddly_fsm::getNumStates(bool show) const
{
  DCASSERT(process);
  return process->getNumStates(this, em->cout(), show);
}

void meddly_fsm::getNumStates(result &count) const
{
  DCASSERT(process);
  process->getNumStates(count);
}

void meddly_fsm::getReachable(result &x) const
{
  DCASSERT(is_finished);
  if (0==process || 0==process->mdd_wrap || 0==process->states) {
    x.setNull();
    return;
  }
  x.setPtr(
    new stateset(this, Share(process->mdd_wrap), Share(process->states),
                       Share(process->mxd_wrap), Share(process->nsf)
                )
  );
}

void meddly_fsm::getPotential(expr* p, result &ss) const
{
  DCASSERT(is_finished);
  if (0==p || 0==process || 0==process->mdd_wrap) {
    ss.setNull();
    return;
  }

  // Build expression as MTMDD
  traverse_data x(traverse_data::BuildDD);
  x.answer = &ss;
  x.ddlib = mtmdd_wrap;
  p->Traverse(x);
  if (!ss.isNormal()) return;
  shared_ddedge* mtans = smart_cast <shared_ddedge*> (ss.getPtr());
  DCASSERT(mtans);

  // copy into MDD
  shared_ddedge* ans = new shared_ddedge(process->mdd_wrap->getForest());
  MEDDLY::apply(MEDDLY::COPY, mtans->E, ans->E);

  // This should clobber the MTMDD
  ss.setPtr( new stateset(this, Share(process->mdd_wrap), ans,
                                Share(process->mxd_wrap), Share(process->nsf)
                 ) 
  );
}

void meddly_fsm::getInitialStates(result &x) const
{
  DCASSERT(is_finished);
  if (0==process || 0==process->mdd_wrap || 0==process->initial) {
    x.setNull();
    return;
  }
  x.setPtr(
    new stateset(this, Share(process->mdd_wrap), Share(process->initial),
                       Share(process->mxd_wrap), Share(process->nsf)
    )
  );
}

void meddly_fsm::findDeadlockedStates(stateset &p) const
{
  DCASSERT(process);
  DCASSERT(process->mdd_wrap);
  DCASSERT(process->mxd_wrap);

  MEDDLY::forest* f = process->mdd_wrap->getForest();
  DCASSERT(f);

  MEDDLY::dd_edge one(f), live(f);

  f->createEdge(true, one);

  // EX true gives all "live" states
  MEDDLY::apply(MEDDLY::PRE_IMAGE, one, process->nsf->E, live);

  // Subtract live states from p
  shared_ddedge* pse = smart_cast <shared_ddedge*> (p.changeStateDD());
  DCASSERT(pse);
  pse->E -= live;
}

long meddly_fsm::getNumArcs(bool show) const
{
  DCASSERT(process);
  DCASSERT(process->mxd_wrap);

  long ns;
  process->mdd_wrap->getCardinality(process->states, ns);

  long na = -1;
  if (process->proc_uses_actual) {
    process->proc_wrap->getCardinality(process->proc, na);
  } else {
    sv_encoder::error e;
    shared_object* actual = buildActualNSF(e);
    if (e) {
      if (GetParent()->StartError(0)) {
        em->cerr() << "Couldn't build actual edges: ";
        em->cerr() << sv_encoder::getNameOfError(e);
        GetParent()->DoneError();
      }
    } else {
      process->proc_wrap->getCardinality(actual, na);
    }
    Delete(actual);
  }

  if (!show)                    return na;
  if (tooManyStates(ns, show))  return na;
  if (tooManyArcs(na, show))    return na;

  if (!display_graph_node_names) {
    process->buildIndexSet();
    DCASSERT(process->index_wrap);
    DCASSERT(process->state_indexes);
  }

  long count_na = 0;
  shared_state* fst = new shared_state(parent);
  shared_state* tst = new shared_state(parent);
  long fc;
  process->states->startIterator();
  for (fc = 0; 
        !process->states->isIterDone(); 
        process->states->incIter(), ++fc) 
  {
    const int* fmt = process->states->getIterMinterm();
    em->cout() << "From state ";
    if (display_graph_node_names) {
      CHECK_RETURN(
        process->mdd_wrap->minterm2state(fmt, fst),
        sv_encoder::Success
      );
      fst->Print(em->cout(), 0);
    } else {
      em->cout() << fc;
    }
    em->cout() << ":\n";
    em->cout().Check();

    //
    // Iterate directly over the selected row
    //

    process->proc->startIteratorRow(fmt);
    for (; !process->proc->isIterDone(); process->proc->incIter() ) {
      const int* tmt = process->proc->getIterPrimedMinterm();
      em->cout() << "\tTo state ";
      if (display_graph_node_names) {
        process->mdd_wrap->minterm2state(tmt, tst);
        tst->Print(em->cout(), 0);
      } else {
        int index;
        process->index_wrap->getForest()->evaluate(
            process->state_indexes->E, tmt, index
        );
        em->cout() << index;
      }
      em->cout() << "\n";
      em->cout().Check();
      count_na++;
    }

  } // for fc

  process->proc->freeIterator();
  process->states->freeIterator();
  Delete(fst);
  Delete(tst);
  DCASSERT(na == count_na);
  em->cout().flush();
  
  return na;
}

void meddly_fsm::getNumArcs(result &count) const
{
  DCASSERT(process);
  DCASSERT(process->proc_wrap);
  process->proc_wrap->getCardinality(process->proc, count);

  if (process->proc_uses_actual) {
    process->proc_wrap->getCardinality(process->proc, count);
  } else {
    shared_object* actual = process->proc_wrap->makeEdge(0);
    DCASSERT(actual);
    sv_encoder::error e;
    e = process->proc_wrap->selectRows(process->proc, process->states, actual);
    if (e) {
      if (GetParent()->StartError(0)) {
        em->cerr() << "Couldn't build actual edges: ";
        em->cerr() << sv_encoder::getNameOfError(e);
        GetParent()->DoneError();
      }
      count.setNull();
    } else {
      process->proc_wrap->getCardinality(actual, count);
    }
    Delete(actual);
  }
}


// ******************************************************************
// *                                                                *
// *                           Front  end                           *
// *                                                                *
// ******************************************************************

checkable_lldsm* StartMeddlyFSM(meddly_states* ss)
{
  if (0==ss) return 0;
  return new meddly_fsm(ss);
}

meddly_states* GrabMeddlyFSMStates(lldsm* fsm)
{
  meddly_fsm* mddfsm = dynamic_cast <meddly_fsm*> (fsm);
  if (0==mddfsm) return 0;
  return mddfsm->grabStates();
}

sv_encoder::error FinishMeddlyFSM(lldsm* fsm, bool pot)
{
  meddly_fsm* mddfsm = dynamic_cast <meddly_fsm*> (fsm);
  sv_encoder::error e = sv_encoder::Success;
  if (0==mddfsm) return e;
  meddly_states* ss = mddfsm->grabStates();
  if (ss->proc) {
    mddfsm->finish();
    return e;
  }
  if (pot) {
    ss->proc = Share(ss->nsf);
    ss->proc_uses_actual = false;
  } else {
    ss->proc = mddfsm->buildActualNSF(e);
    if (e) {
      // use potential instead
      Delete(ss->proc);
      ss->proc = Share(ss->nsf);
      ss->proc_uses_actual = false;
    } else {
      ss->proc_uses_actual = true;
    }
  }
  mddfsm->finish();
  return e;
}
