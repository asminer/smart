
// $Id$

#include "exact.h"

#include "../Timers/timers.h"

#include "../ExprLib/exprman.h"
#include "../ExprLib/engine.h"
#include "../ExprLib/mod_inst.h"
#include "../ExprLib/mod_vars.h"
#include "../ExprLib/measures.h"

#include "../Formlsms/stoch_llm.h"
#include "../Formlsms/phase_hlm.h"

#include "../Modules/statevects.h"

// external

#include "intset.h"
#include "lslib.h"

// **************************************************************************
// *                                                                        *
// *                           exact_mcmsr  class                           *
// *                                                                        *
// **************************************************************************

class exact_mcmsr : public subengine {
  timer* w;
protected:
  static named_msg eng_debug;
  static named_msg eng_report;
  static engtype* ProcessGeneration;
  friend void InitializeExactSolutionEngines(exprman* em);
public:
  exact_mcmsr();
  virtual bool AppliesToModelType(hldsm::model_type) const;
protected:

  class msr_visitor : public lldsm::state_visitor {
  protected:
    measure *m;
    double* p;
    intset* infmask;
    result tmp;
    result ans;
  public:
    msr_visitor(const hldsm* mdl);
    inline void newMsr(measure* _m) {
      m = _m;
      m->PrecomputeRHS();
      ans.setReal(0);
    }
    inline void setVector(double* x, intset* mask) {
      p = x;
      infmask = mask;
    }
    inline void finish() {
      m->SetValue(ans);
    }
    virtual bool canSkipIndex() {
      return (0==p[x.current_state_index]);
    }

    inline bool infinitySubtract(measure* m) {
      if (em->startError()) {
        em->causedBy(m);
        em->cerr() << "Undefined operation (infty-infty) while computing ";
        em->cerr() << m->Name();
        em->stopIO();
      }
      ans.setNull();
      return true;
    }
  };

  class realmsr_visitor : public msr_visitor {
  public:
    realmsr_visitor(const hldsm* mdl);
    virtual bool visit();
  };

  class boolmsr_visitor : public msr_visitor {
  public:
    boolmsr_visitor(const hldsm* mdl);
    virtual bool visit();
  };


  void GenerateProc(hldsm* m) const;

  inline bool startMsrs(const char* what, const char* name) { 
    if (eng_report.startReport()) {
      eng_report.report() << "Computing ";
      if (what) eng_report.report() << what << " ";
      eng_report.report() << "measures";
      if (name) eng_report.report() << " for model " << name;
      eng_report.report().Put('\n');
      return true;
    }
    return false;
  }
  inline bool stopMsrs(int count, const char* what, const char* name, const timer* w) {
    if (eng_report.startReport()) {
      eng_report.report() << "Computed  ";
      if (what) eng_report.report() << what << " ";
      eng_report.report() << "measures";
      if (name) eng_report.report() << " for model " << name;
      eng_report.report().Put('\n');
      if (count) {
         eng_report.report() << "\t" << count << " measures computed\n";
      }
      if (w) {
        eng_report.report() << "\t" << w->elapsed();
        eng_report.report() << " seconds required for measure computation\n";
      }
      return true;
    }
    return false;
  }

  inline void infinitySubtract(measure* m) const {
    if (em->startError()) {
      em->causedBy(m);
      em->cerr() << "Undefined operation (infty-infty) while computing ";
      em->cerr() << m->Name();
      em->stopIO();
    }
    m->SetNull();
  }
};

named_msg exact_mcmsr::eng_debug;
named_msg exact_mcmsr::eng_report;
engtype* exact_mcmsr::ProcessGeneration = 0;

// **************************************************************************
// *                          msr_visitor  methods                          *
// **************************************************************************

exact_mcmsr::msr_visitor::msr_visitor(const hldsm* mdl)
 : state_visitor(mdl)
{
  x.answer = &tmp;
}

// **************************************************************************
// *                        realmsr_visitor  methods                        *
// **************************************************************************

exact_mcmsr::realmsr_visitor::realmsr_visitor(const hldsm* mdl)
 : msr_visitor(mdl)
{
}

