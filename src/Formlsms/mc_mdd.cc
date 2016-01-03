
// $Id$

#include "mc_llm.h"
#include "rss_mdd.h"
#include "../ExprLib/mod_vars.h"

#include "../Modules/glue_meddly.h"
#include "../Modules/statesets.h"

#include "mc_mdd.h"
#include "timerlib.h"

#include "../include/lslib.h"
#include "../include/meddly_expert.h"

// #define DEBUG_DIAGONAL

// ******************************************************************
// *                                                                *
// *                        meddly_mc  class                        *
// *                                                                *
// ******************************************************************

class meddly_mc : public markov_lldsm {
  class LS_wrapper : public LS_Generic_Matrix {
    double* diagonals;
    MEDDLY::specialized_operation* vectmult;
  public:
    LS_wrapper(MEDDLY::specialized_operation* vm, double* d, long dsize);
    virtual ~LS_wrapper();
    virtual void MatrixVectorMultiply(double* y, const float* x) const {
      throw LS_Not_Implemented;
    }
    virtual void MatrixVectorMultiply(double* y, const double* x) const {
      DCASSERT(vectmult);
      vectmult->compute(y, x);
    };
    virtual void DivideDiag(double* x) const {
      for (long i=Size()-1; i>=0; i--) {
        x[i] /= diagonals[i];
      }
    }
    virtual void DivideDiag(double* x, double scalar) const {
      for (long i=Size()-1; i>=0; i--) {
        x[i] *= scalar / diagonals[i];
      }
    };
  };

  bool is_finished;
  double* diagonals;
  long dsize;
  // For getPotential():
  meddly_encoder* mtmdd_wrap;
protected:
  meddly_states* process;
public:
  meddly_mc(meddly_states* ss);
protected:
  virtual ~meddly_mc();
  const char* getClassName() const { return "meddly_mc"; }
  void buildDiagonals();
public:
  inline meddly_states*     grabStates() { return process; }

  inline void finish() {
    DCASSERT(false==is_finished);
    is_finished = true;
    buildDiagonals();
  }

  shared_ddedge* buildActualProc(const shared_ddedge* p) const;

  virtual void visitStates(state_visitor &x) const;

  // Required for a useful "graph_lldsm":
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

  // Numerical solutions:
  virtual bool computeSteadyState(double* probs) const;

  inline bool statusOK(const LS_Output &o, const char* who) const {
    switch (o.status) {
      case LS_Success:
          return true;

      case LS_No_Convergence:
          if (em->startWarning()) {
            em->noCause();
            em->warn() << "Markov chain linear solver (for ";
            em->warn() << who << ") did not converge";
            em->stopIO();
          }
          return true;

      case LS_Out_Of_Memory:
          if (em->startError()) {
            em->noCause();
            em->cerr() << "Insufficient memory for Markov chain ";
            em->cerr() << who << " solver";
            em->stopIO();
          }
          return false;

      case LS_Wrong_Format:
          if (em->startError()) {
            em->noCause();
            em->cerr() << "Wrong matrix format for Markov chain linear solver";
            em->stopIO();
          }
          return false;

      default:
          if (em->startInternal(__FILE__, __LINE__)) {
            em->noCause();
            em->internal() << "Unexpected error.";
            em->stopIO();
          }
    } // switch
    return false;
  }
};

// ******************************************************************
// *                       LS_wrapper methods                       *
// ******************************************************************

meddly_mc::LS_wrapper
::LS_wrapper(MEDDLY::specialized_operation* vm, double* d, long ds)
 : LS_Generic_Matrix(0, ds, ds)
{
  vectmult = vm;
  diagonals = d;
}

meddly_mc::LS_wrapper::~LS_wrapper()
{
}

// ******************************************************************
// *                       meddly_mc  methods                       *
// ******************************************************************

meddly_mc::meddly_mc(meddly_states* ss)
: markov_lldsm(false) // TBD what about dtmcs?
{
  process = ss;
  is_finished = false;
  // Build scratch space
  MEDDLY::forest* f = process->createForest(
    false, MEDDLY::forest::INTEGER, MEDDLY::forest::MULTI_TERMINAL
  );
  mtmdd_wrap = process->copyMddWrapperWithDifferentForest("mtmdd", f);
  DCASSERT(mtmdd_wrap);
  diagonals = 0;
}

meddly_mc::~meddly_mc()
{
  Delete(mtmdd_wrap);
  Delete(process);
  delete[] diagonals;
}

