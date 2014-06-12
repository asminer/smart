
// $Id$

#include "csl_msr.h"
#include "../ExprLib/engine.h"
#include "../ExprLib/measures.h"
#include "stoch_llm.h"

#include "../Modules/biginttype.h"
#include "../Modules/statesets.h"
#include "../Modules/statevects.h"

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

  inline const lldsm* getLLM(expr* p, traverse_data &x, result &slot) const {
    model_instance* mi = grabModelInstance(x, p);
    if (0==mi) return 0;
    lldsm* foo = BuildRG(mi->GetCompiledModel(), x.parent);
    if (0==foo) return 0;
    if (foo->Type() == lldsm::Error) return 0;
    checkable_lldsm* llm = dynamic_cast <checkable_lldsm*>(foo);
    if (0==llm) return 0;
    x.answer = &slot;
    slot.setPtr(Share(llm));
    return llm; 
  }

  /*
  inline checkable_lldsm* getLLM(traverse_data &x, expr* p) const {
    model_instance* mi = grabModelInstance(x, p);
    if (0==mi) return 0;
    lldsm* foo = BuildRG(mi->GetCompiledModel(), x.parent);
    if (0==foo) return 0;
    if (foo->Type() == lldsm::Error) return 0;
    return dynamic_cast <checkable_lldsm*>(foo);
  }
  */

  inline bool badStateset(const lldsm* m, expr* p, traverse_data &x, result &slot) const {
    if (0==m || 0==p) return true;
    x.answer = &slot;
    p->Compute(x);
    if (!slot.isNormal()) return true;
    stateset* ss = smart_cast <stateset*> (slot.getPtr());
    DCASSERT(ss);
    if (!ss->isExplicit()) {
      if (em->startError()) {
        em->causedBy(x.parent);
        em->cerr() << "Sorry, stateset in " << Name();
        em->cerr() << " must be explicit";
        em->stopIO();
      }   
      return true;
    }
    if (ss->getParent() == m) return false;
    if (em->startError()) {
      em->causedBy(x.parent);
      em->cerr() << "Stateset in " << Name();
      em->cerr() << " expression is from a different model";
      em->stopIO();
    }   
    return 0;
  }

  inline bool badStatedist(const lldsm* m, expr* p, traverse_data &x, result &slot) const {
    if (0==m || 0==p) return true;
    x.answer = &slot;
    p->Compute(x);
    if (!slot.isNormal()) return true;
    statedist* sd = smart_cast <statedist*> (slot.getPtr());
    DCASSERT(sd);
    if (sd->getParent() == m) return false;
    if (em->startError()) {
      em->causedBy(x.parent);
      em->cerr() << "Statedist in " << Name();
      em->cerr() << " expression is from a different model";
      em->stopIO();
    }   
    return 0;
  }

/*
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
  */

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

  static inline void nullAnswer(traverse_data &x, result *ans) {
    if (ans) ans->setNull();
    x.answer = ans;
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
  PF_func(bool dist_param);
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

PF_func::PF_func(bool dist_param)
 : CSL_engine(em->REAL, "PF", dist_param ? 3 : 2)
{
  SetFormal(0, em->MODEL, "m");
  HideFormal(0);
  SetFormal(1, em->STATESET, "p");
  if (dist_param) {
    SetFormal(2, em->STATEDIST, "pi0");
    SetDocumentation("Determine the probability that a path, starting according to the initial distribution pi0, has the form:\n~~~~? ---> ? ---> ... ---> ? ---> p ---> ? ...");
  } else {
    SetDocumentation("Determine the probability that a path, starting according to the model's initial distribution, has the form:\n~~~~? ---> ? ---> ... ---> ? ---> p ---> ? ...");
  }
}

void PF_func::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);
  DCASSERT(2==np || 3==np);

  result* ans = x.answer;
  result engpass[4];

  // slot 0 : model
  const lldsm* llm = getLLM(pass[0], x, engpass[0]);
  if (0==llm) {
    return nullAnswer(x, ans);    
  }

  // slot 1 : null (true of "true U p")
  engpass[1].setNull();

  // slot 2 : p of "true U p"
  if (badStateset(llm, pass[1], x, engpass[2])) {
    return nullAnswer(x, ans);
  }

  // slot 3 : initial distribution
  if (2==np) {
    engpass[3].setNull();
  } else {
    if (badStatedist(llm, pass[2], x, engpass[3])) {
      return nullAnswer(x, ans);
    }
  }

  x.answer = ans;
  launchEngine(PU_explicit, engpass, 4, x);
}