bool exact_mcmsr::realmsr_visitor::visit()
{
  DCASSERT(m->RHSType());
  DCASSERT(m->RHSType()->getBaseType() == em->REAL);
  m->ComputeRHS(x);
  if (tmp.isNormal()) {
    double term = p[x.current_state_index] * tmp.getReal();
    if (0==term) return false;

    if (infmask && infmask->contains(x.current_state_index)) {
      // this term is +/- infinity
      if (ans.isInfinity()) {
        // check if signs match
        if (ans.signInfinity() != SIGN(term)) {
          return infinitySubtract(m);
        }
        // they match, we're fine
      } else {
        ans.setInfinity(SIGN(term));
      }
    } else { 
      // this term is finite
      if (ans.isInfinity()) {
        // do nothing
      } else {
        ans.setReal(ans.getReal() + term);
      }
    }
    return false;
  } 
    
  if (tmp.isInfinity()) {
    int termsign = SIGN(p[x.current_state_index] * tmp.signInfinity());
    if (ans.isInfinity()) {
      if (ans.signInfinity() != termsign) {
        return infinitySubtract(m);
      }
    } else {
      ans.setInfinity(termsign);
    }
  }

  // abnormal result, deal with it
  DCASSERT(tmp.isNull());
  ans.setNull();
  return true;
}

// **************************************************************************
// *                        boolmsr_visitor  methods                        *
// **************************************************************************

exact_mcmsr::boolmsr_visitor::boolmsr_visitor(const hldsm* mdl)
 : msr_visitor(mdl)
{
}

bool exact_mcmsr::boolmsr_visitor::visit()
{
  DCASSERT(m->RHSType());
  DCASSERT(m->RHSType()->getBaseType() == em->BOOL);
  m->ComputeRHS(x);
  if (tmp.isNormal()) {
    if (false == tmp.getBool()) return false;
    if (infmask && infmask->contains(x.current_state_index)) {
      // this term is +/- infinity
      if (ans.isInfinity()) {
        if (ans.signInfinity() != SIGN(p[x.current_state_index])) {
          return infinitySubtract(m);
        }
      } else {
        ans.setInfinity(SIGN(p[x.current_state_index]));
      }
    } else {
      // this term is finite
      if (ans.isInfinity()) {
        // do nothing
      } else {
        ans.setReal(ans.getReal() + p[x.current_state_index]);
      }
    }
    return false;
  }

  DCASSERT(tmp.isNull());
  ans.setNull();
  return true;
}

// **************************************************************************
// *                          exact_mcmsr  methods                          *
// **************************************************************************

exact_mcmsr::exact_mcmsr() : subengine()
{
}

bool exact_mcmsr::AppliesToModelType(hldsm::model_type mt) const
{
  return 
    (hldsm::Enumerated == mt)  || 
    (hldsm::Asynch_Events == mt)   || 
    (hldsm::Synch_Events == mt);
}

void exact_mcmsr::GenerateProc(hldsm* m) const
{
  result f;
  f.setBool(false);
  lldsm* proc = m->GetProcess();
  if (proc) {
    subengine* gen = proc->getCompletionEngine();
    if (gen)  gen->RunEngine(m, f);
    if (lldsm::Error == proc->Type()) throw Engine_Failed;
  } else {
    if (0==ProcessGeneration) throw No_Engine;
    ProcessGeneration->runEngine(m, f);
    proc = m->GetProcess();
  }
  DCASSERT(proc->Type() == lldsm::DTMC || proc->Type() == lldsm::CTMC);
}


// **************************************************************************
// *                                                                        *
// *                           mcex_steady  class                           *
// *                                                                        *
// **************************************************************************

class mcex_steady : public exact_mcmsr {
public:
  mcex_steady();
  virtual void SolveMeasures(hldsm* m, set_of_measures* list);
};

mcex_steady the_mcex_steady;

// **************************************************************************
// *                          mcex_steady  methods                          *
// **************************************************************************

mcex_steady::mcex_steady() : exact_mcmsr()
{
}