void meddly_mc::buildDiagonals()
{
  DCASSERT(0==diagonals);
  dsize = getNumStates();
  if (dsize <= 0) return;
#ifdef DEBUG_DIAGONAL
  fprintf(stderr, "Building diagonals for %ld states\n", dsize);
#endif

  process->buildIndexSet();
  DCASSERT(process->index_wrap);
  DCASSERT(process->state_indexes);
  DCASSERT(process->proc_wrap);
  DCASSERT(process->proc);

  diagonals = new double[dsize];
  double* one = new double[dsize];
  for (int i=dsize-1; i>=0; i--) {
    diagonals[i] = 0;
    one[i] = 1;
  }

  try {
    MEDDLY::specialized_operation* MV;
    MV = MEDDLY::MATR_EXPLVECT_MULT->buildOperation(
      process->state_indexes->E, process->proc->E, process->state_indexes->E
    );
    MV->compute(diagonals, one);
    MEDDLY::destroyOperation(MV);
  }
  catch (MEDDLY::error e) {
    if (GetParent()->StartError(0)) {
      em->cerr() << "Couldn't build diagonals: ";
      em->cerr() << e.getName();
      GetParent()->DoneError();
    }

  }
  delete[] one;

#ifdef DEBUG_DIAGONAL
  fprintf(stderr, "Done, got:\n[%lf", diagonals[0]);
  for (int i=1; i<dsize; i++) {
    fprintf(stderr, ", %lf", diagonals[i]);
  }
  fprintf(stderr, "]\n");
#endif
}

shared_ddedge* 
meddly_mc::buildActualProc(const shared_ddedge* p) const
{
  if (0==p) return 0;
  DCASSERT(process);
  DCASSERT(process->proc_wrap);
  // strip off unreachable edges from process 
  shared_ddedge* actual = new shared_ddedge(process->proc_wrap->getForest());
  try {
    process->mxd_wrap->selectRows(process->proc, process->states, actual);
  }
  catch (sv_encoder::error e) {
    Delete(actual);
    return 0;
  }
  return actual;
}

void meddly_mc::visitStates(state_visitor &x) const
{
  DCASSERT(process);
  process->visitStates(x);
}

long meddly_mc::getNumStates() const
{
  DCASSERT(process);
  long count;
  process->getNumStates(count);
  return count;
}

void meddly_mc::getNumStates(result &count) const
{
  DCASSERT(process);
  process->getNumStates(count);
}

void meddly_mc::showStates(bool internal) const
{
  DCASSERT(process);
  return process->showStates(this, em->cout(), internal);
}

void meddly_mc::getReachable(result &x) const
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

void meddly_mc::getPotential(expr* p, result &ss) const
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

void meddly_mc::getInitialStates(result &x) const
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

void meddly_mc::findDeadlockedStates(stateset &p) const
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

long meddly_mc::getNumArcs() const
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

void meddly_mc::getNumArcs(result &count) const
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


void meddly_mc::showArcs(bool internal) const
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

bool meddly_mc::computeSteadyState(double* probs) const
{
  for (long i=dsize-1; i>=0; i--) probs[i] = 1;
  timer w;
  startSteadyReport(w);
  LS_Output outdata;
  MEDDLY::specialized_operation* vm;
  vm = MEDDLY::EXPLVECT_MATR_MULT->buildOperation(
    process->state_indexes->E, process->proc->E, process->state_indexes->E
  );
  LS_wrapper foo(vm, diagonals, dsize);
  Solve_AxZero(foo, probs, getSolverOptions(), outdata);
  stopSteadyReport(w, outdata.num_iters);
  MEDDLY::destroyOperation(vm);
  return statusOK(outdata, "steady-state");
}

// ******************************************************************
// *                                                                *
// *                           Front  end                           *
// *                                                                *
// ******************************************************************

markov_lldsm* StartMeddlyMC(meddly_states* ss)
{
  if (0==ss) return 0;
  return new meddly_mc(ss);
}

meddly_states* GrabMeddlyMCStates(lldsm* mc)
{
  meddly_mc* mddmc = dynamic_cast <meddly_mc*> (mc);
  if (0==mddmc) return 0;
  return mddmc->grabStates();
}

void FinishMeddlyMC(lldsm* mc, bool pot)
{
  meddly_mc* mddmc = dynamic_cast <meddly_mc*> (mc);
  if (0==mddmc) return;
  meddly_states* ss = mddmc->grabStates();
  shared_ddedge* potproc = Share(ss->proc);
  if (pot) {
    ss->proc_uses_actual = false;
  } else {
    Delete(ss->proc);
    try {
      ss->proc = mddmc->buildActualProc(potproc);
      ss->proc_uses_actual = true;
    }
    catch (sv_encoder::error e) {
      // use potential instead
      ss->proc = Share(potproc);
      ss->proc_uses_actual = false;
    }
  }
  Delete(potproc);
  mddmc->finish();
}
