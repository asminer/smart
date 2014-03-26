
// $Id$

#include "mc_llm.h"
#include "rss_mdd.h"
#include "../ExprLib/mod_vars.h"

#include "../Modules/glue_meddly.h"
#include "../Modules/statesets.h"

#include "mc_mdd.h"

#include "../include/lslib.h"
#include "../include/meddly_expert.h"

// #define DEBUG_DIAGONAL

// ******************************************************************
// *                                                                *
// *                        meddly_mc  class                        *
// *                                                                *
// ******************************************************************

class meddly_mc : public markov_lldsm {
  class LS_wrapper : public LS_Abstract_Matrix {
    double* diagonals;
    MEDDLY::numerical_operation* vectmult;
  public:
    LS_wrapper(MEDDLY::numerical_operation* vm, double* d, long dsize);
    virtual ~LS_wrapper();
    virtual bool IsTransposed() const {
      return false; // not sure about this
    }
    virtual long SolveRow(long, const float*, double &) const {
      return -1;
    }
    virtual long SolveRow(long, const double*, double &) const {
      return -1;
    }
    virtual void NoDiag_MultByRows(const float* x, double* y) const {
      DCASSERT(0);
    }
    virtual void NoDiag_MultByRows(const double* x, double* y) const {
      DCASSERT(vectmult);
      vectmult->compute(y, x);
    };
    virtual void NoDiag_MultByCols(const float* x, double* y) const {
      DCASSERT(0);
    }
    virtual void NoDiag_MultByCols(const double* x, double* y) const {
      DCASSERT(0);
    };
    virtual void DivideDiag(double* x, double scalar) const {
      for (long i=size-1; i>=0; i--) {
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

  // Required for a useful "checkable_lldsm":
  virtual long getNumStates(bool show) const;
  virtual void getNumStates(result& count) const;
  virtual void getReachable(result &ss) const;
  virtual void getPotential(expr* p, result &ss) const;
  virtual void getInitialStates(result &x) const;

  virtual void findDeadlockedStates(stateset &) const;

  virtual long getNumArcs(bool show) const;

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
::LS_wrapper(MEDDLY::numerical_operation* vm, double* d, long ds)
 : LS_Abstract_Matrix(ds)
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
  DCASSERT(process->vars);
  DCASSERT(process->mdd_wrap);
  MEDDLY::forest* f = process->vars->createForest(
    false, MEDDLY::forest::INTEGER, MEDDLY::forest::MULTI_TERMINAL
  );
  mtmdd_wrap = process->mdd_wrap->copyWithDifferentForest("mtmdd", f);
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
  dsize = getNumStates(false);
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

  MEDDLY::numerical_operation* MV = MEDDLY::MATR_EXPLVECT_MULT->buildOperation(
    process->state_indexes->E, process->proc->E, process->state_indexes->E
  );
  MV->compute(diagonals, one);
  MEDDLY::destroyOperation(MV);
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
  DCASSERT(process->states);
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

long meddly_mc::getNumStates(bool show) const
{
  DCASSERT(process);
  return process->getNumStates(this, em->cout(), show);
}

void meddly_mc::getNumStates(result &count) const
{
  DCASSERT(process);
  process->getNumStates(count);
}

void meddly_mc::getReachable(result &x) const
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

void meddly_mc::getPotential(expr* p, result &ss) const
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

void meddly_mc::getInitialStates(result &x) const
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

void meddly_mc::findDeadlockedStates(stateset &p) const
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

long meddly_mc::getNumArcs(bool show) const
{
  DCASSERT(process);
  DCASSERT(process->mxd_wrap);

  long ns;
  process->mdd_wrap->getCardinality(process->states, ns);

  long na = -1;
  if (process->proc_uses_actual) {
    process->proc_wrap->getCardinality(process->proc, na);
  } else {
    shared_object* actual = 0;
    try {
      actual = buildActualProc(process->proc);
      DCASSERT(actual);
      process->proc_wrap->getCardinality(actual, na);
    }
    catch (sv_encoder::error e) {
      if (GetParent()->StartError(0)) {
        em->cerr() << "Couldn't build actual edges: ";
        em->cerr() << sv_encoder::getNameOfError(e);
        GetParent()->DoneError();
      }
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
    if (display_graph_node_names) {
      em->cout() << "From ";
      process->mdd_wrap->minterm2state(fmt, fst); // TBD: check error code
      fst->Print(em->cout(), 0);
    } else {
      em->cout() << "Row " << fc;
    }
    em->cout() << ":\n";
    em->cout().Check();

    //
    // Iterate directly over the selected row
    //

    process->proc->startIteratorRow(fmt);
    for (; !process->proc->isIterDone(); process->proc->incIter() ) {
      const int* tmt = process->proc->getIterPrimedMinterm();
      em->cout() << "\t";
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
      float rate;
      process->proc->getForest()->evaluate(process->proc->E, fmt, tmt, rate);
      em->cout() << " : " << rate << "\n";
      em->cout().Check();
      count_na++;
    }
  }
  process->proc->freeIterator();
  process->states->freeIterator();
  Delete(fst);
  Delete(tst);
  DCASSERT(count_na == na);
  em->cout().flush();
  
  return na;
}

bool meddly_mc::computeSteadyState(double* probs) const
{
  for (long i=dsize-1; i>=0; i--) probs[i] = 1;
  startSteadyReport();
  LS_Output outdata;
  MEDDLY::numerical_operation* vm = MEDDLY::EXPLVECT_MATR_MULT->buildOperation(
    process->state_indexes->E, process->proc->E, process->state_indexes->E
  );
  LS_wrapper foo(vm, diagonals, dsize);
  Solve_AxZero(&foo, probs, getSolverOptions(), outdata);
  stopSteadyReport(outdata.num_iters);
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