void mcex_steady::SolveMeasures(hldsm* mdl, set_of_measures* list)
{
  DCASSERT(mdl);
  DCASSERT(AppliesToModelType(mdl->Type()));
  if (eng_debug.startReport()) {
    eng_debug.report() << "Running exact steady-state engine\n";
    eng_debug.stopIO();
  }
  timer* w = 0;
  DCASSERT(mdl);
  GenerateProc(mdl);
  const stochastic_lldsm* proc = 
    smart_cast<const stochastic_lldsm*> (mdl->GetProcess());
  DCASSERT(proc);
  long NS = proc->getNumStates(false);
  if (NS < 0) throw Engine_Failed;
  statevect* dist = 0;
  double* p = 0;
  bool ok = true;
  if (NS) {
    p = (double*) malloc(NS * sizeof(double));
    if (0==p) throw Out_Of_Memory;
    ok = proc->computeSteadyState(p);
  }
  if (ok) {
    realmsr_visitor rv(mdl);
    rv.setVector(p, 0);
    boolmsr_visitor bv(mdl);
    bv.setVector(p, 0);
    long count = 0;
    if (startMsrs("steady-state", mdl->Name())) {
      em->stopIO();
      w = makeTimer();
    }
    for (measure* m = list->popMeasure(); m; m=list->popMeasure()) {
      if (em->STATEDIST == m->Type()) {
        //
        // This is a distribution measure, just copy it!
        //
        result v;
        if (0==dist) {
          dist = new statedist(proc, p, NS);
          v.setPtr(dist);
        } else {
          v.setPtr(Share(dist));
        }
        m->SetValue(v);
        continue;
      }
      //
      // Ordinary measure, compute it
      //
      const type* mt = m->RHSType();
      if (mt) mt = mt->getBaseType();
      if (mt == em->REAL) {
        rv.newMsr(m);
        proc->visitStates(rv);
        rv.finish();
        count++;
        continue;
      }
      if (mt == em->BOOL) {
        bv.newMsr(m);
        proc->visitStates(bv);
        bv.finish();
        count++;
        continue;
      }
      //
      // Some kind of error, null failsafe
      //
      m->SetNull();
    } // for m
    if (stopMsrs(count, "steady-state", mdl->Name(), w)) {
      em->stopIO();
      doneTimer(w);
    }
  }
  free(p);
  if (eng_debug.startReport()) {
    eng_debug.report() << "Finished exact steady-state engine\n";
    eng_debug.stopIO();
  }
  if (!ok) throw Engine_Failed;
}

// **************************************************************************
// *                                                                        *
// *                            mcex_trans class                            *
// *                                                                        *
// **************************************************************************

class mcex_trans : public exact_mcmsr {
public:
  mcex_trans();
  virtual void SolveMeasures(hldsm* m, set_of_measures* list);
};

mcex_trans the_mcex_trans;

// **************************************************************************
// *                           mcex_trans methods                           *
// **************************************************************************

mcex_trans::mcex_trans() : exact_mcmsr()
{
}

void mcex_trans::SolveMeasures(hldsm* mdl, set_of_measures* list)
{
  DCASSERT(mdl);
  DCASSERT(AppliesToModelType(mdl->Type()));
  if (eng_debug.startReport()) {
    eng_debug.report() << "Running exact transient engine\n";
    eng_debug.stopIO();
  }
  DCASSERT(mdl);
  GenerateProc(mdl);
  const stochastic_lldsm* proc = 
    smart_cast<const stochastic_lldsm*> (mdl->GetProcess());
  DCASSERT(proc);
  DCASSERT(proc->Type() == lldsm::DTMC || proc->Type() == lldsm::CTMC);
  long NS = proc->getNumStates(false);
  if (NS < 0) throw Engine_Failed;
  statevect* dist = 0;
  double* p = 0;
  double* aux1 = 0;
  double* aux2 = 0;
  bool ok = true;
  if (NS) {
    p = (double*) malloc(NS * sizeof(double));
    aux1 = (double*) malloc(NS * sizeof(double));
    ok = (p && aux1);
    if (proc->Type() == lldsm::CTMC) {
      aux2 = (double*) malloc(NS * sizeof(double));
      ok = ok && aux2;
    }
    if (!ok) {
      free(p);
      free(aux1);
      free(aux2);
      throw Out_Of_Memory;
    }
  }
  double last_time = 0.0;
  statedist* initial = proc->getInitialDistribution();
  initial->ExportTo(p);
  Delete(initial);
  realmsr_visitor rv(mdl);
  boolmsr_visitor bv(mdl);

  // go through measures
  for (measure* m = list->popMeasure(); m; m=list->popMeasure()) {
    if (!ok) break;
    time_measure* tm = smart_cast <time_measure*> (m);
    DCASSERT(tm);
    double dt = tm->GetTime() - last_time;
    DCASSERT(dt >= 0);
    if (dt) {
      dist = 0;
      if (eng_debug.startReport()) {
        eng_debug.report() << "time = " << tm->GetTime();
        eng_debug.report() << ", delta = " << dt << "\n";
        eng_debug.stopIO();
      }
      ok = proc->computeTransient(dt, p, aux1, aux2);
      last_time = tm->GetTime();
      if (!ok) break;
    } // if dt
    if (em->STATEDIST == m->Type()) {
        //
        // This is a distribution measure, just copy it!
        //
        result v;
        if (0==dist) {
          dist = new statedist(proc, p, NS);
          v.setPtr(dist);
        } else {
          v.setPtr(Share(dist));
        }
        m->SetValue(v);
        continue;
    }
    //
    // Ordinary measure, compute it
    //
    const type* mt = tm->RHSType();
    if (mt) mt = mt->getBaseType();
    if (mt == em->REAL) {
      rv.setVector(p, 0);
      rv.newMsr(m);
      proc->visitStates(rv);
      rv.finish();
      continue;
    }
    if (mt == em->BOOL) {
      bv.setVector(p, 0);
      bv.newMsr(m);
      proc->visitStates(bv);
      bv.finish();
      continue;
    }
    //
    // Some kind of error, null failsafe
    //
    m->SetNull();
  } // for m
  free(p);
  free(aux1);
  free(aux2);
  if (eng_debug.startReport()) {
    eng_debug.report() << "Finished exact transient engine\n";
    eng_debug.stopIO();
  }
  if (!ok) throw Engine_Failed;
}