// *****************************************************************
// *                                                               *
// *                            PU_func                            *
// *                                                               *
// *****************************************************************

class PU_func : public CSL_engine {
public:
  PU_func(bool dist_param);
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

PU_func::PU_func(bool dist_param)
 : CSL_engine(em->REAL, "PU", dist_param ? 4 : 3)
{
  SetFormal(0, em->MODEL, "m");
  HideFormal(0);
  SetFormal(1, em->STATESET, "p");
  SetFormal(2, em->STATESET, "q");
  if (dist_param) {
    SetFormal(3, em->STATEDIST, "pi0");
    SetDocumentation("Determine the probability that a path, starting according to the initial distribution pi0, has the form:\n~~~~p ---> p ---> ... ---> p ---> q ---> ? ...\nwhere q is eventually satisfied (after zero or more states where p is satisfied).");
  } else {
    SetDocumentation("Determine the probability that a path, starting according to the model's initial distribution, has the form:\n~~~~p ---> p ---> ... ---> p ---> q ---> ? ...\nwhere q is eventually satisfied (after zero or more states where p is satisfied).");
  }
}

void PU_func::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);
  DCASSERT(3==np || 4==np);

  result* ans = x.answer;
  result engpass[4];

  // slot 0 : model
  const lldsm* llm = getLLM(pass[0], x, engpass[0]);
  if (0==llm) {
    return nullAnswer(x, ans);    
  }

  // slot 1 : p of "p U q"
  if (badStateset(llm, pass[1], x, engpass[1])) {
    return nullAnswer(x, ans);
  }

  // slot 2 : q of "true U p"
  if (badStateset(llm, pass[2], x, engpass[2])) {
    return nullAnswer(x, ans);
  }

  // slot 3 : initial distribution
  if (3==np) {
    engpass[3].setNull();
  } else {
    if (badStatedist(llm, pass[3], x, engpass[3])) {
      return nullAnswer(x, ans);
    }
  }

  x.answer = ans;
  launchEngine(PU_explicit, engpass, 4, x);
}

// *****************************************************************
// *                                                               *
// *                            TF_func                            *
// *                                                               *
// *****************************************************************

class TF_func : public CSL_engine {
  lldsm::model_type llm_type;
public:
  TF_func(const type* rt, const char* name, bool dist_param);
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

TF_func::TF_func(const type* rt, const char* name, bool dist_param)
 : CSL_engine(rt, name, dist_param ? 3 : 2)
{
  llm_type = (em->INT == rt->getBaseType()) ? lldsm::DTMC : lldsm::CTMC;
  SetFormal(0, em->MODEL, "m");
  HideFormal(0);
  SetFormal(1, em->STATESET, "p");
  if (dist_param) {
    SetFormal(2, em->STATEDIST, "pi0");
    SetDocumentation("Determine the time (as a distribution) until a state in the set p is reached, with a time of infinity if p is never reached, when the model starts according to initial distribution pi0.  (Compare with PF.)  Returns null if the distribution type does not match.");
  } else {
    SetDocumentation("Determine the time (as a distribution) until a state in the set p is reached, with a time of infinity if p is never reached, when the model starts according to its initial distribution.  (Compare with PF.)  Returns null if the distribution type does not match.");
  }
}

void TF_func::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);
  DCASSERT(2==np || 3==np);

  result* ans = x.answer;
  result engpass[4];

  // slot 0 : model
  const lldsm* llm = getLLM(pass[0], x, engpass[0]);
  if (0==llm) {
    return nullAnswer(x, ans);    
  }

  // slot 1 : null (true of "true U p")
  engpass[1].setNull();

  // slot 2 : p of "true U p"
  if (badStateset(llm, pass[1], x, engpass[2])) {
    return nullAnswer(x, ans);
  }

  // slot 3 : initial distribution
  if (2==np) {
    engpass[3].setNull();
  } else {
    if (badStatedist(llm, pass[2], x, engpass[3])) {
      return nullAnswer(x, ans);
    }
  }

  x.answer = ans;
  launchEngine(TU_generator, engpass, 4, x);

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
  TU_func(const type* rt, const char* name, bool dist_param);
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

