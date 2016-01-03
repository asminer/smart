
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

  inline shared_ddedge* buildActualNSF() const {
    DCASSERT(process);
    return process->buildActualNSF();
  };

  inline void finish() {
    DCASSERT(false==is_finished);
    is_finished = true;
  }

  virtual void visitStates(state_visitor &x) const;

  // Required for a useful "checkable_lldsm":
  virtual void getNumStates(result& count) const;
  virtual long getNumStates() const;
  virtual void showStates(bool internal) const;
  virtual void getReachable(result &ss) const;
  virtual void getPotential(expr* p, result &ss) const;
  virtual void getInitialStates(result &x) const;

  virtual void findDeadlockedStates(stateset &) const;

  virtual long getNumArcs() const;
  virtual void getNumArcs(result& count) const;
  virtual void showArcs(bool internal) const;
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
  MEDDLY::forest* f = process->createForest(
    false, MEDDLY::forest::INTEGER, MEDDLY::forest::MULTI_TERMINAL
  );
  mtmdd_wrap = process->copyMddWrapperWithDifferentForest("MTMDD", f);
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

void meddly_fsm::getNumStates(result &count) const
{
  DCASSERT(process);
  process->getNumStates(count);
}

long meddly_fsm::getNumStates() const
{
  DCASSERT(process);
  long count;
  process->getNumStates(count);
  return count;
}

void meddly_fsm::showStates(bool internal) const
{
  DCASSERT(process);
  return process->showStates(this, em->cout(), internal);
}

void meddly_fsm::getReachable(result &x) const
{
  DCASSERT(is_finished);
  if (0==process || !process->hasMddWrapper() || !process->hasStates()) {
    x.setNull();
    return;
  }
  x.setPtr(
    new stateset(this, process->shareMddWrap(), process->shareStates(), 
                       process->shareMxdWrap(), process->shareNSF()
                )
  );
}

void meddly_fsm::getPotential(expr* p, result &ss) const
{
  DCASSERT(is_finished);
  if (0==p || 0==process || !process->hasMddWrapper()) {
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
  shared_ddedge* ans = process->newMddEdge();
  MEDDLY::apply(MEDDLY::COPY, mtans->E, ans->E);

  // This should clobber the MTMDD
  ss.setPtr( new stateset(this, process->shareMddWrap(), ans,
                                process->shareMxdWrap(), process->shareNSF()
                 ) 
  );
}

void meddly_fsm::getInitialStates(result &x) const
{
  DCASSERT(is_finished);
  if (0==process || !process->hasMddWrapper() || !process->hasInitial()) {
    x.setNull();
    return;
  }
  x.setPtr(
    new stateset(this, process->shareMddWrap(), process->shareInitial(),
                       process->shareMxdWrap(), process->shareNSF()
    )
  );
}

void meddly_fsm::findDeadlockedStates(stateset &p) const
{
  DCASSERT(process);

  MEDDLY::forest* f = process->getMddForest();
  DCASSERT(f);

  MEDDLY::dd_edge one(f), live(f);

  f->createEdge(true, one);

  // EX true gives all "live" states
  MEDDLY::apply(MEDDLY::PRE_IMAGE, one, process->getNSF(), live);

  // Subtract live states from p
  shared_ddedge* pse = smart_cast <shared_ddedge*> (p.changeStateDD());
  DCASSERT(pse);
  pse->E -= live;
}

long meddly_fsm::getNumArcs() const
{
  DCASSERT(process);

  long na = -1;
  try {
    process->getNumArcs(na);
  }
  catch (sv_encoder::error e) {
    if (GetParent()->StartError(0)) {
      em->cerr() << "Couldn't count/build actual edges: ";
      em->cerr() << sv_encoder::getNameOfError(e);
      GetParent()->DoneError();
    }
  }

  return na;
}

void meddly_fsm::getNumArcs(result &count) const
{
  DCASSERT(process);

  count.setNull();

  try {
    process->getNumArcs(count);
  }
  catch (sv_encoder::error e) {
    if (GetParent()->StartError(0)) {
      em->cerr() << "Couldn't count/build actual edges: ";
      em->cerr() << sv_encoder::getNameOfError(e);
      GetParent()->DoneError();
    }
  }
}


void meddly_fsm::showArcs(bool internal) const
{
  DCASSERT(process);
  try {
    process->showArcs(this, em->cout(), internal, displayGraphNodeNames());
  }
  catch (sv_encoder::error e) {
    if (GetParent()->StartError(0)) {
      em->cerr() << "Couldn't count/build/show actual edges: ";
      em->cerr() << sv_encoder::getNameOfError(e);
      GetParent()->DoneError();
    }
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

void FinishMeddlyFSM(lldsm* fsm, bool pot)
{
  meddly_fsm* mddfsm = dynamic_cast <meddly_fsm*> (fsm);
  if (0==mddfsm) return;
  meddly_states* ss = mddfsm->grabStates();
  if (ss->proc) {
    mddfsm->finish();
    return;
  }
  ss->proc_wrap = Share(ss->mxd_wrap);
  if (pot) {
    ss->proc = Share(ss->nsf);
    ss->proc_uses_actual = false;
  } else {
    try {
      ss->proc = mddfsm->buildActualNSF();
      ss->proc_uses_actual = true;
    }
    catch (sv_encoder::error e) {
      // use potential instead
      Delete(ss->proc);
      ss->proc = Share(ss->nsf);
      ss->proc_uses_actual = false;
    } 
  }
  mddfsm->finish();
}