// **************************************************************************
// *                                                                        *
// *                             mcex_acc class                             *
// *                                                                        *
// **************************************************************************

class mcex_acc : public exact_mcmsr {
public:
  mcex_acc();
  virtual void SolveMeasures(hldsm* m, set_of_measures* list);
};

mcex_acc the_mcex_acc;

// **************************************************************************
// *                            mcex_acc methods                            *
// **************************************************************************

mcex_acc::mcex_acc() : exact_mcmsr()
{
}

void mcex_acc::SolveMeasures(hldsm* mdl, set_of_measures* list)
{
  DCASSERT(mdl);
  DCASSERT(AppliesToModelType(mdl->Type()));
  if (eng_debug.startReport()) {
    eng_debug.report() << "Running exact accumulated engine\n";
    eng_debug.stopIO();
  }
  DCASSERT(mdl);
  GenerateProc(mdl);
  const stochastic_lldsm* proc = 
    smart_cast<const stochastic_lldsm*> (mdl->GetProcess());
  DCASSERT(proc);
  DCASSERT(proc->Type() == lldsm::DTMC || proc->Type() == lldsm::CTMC);
  long NS = proc->getNumStates(false);
  if (NS < 0) throw Engine_Failed;
  double* p0 = 0;
  double* n = 0;
  double* aux1 = 0;
  double* aux2 = 0;
  bool ok = true;
  if (NS) {
    p0 = (double*) malloc(NS * sizeof(double));
    n = (double*) malloc(NS * sizeof(double));
    aux1 = (double*) malloc(NS * sizeof(double));
    aux2 = (double*) malloc(NS * sizeof(double));
    ok = (p0 && n && aux1 && aux2);
    if (!ok) {
      free(p0);
      free(n);
      free(aux1);
      free(aux2);
      throw Out_Of_Memory;
    }
  }
  double last_start = 0.0;
  double last_stop = -1.0;
  statedist* initial = proc->getInitialDistribution();
  initial->ExportTo(p0);
  Delete(initial);
  realmsr_visitor rv(mdl);
  boolmsr_visitor bv(mdl);

  // go through measures
  for (measure* m = list->popMeasure(); m; m=list->popMeasure()) {
    if (!ok) break;
    time_measure* tm = smart_cast <time_measure*> (m);
    DCASSERT(tm);
    double dt = tm->GetTime() - last_start;
    DCASSERT(dt >= 0);
    if (dt) {
      if (eng_debug.startReport()) {
        eng_debug.report() << "time = " << tm->GetTime();
        eng_debug.report() << ", delta = " << dt << "\n";
        eng_debug.stopIO();
      }
      ok = proc->computeTransient(dt, p0, aux1, aux2);
      last_start = tm->GetTime();
      if (!ok) break;
      last_stop = tm->GetStopTime();
      ok = proc->computeAccumulated(last_stop-last_start, p0, n, aux1, aux2);
      if (!ok) break;
    } else if (last_stop != tm->GetStopTime()) {
      last_stop = tm->GetStopTime();
      ok = proc->computeAccumulated(last_stop-last_start, p0, n, aux1, aux2);
    } // if dt
    const type* mt = tm->RHSType();
    if (mt) mt = mt->getBaseType();
    if (mt == em->REAL) {
      rv.setVector(n, 0);
      rv.newMsr(m);
      proc->visitStates(rv);
      rv.finish();
      continue;
    }
    if (mt == em->BOOL) {
      bv.setVector(n, 0);
      bv.newMsr(m);
      proc->visitStates(bv);
      bv.finish();
      continue;
    }
    m->SetNull();
  } // for m
  free(p0);
  free(n);
  free(aux1);
  free(aux2);
  if (eng_debug.startReport()) {
    eng_debug.report() << "Finished exact accumulated engine\n";
    eng_debug.stopIO();
  }
  if (!ok) throw Engine_Failed;
}