TU_func::TU_func(const type* rt, const char* name, bool dist_param)
 : CSL_engine(rt, name, dist_param ? 4 : 3)
{
  llm_type = (em->INT == rt->getBaseType()) ? lldsm::DTMC : lldsm::CTMC;
  SetFormal(0, em->MODEL, "m");
  HideFormal(0);
  SetFormal(1, em->STATESET, "p");
  SetFormal(2, em->STATESET, "q");
  if (dist_param) {
    SetFormal(3, em->STATEDIST, "pi0");
    SetDocumentation("Determine the time (as a distribution) until a state in the set q is reached, along a path of states in the set p.  (Compare with PU.)  The time is infinity for paths that never reach q, or reach !p first.  The model starts according to the initial distribution pi0.  Returns null if the distribution type does not match.");
  } else {
    SetDocumentation("Determine the time (as a distribution) until a state in the set q is reached, along a path of states in the set p.  (Compare with PU.)  The time is infinity for paths that never reach q, or reach !p first.  The model starts according to its initial distribution.  Returns null if the distribution type does not match.");
  }
}

void TU_func::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);
  DCASSERT(3==np || 4==np);

  result* ans = x.answer;
  result engpass[4];

  // slot 0 : model
  const lldsm* llm = getLLM(pass[0], x, engpass[0]);
  if (0==llm) {
    return nullAnswer(x, ans);    
  }

  // slot 1 : p of "p U q"
  if (badStateset(llm, pass[1], x, engpass[1])) {
    return nullAnswer(x, ans);
  }

  // slot 2 : q of "true U p"
  if (badStateset(llm, pass[2], x, engpass[2])) {
    return nullAnswer(x, ans);
  }

  // slot 3 : initial distribution
  if (3==np) {
    engpass[3].setNull();
  } else {
    if (badStatedist(llm, pass[3], x, engpass[3])) {
      return nullAnswer(x, ans);
    }
  }

  x.answer = ans;
  launchEngine(TU_generator, engpass, 4, x);

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
      //
      // Parameters - same as TU_generator
  );

  CSL_engine::TU_generator = MakeEngineType(em,
      "TUgenerator",
      "Generates the distribution for TU formulas in CSL/PCTL.",
      engtype::FunctionCall
      //
      // Parameter 0: the model
      // Parameter 1: p of p U q; can be null to indicate "true"
      // Parameter 2: q of p U q
      // Parameter 3: the initial distribution to use;
      //              if null, we use the initial distribution
  );

  CSL_engine::ProcGen = em->findEngineType("ProcessGeneration");

  // Add functions
  if (0==common) return;

  const type* phint  = em->INT  ? em->INT ->modifyType(PHASE) : 0;
  const type* phreal = em->REAL ? em->REAL->modifyType(PHASE) : 0;

  common->Append(new PF_func(false) );
  common->Append(new PF_func(true ) );
  common->Append(new PU_func(false) );
  common->Append(new PU_func(true ) );

  if (phint) {
    common->Append(new TF_func(phint, "phi_TF", false));
    common->Append(new TF_func(phint, "phi_TF", true ));
    common->Append(new TU_func(phint, "phi_TU", false));
    common->Append(new TU_func(phint, "phi_TU", true ));
  }
  if (phreal) {
    common->Append(new TF_func(phreal, "phr_TF", false));
    common->Append(new TF_func(phreal, "phr_TF", true ));
    common->Append(new TU_func(phreal, "phr_TU", false));
    common->Append(new TU_func(phreal, "phr_TU", true ));
  }
}

