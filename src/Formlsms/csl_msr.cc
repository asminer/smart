
// $Id$

#include "csl_msr.h"
#include "../ExprLib/engine.h"
#include "../ExprLib/measures.h"
#include "check_llm.h"

#include "../Modules/biginttype.h"
#include "../Modules/statesets.h"


// *****************************************************************
// *                                                               *
// *                           CSL_engine                          *
// *                                                               *
// *****************************************************************

class CSL_engine : public msr_noengine {
protected:
  /** Engines for computing PU, explicitly.
      This is a "Function call" style engine.
      parameter 0: pointer, to the stochastic model.
      parameter 1: pointer, to stateset p; if NULL we compute PF.
      parameter 2: pointer, to stateset q.
      Computes probability that a path from an initial state
      satisfies p U q, or F q if p is null.
  */
  static engtype* PU_explicit;

  /** Engines for building a phase type for TU.
      This is a "Function call" style engine.
      parameter 0: pointer, to the stochastic model.
      parameter 1: pointer, to stateset p; if NULL we compute PF.
      parameter 2: pointer, to stateset q.
      Computes the distribution where a path from an initial state
      satisfies p U q, or F q if p is null.
  */
  static engtype* TU_generator;

  static engtype* ProcGen;
 
  friend void InitCSLMeasureFuncs(exprman* em, List <msr_func> *common);

public:
  CSL_engine(const type* t, const char* name, int np);

protected:
  static const char* nameType(lldsm::model_type t) {
    switch (t) {
      case lldsm::DTMC:   return "ph int";
      case lldsm::CTMC:   return "ph real";
      case lldsm::GSP:    return "rand real";
      default:            return "an invalid type";
    };
  }

  inline lldsm* BuildRG(hldsm* hlm, const expr* err) const {
    if (0==hlm)  return 0;
    result f;
    f.setBool(false);

    try {
      lldsm* llm = hlm->GetProcess();
      if (llm) {
        subengine* gen = llm->getCompletionEngine();
        if (0==gen) return llm;
        gen->RunEngine(hlm, f);
      } else {
        if (!ProcGen) throw subengine::No_Engine;
        ProcGen->runEngine(hlm, f);
      }
      return hlm->GetProcess();
    } // try
    catch (subengine::error e) {
      if (em->startError()) {
        em->causedBy(err);
        em->cerr() << "Couldn't build reachability graph: ";
        em->cerr() << subengine::getNameOfError(e);
        em->stopIO();
      }
      return 0;
    } // catch
  }

  inline checkable_lldsm* getLLM(traverse_data &x, expr* p) const {
    model_instance* mi = grabModelInstance(x, p);
    if (0==mi) return 0;
    lldsm* foo = BuildRG(mi->GetCompiledModel(), x.parent);
    if (0==foo) return 0;
    if (foo->Type() == lldsm::Error) return 0;
    return dynamic_cast <checkable_lldsm*>(foo);
  }

  inline stateset* grabParam(const lldsm* m, expr* p, traverse_data &x) const {
    if (0==m || 0==p) return 0;
    p->Compute(x);
    if (!x.answer->isNormal()) return 0;
    stateset* ss = smart_cast <stateset*> (Share(x.answer->getPtr()));
    DCASSERT(ss);
    if (ss->getParent() == m) return ss;
    if (em->startError()) {
      em->causedBy(x.parent);
      em->cerr() << "Stateset in " << Name();
      em->cerr() << " expression is from a different model";
      em->stopIO();
    }   
    Delete(ss);
    return 0;
  }

  inline bool mismatched(const stateset* p, const stateset* q, traverse_data &x) const {
    DCASSERT(p);
    DCASSERT(q);
    if (p->isExplicit() == q->isExplicit()) return false;
    if (em->startError()) {
      em->causedBy(x.parent);
      em->cerr() << "Statesets in " << Name();
      em->cerr() << " use different storage types";
      em->stopIO();
    }   
    return true;
  }

  inline void launchEngine(engtype* et, result* pass, int np, traverse_data &x) const {
    try {
      if (et)   et->runEngine(pass, np,x);
      else      throw subengine::No_Engine;
    }
    catch (subengine::error e) {
      if (em->startError()) {
        em->causedBy(x.parent);
        em->cerr() << "Couldn't compute " << Name() << ": ";
        em->cerr() << subengine::getNameOfError(e);
        em->stopIO();
      }
      x.answer->setNull();
    }
  }


};