// **************************************************************************
// *                                                                        *
// *                           mcex_infacc  class                           *
// *                                                                        *
// **************************************************************************

class mcex_infacc : public exact_mcmsr {
public:
  mcex_infacc();
  virtual void SolveMeasures(hldsm* m, set_of_measures* list);
};

mcex_infacc the_mcex_infacc;

// **************************************************************************
// *                          mcex_infacc  methods                          *
// **************************************************************************

mcex_infacc::mcex_infacc() : exact_mcmsr()
{
}

void mcex_infacc::SolveMeasures(hldsm* mdl, set_of_measures* list)
{
  DCASSERT(mdl);
  DCASSERT(AppliesToModelType(mdl->Type()));
  if (eng_debug.startReport()) {
    eng_debug.report() << "Running exact infinite accumulation engine\n";
    eng_debug.stopIO();
  }
  DCASSERT(mdl);
  GenerateProc(mdl);
  const stochastic_lldsm* proc = 
    smart_cast<const stochastic_lldsm*> (mdl->GetProcess());
  DCASSERT(proc);
  DCASSERT(proc->Type() == lldsm::DTMC || proc->Type() == lldsm::CTMC);
  long NS = proc->getNumStates(false);
  if (NS < 0) throw Engine_Failed;
  double* p0 = 0;
  double* n = 0;
  double* aux1 = 0;
  double* aux2 = 0;
  intset not_transient(NS);
  proc->getClass(0, not_transient);
  not_transient.complement();
  bool ok = true;
  if (NS) {
    p0 = (double*) malloc(NS * sizeof(double));
    n = (double*) malloc(NS * sizeof(double));
    aux1 = (double*) malloc(NS * sizeof(double));
    ok = (p0 && n && aux1);
    if (proc->Type() == lldsm::CTMC) {
      aux2 = (double*) malloc(NS * sizeof(double));
      ok = ok && aux2;
    }
  }
  if (!ok) {
    free(p0);
    free(n);
    free(aux1);
    free(aux2);
    throw Out_Of_Memory;
  }
  double last_time = 0.0;
  bool not_computed = true;
  statedist* initial = proc->getInitialDistribution();
  initial->ExportTo(p0);
  Delete(initial);
  realmsr_visitor rv(mdl);
  boolmsr_visitor bv(mdl);

  // go through measures
  for (measure* m = list->popMeasure(); m; m=list->popMeasure()) {
    if (!ok) break;
    time_measure* tm = smart_cast <time_measure*> (m);
    DCASSERT(tm);
    double dt = tm->GetTime() - last_time;
    DCASSERT(dt >= 0);
    if (dt || not_computed) {
      if (eng_debug.startReport()) {
        eng_debug.report() << "time = " << tm->GetTime();
        eng_debug.report() << ", delta = " << dt << "\n";
        eng_debug.stopIO();
      }
      ok = proc->computeTransient(dt, p0, aux1, aux2);
      last_time = tm->GetTime();
      if (!ok) break;
      // we have initial distribution updated, now compute n vector
      ok = proc->computeClassProbs(p0, n);
      not_computed = false;
      if (!ok) break;
    } // if dt

    // evaluate measure
    const type* mt = tm->RHSType();
    if (mt) mt = mt->getBaseType();
    if (mt == em->REAL) {
      rv.setVector(n, &not_transient);
      rv.newMsr(m);
      proc->visitStates(rv);
      rv.finish();
      continue;
    }
    if (mt == em->BOOL) {
      bv.setVector(n, &not_transient);
      bv.newMsr(m);
      proc->visitStates(bv);
      bv.finish();
      continue;
    }
    m->SetNull();
  } // for m
  free(p0);
  free(n);
  free(aux1);
  free(aux2);
  if (eng_debug.startReport()) {
    eng_debug.report() << "Finished exact infinite accumulation engine\n";
    eng_debug.stopIO();
  }
  if (!ok) throw Engine_Failed;
}

// **************************************************************************
// *                                                                        *
// *                         exact_ph_analyze class                         *
// *                                                                        *
// **************************************************************************

/// Used for both phase average and phase variance computation.
class exact_ph_analyze : public exact_mcmsr {
  bool avg_only;
public:
  exact_ph_analyze(bool ao);
  virtual bool AppliesToModelType(hldsm::model_type) const;
  virtual void RunEngine(hldsm* m, result &parm);
};

exact_ph_analyze the_exact_ph_avg(true);
exact_ph_analyze the_exact_ph_var(false);

// **************************************************************************
// *                        exact_ph_analyze methods                        *
// **************************************************************************

exact_ph_analyze::exact_ph_analyze(bool ao) : exact_mcmsr()
{
  avg_only = ao;
}

bool exact_ph_analyze::AppliesToModelType(hldsm::model_type mt) const
{
  return (hldsm::Phase_Type==mt);
}


void exact_ph_analyze::RunEngine(hldsm* foo, result &fls)
{
  phase_hlm* X = smart_cast <phase_hlm*> (foo);
  DCASSERT(X);

  if (eng_debug.startReport()) {
    if (avg_only) {
      eng_debug.report() << "Computing average for a ";
    } else {
      eng_debug.report() << "Computing variance for a ";
    }
    eng_debug.report() << ((X->isDiscrete()) ? "Discrete" : "Continuous");
    eng_debug.report() << " phase type\n";
    eng_debug.stopIO();
  }

  //
  // Generate the process
  //
  stochastic_lldsm* proc = smart_cast <stochastic_lldsm*> (X->GetProcess());

  if (0==proc) { 
    if (0==ProcessGeneration) {
      throw No_Engine;
    }

    fls.setBool(false);
    ProcessGeneration->runEngine(X, fls);

    proc = smart_cast <stochastic_lldsm*> (X->GetProcess());
  }

  if (0==proc) {
    em->cout() << "\tCouldn't build process for ";
    foo->Print(em->cout(), 0);
    em->cout() << "\n";
    return;
  }

  //
  // Check if numerical solution engine was run already
  //

  if (proc->knownVTTA())  return;

  //
  // Get process information
  //

  long ns = proc->getNumStates(false);
  long goal = proc->getAcceptingState();
  if (goal < 0) {
    // No goal state - we're constant infinity
    proc->setAcceptProb(0);
    proc->setTrapProb(1);
    proc->setInfiniteMTTA();
    proc->setVTTA(0); 
    return;
  }
  long trap = proc->getTrapState();
  if (1==ns) {
    // degenerate case - one state, must be the goal
    DCASSERT(trap<0);
    proc->setAcceptProb(1);
    proc->setTrapProb(0);
    proc->setMTTA(0);
    proc->setVTTA(0);
    return;
  }
  double* init = new double[ns];
  double* vx = new double[ns];

  //
  // Compute time spent in each state
  // (because we need it anyway)
  //

  timer* watch = 0;
  if (eng_report.startReport()) {
    eng_report.report() << "Computing time spent in states\n";
    eng_report.stopIO();
    watch = makeTimer();
    watch->reset();
  }
  
  statedist* initial = proc->getInitialDistribution();
  initial->ExportTo(init);
  Delete(initial);
  
  if (!proc->computeClassProbs(init, vx)) {
    doneTimer(watch);
    throw Engine_Failed;
  }

  if (eng_report.startReport()) {
    eng_report.report() << "computation took " << watch->elapsed() << " seconds\n";
    eng_report.stopIO();
    doneTimer(watch);
  }

  //
  // Part of our job - save accept and trap probs.
  //
  proc->setAcceptProb(vx[goal]);
  if (trap >= 0) {
    proc->setTrapProb(vx[trap]);
  } else {
    proc->setTrapProb(0);
  }

  //
  // Compute overall MTTA, since we have it
  //

  if (proc->getTrapProb() > 0) {
    proc->setInfiniteMTTA();
    if (0==proc->getAcceptProb()) {
      // always get to trap,
      // means we're "constant infinity"
      proc->setVTTA(0); 
    } else {
      proc->setInfiniteVTTA();
    }
    return;
  }

  double mtta = 0.0;
  for (int i=0; i<ns; i++) {
    if (trap == i) continue;
    if (goal == i) continue;
    mtta += vx[i];
  }
  proc->setMTTA(mtta);

  if (!avg_only) {
      //
      // Now, determine variance by state
      //
      // Extreme magic, discrete case:
      //      From "Neuts", 2nd factorial moment is
      //        E[X(X-1)] = 2 alpha T (I-T)^{-2} e
      //      and we can massage this into
      //        E[X(X-1)] = 2(n-p0)N e
      //      so we need to compute the vector
      //        v = 2(n-p0)N
      //      which we do by solving
      //        v(Pzz-I) = -2(n-p0)
      //      by using 2(n-p0) as the initial distribution.
      //  
      //  Extreme magic, continuous case:
      //      Also from "Neuts", second moment is
      //        E[X^2] = 2 alpha T^{-2} e
      //      and we can massage this into
      //        E[X^2] = -2 sigma Qzz^{-1} e
      //      so we need to compute the vector
      //        v = -2 sigma Qzz^{-1}
      //      which we do by solving
      //        v Qzz = -2sigma
      //      by using 2sigma as the initial distribution
      //
      if (eng_report.startReport()) {
        eng_report.report() << "Computing variance by state\n";
        eng_report.stopIO();
        watch = makeTimer();
        watch->reset();
      }
    
      //
      //  Build fake initial vector
      //
      if (X->isDiscrete()) {
        // Discrete case
        for (int i=0; i<ns; i++) {
          init[i] = 2*(vx[i] - init[i]);
        }
      } else {
        // Continuous case
        for (int i=0; i<ns; i++) {
          init[i] = 2*vx[i];
        }
      }
    
      //
      // Compute our v vector
      //
      if (!proc->computeTimeInStates(init, vx)) {
        doneTimer(watch);
        throw Engine_Failed;
      }

      if (eng_report.startReport()) {
        eng_report.report() << "computation took " << watch->elapsed() << " seconds\n";
        eng_report.stopIO();
        doneTimer(watch);
      }

      //
      // Compute overall VTTA
      //
      double vtta = 0.0;
      for (int i=0; i<ns; i++) {
        if (trap == i) continue;
        if (goal == i) continue;
        vtta += vx[i];
      }

      //
      // Adjust result - because so far, we do NOT have the variance
      //
      if (X->isDiscrete()) {
        // so far, we have vtta = E[X(X-1)] = E[XX] - E[X]
        vtta += mtta; 
      } 
      // vtta is currently the second moment
      vtta -= mtta * mtta;
      proc->setVTTA(vtta);
  }

  //
  // Cleanup
  //
  delete[] init;
  delete[] vx;
}