engtype*  CSL_engine::PU_explicit             = 0;
engtype*  CSL_engine::TU_generator            = 0;
engtype*  CSL_engine::ProcGen                 = 0;

CSL_engine::CSL_engine(const type* t, const char* name, int np)
 : msr_noengine(CSL, t, name, np)
{
}


// *****************************************************************
// *                                                               *
// *                            PF_func                            *
// *                                                               *
// *****************************************************************

class PF_func : public CSL_engine {
public:
  PF_func();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

PF_func::PF_func()
 : CSL_engine(em->REAL, "PF", 2)
{
  SetFormal(0, em->MODEL, "m");
  HideFormal(0);
  SetFormal(1, em->STATESET, "p");
  SetDocumentation("Determine the probability that a path starting from an initial state has the form:\n~~~~? ---> ? ---> ... ---> ? ---> p ---> ? ...");
}

void PF_func::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);
  DCASSERT(2==np);
  lldsm* llm = getLLM(x, pass[0]);
  stateset* p = grabParam(llm, pass[1], x);
  if (0==p) {
    x.answer->setNull();
    return;
  }

  if (!p->isExplicit()) {
    // TBD - error message
    x.answer->setNull();
    return;
  }

  result engpass[3];
  engpass[0].setPtr(Share(llm));
  engpass[1].setNull();
  engpass[2].setPtr(p);
  launchEngine(PU_explicit, engpass, 3, x);
}

// *****************************************************************
// *                                                               *
// *                            PU_func                            *
// *                                                               *
// *****************************************************************

class PU_func : public CSL_engine {
public:
  PU_func();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

PU_func::PU_func()
 : CSL_engine(em->REAL, "PU", 3)
{
  SetFormal(0, em->MODEL, "m");
  HideFormal(0);
  SetFormal(1, em->STATESET, "p");
  SetFormal(2, em->STATESET, "q");
  SetDocumentation("Determine the probability that a path starting from an initial state has the form:\n~~~~p ---> p ---> ... ---> p ---> q ---> ? ...\nwhere q is eventually satisfied (after zero or more states where p is satisfied).");
}

void PU_func::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);
  DCASSERT(3==np);
  lldsm* llm = getLLM(x, pass[0]);
  stateset* p = grabParam(llm, pass[1], x);
  if (0==p) {
    x.answer->setNull();
    return;
  }
  stateset* q = grabParam(llm, pass[2], x);
  if (0==q) {
    Delete(p);
    x.answer->setNull();
    return;
  }
  if (mismatched(p, q, x)) {
    Delete(p);
    Delete(q);
    x.answer->setNull();
    return;
  }

  if (!p->isExplicit()) {
    // TBD - error message
    x.answer->setNull();
    return;
  }

  result engpass[3];
  engpass[0].setPtr(Share(llm));
  engpass[1].setPtr(p);
  engpass[2].setPtr(q);
  launchEngine(PU_explicit, engpass, 3, x);
}

// *****************************************************************
// *                                                               *
// *                            TF_func                            *
// *                                                               *
// *****************************************************************

class TF_func : public CSL_engine {
  lldsm::model_type llm_type;
public:
  TF_func(const type* rt, const char* name);
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

TF_func::TF_func(const type* rt, const char* name)
 : CSL_engine(rt, name, 2)
{
  llm_type = (em->INT == rt->getBaseType()) ? lldsm::DTMC : lldsm::CTMC;
  SetFormal(0, em->MODEL, "m");
  HideFormal(0);
  SetFormal(1, em->STATESET, "p");
  SetDocumentation("Determine the time (as a distribution) until a state in the set p is reached, with a time of infinity if p is never reached.  (Compare with PF.)  Returns null if the distribution type does not match.");
}

void TF_func::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);
  DCASSERT(2==np);
  lldsm* llm = getLLM(x, pass[0]);
  stateset* p = grabParam(llm, pass[1], x);
  if (0==p) {
    x.answer->setNull();
    return;
  }

  if (!p->isExplicit()) {
    // TBD - error message
    x.answer->setNull();
    return;
  }

  result engpass[3];
  engpass[0].setPtr(Share(llm));
  engpass[1].setNull();
  engpass[2].setPtr(p);
  launchEngine(TU_generator, engpass, 3, x);

  // Check that the type matches
  if (!x.answer->isNormal()) return;
  hldsm* tta = smart_cast <hldsm*>(x.answer->getPtr());
  DCASSERT(tta);
  if (tta->GetProcessType() == llm_type) return;

  // Report the type mismatch here
  if (em->startError()) {
    em->causedBy(x.parent);
    em->cerr() << "Underlying distribution is ";
    em->cerr() << nameType(tta->GetProcessType());
    em->cerr() << " instead of " << nameType(llm_type);
    em->stopIO();
  }

  x.answer->setNull();
}