// **************************************************************************
// *                                                                        *
// *                               Front  end                               *
// *                                                                        *
// **************************************************************************

void InitializeExactSolutionEngines(exprman* em)
{
  if (0==em)  return;

  option* debug = em->findOption("Debug");
  exact_mcmsr::eng_debug.Initialize(debug,
    "exact_solver",
    "When set, diagnostic messages are displayed regarding Markov chain exact solution engines.", 
    false
  );

  option* report = em->findOption("Report");
  exact_mcmsr::eng_report.Initialize(report,
    "exact_solver",
    "When set, exact solution measure performance is reported.", 
    false
  );

  const char* exact = "EXACT";
  const char* desc = "Exact analysis of underlying stochastic process";

  RegisterEngine(em, "SteadyStateAverage", exact, desc, &the_mcex_steady);
  RegisterEngine(em, "TransientAverage", exact, desc, &the_mcex_trans);
  RegisterEngine(em, "SteadyStateAccumulated", exact, desc, &the_mcex_infacc);
  RegisterEngine(em, "TransientAccumulated", exact, desc, &the_mcex_acc);

  RegisterEngine(em, "AvgPh", exact, desc, &the_exact_ph_avg);
  RegisterEngine(em, "VarPh", exact, desc, &the_exact_ph_var);

  exact_mcmsr::ProcessGeneration = em->findEngineType("ProcessGeneration");
}