// *****************************************************************
// *                                                               *
// *                            TU_func                            *
// *                                                               *
// *****************************************************************

class TU_func : public CSL_engine {
  lldsm::model_type llm_type;
public:
  TU_func(const type* rt, const char* name);
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

TU_func::TU_func(const type* rt, const char* name)
 : CSL_engine(rt, name, 3)
{
  llm_type = (em->INT == rt->getBaseType()) ? lldsm::DTMC : lldsm::CTMC;
  SetFormal(0, em->MODEL, "m");
  HideFormal(0);
  SetFormal(1, em->STATESET, "p");
  SetFormal(2, em->STATESET, "q");
  SetDocumentation("Determine the time (as a distribution) until a state in the set q is reached, along a path of states in the set p.  (Compare with PU.)  The time is infinity for paths that never reach q, or reach !p first.  Returns null if the distribution type does not match.");
}

void TU_func::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);
  DCASSERT(3==np);
  lldsm* llm = getLLM(x, pass[0]);
  stateset* p = grabParam(llm, pass[1], x);
  if (0==p) {
    x.answer->setNull();
    return;
  }
  stateset* q = grabParam(llm, pass[2], x);
  if (0==q) {
    Delete(p);
    x.answer->setNull();
    return;
  }
  if (mismatched(p, q, x)) {
    Delete(p);
    Delete(q);
    x.answer->setNull();
    return;
  }

  if (!p->isExplicit()) {
    // TBD - error message
    x.answer->setNull();
    return;
  }

  result engpass[3];
  engpass[0].setPtr(Share(llm));
  engpass[1].setPtr(p);
  engpass[2].setPtr(q);
  launchEngine(TU_generator, engpass, 3, x);

  // Check that the type matches
  if (!x.answer->isNormal()) return;
  hldsm* tta = smart_cast <hldsm*>(x.answer->getPtr());
  DCASSERT(tta);
  if (tta->GetProcessType() == llm_type) return;

  // Report the type mismatch here
  if (em->startError()) {
    em->causedBy(x.parent);
    em->cerr() << "Underlying distribution is ";
    em->cerr() << nameType(tta->GetProcessType());
    em->cerr() << " instead of " << nameType(llm_type);
    em->stopIO();
  }

  x.answer->setNull();
}


// ******************************************************************
// *                                                                *
// *                           front  end                           *
// *                                                                *
// ******************************************************************

void InitCSLMeasureFuncs(exprman* em, List <msr_func> *common)
{
  // Initialize engines

  CSL_engine::PU_explicit = MakeEngineType(em,
      "ExplicitPU",
      "Algorithm for explicit computation of PU formulas in CSL/PCTL.",
      engtype::FunctionCall
  );

  CSL_engine::TU_generator = MakeEngineType(em,
      "TUgenerator",
      "Generates the distribution for TU formulas in CSL/PCTL.",
      engtype::FunctionCall
  );

  CSL_engine::ProcGen = em->findEngineType("ProcessGeneration");

  // Add functions
  if (0==common) return;

  const type* phint  = em->INT  ? em->INT ->modifyType(PHASE) : 0;
  const type* phreal = em->REAL ? em->REAL->modifyType(PHASE) : 0;

  common->Append(new PF_func);
  common->Append(new PU_func);

  if (phint) {
    common->Append(new TF_func(phint, "phi_TF"));
    common->Append(new TU_func(phint, "phi_TU"));
  }
  if (phreal) {
    common->Append(new TF_func(phreal, "phr_TF"));
    common->Append(new TU_func(phreal, "phr_TU"));
  }
}

