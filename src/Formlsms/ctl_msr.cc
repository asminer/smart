
// $Id$

#include "ctl_msr.h"
#include "../ExprLib/engine.h"
#include "../ExprLib/measures.h"
#include "check_llm.h"

#include "../Modules/biginttype.h"
#include "../Modules/statesets.h"

#include "../SymTabs/symtabs.h"

// *****************************************************************
// *                                                               *
// *                           CTL_engine                          *
// *                                                               *
// *****************************************************************

class CTL_engine : public msr_noengine {
protected:
  /** Engines for computing EX, explicitly.
      This is a "Function call" style engine.
      parameter 0: boolean, true means "reverse time".
      parameter 1: pointer, to stateset
  */
  static engtype* EX_explicit;

  /** Engines for computing EX, implicitly.
      This is a "Function call" style engine.
      parameter 0: boolean, true means "reverse time".
      parameter 1: pointer, to stateset
  */
  static engtype* EX_symbolic;

  /** Engines for computing EU, explicitly.
      This is a "Function call" style engine.
      parameter 0: boolean, true means "reverse time".
      parameter 1: pointer, to stateset p; if NULL we compute EF.
      parameter 2: pointer, to stateset q.
      Computes states satisfying E p U q, or EF q if p is null.
  */
  static engtype* EU_explicit;

  /** Engines for computing EU, implicitly.
      This is a "Function call" style engine.
      parameter 0: boolean, true means "reverse time".
      parameter 1: pointer, to stateset p; if NULL we compute EF.
      parameter 2: pointer, to stateset q.
      Computes states satisfying E p U q, or EF q if p is null.
  */
  static engtype* EU_symbolic;

  /** Engines for computing (unfair) EG, explicitly.
      This is a "Function call" style engine.
      parameter 0: boolean, true means "reverse time".
      parameter 1: pointer, to stateset
  */
  static engtype* unfairEG_explicit;

  /** Engines for computing (unfair) EG, implicitly.
      This is a "Function call" style engine.
      parameter 0: boolean, true means "reverse time".
      parameter 1: pointer, to stateset
  */
  static engtype* unfairEG_symbolic;

  /** Engines for computing (fair) EG, explicitly.
      This is a "Function call" style engine.
      parameter 0: boolean, true means "reverse time".
      parameter 1: pointer, to stateset
  */
  static engtype* fairEG_explicit;

  /** Engines for computing (fair) EG, implicitly.
      This is a "Function call" style engine.
      parameter 0: boolean, true means "reverse time".
      parameter 1: pointer, to stateset
  */
  static engtype* fairEG_symbolic;

  /** Engines for computing (unfair) AEF, explicitly.
      This is a "Function call" style engine.
      parameter 0: pointer, to low-level (graph) model.
      parameter 1: pointer, to stateset (control states)
      parameter 2: pointer, to stateset (goal states)
      Determines all states from which, we can reach a goal state
      (from control states, we can choose the next state, otherwise we cannot).
      Special cases:
      AEF 0, p = AF p
      AEF 1, p = EF p
  */
  static engtype* unfairAEF_explicit;

  /** Engines for computing (unfair) AEF, implicitly.
      This is a "Function call" style engine.
      parameter 0: pointer, to low-level (graph) model.
      parameter 1: pointer, to stateset (control states)
      parameter 2: pointer, to stateset (goal states)
      Determines all states from which, we can reach a goal state
      (from control states, we can choose the next state, otherwise we cannot).
      Special cases:
      AEF 0, p = AF p
      AEF 1, p = EF p
  */
  static engtype* unfairAEF_symbolic;

  static engtype* ProcGen;
 
  friend void InitCTLMeasureFuncs(symbol_table*, exprman*, List <msr_func> *);

protected:
  bool reverse_time;
public:
  CTL_engine(const type* t, const char* name, bool rt, int np);

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
      if (et) et->runEngine(pass, np, x);
      else    throw subengine::No_Engine;
    } // try
    catch (subengine::error e) {
      if (em->startError()) {
        em->causedBy(x.parent);
        em->cerr() << "Couldn't compute " << Name() << ": ";
        em->cerr() << subengine::getNameOfError(e);
        em->stopIO();
      }
      x.answer->setNull();
    } // catch
  }


};

engtype*  CTL_engine::EX_explicit             = 0;
engtype*  CTL_engine::EX_symbolic             = 0;
engtype*  CTL_engine::EU_explicit             = 0;
engtype*  CTL_engine::EU_symbolic             = 0;
engtype*  CTL_engine::unfairEG_explicit       = 0;
engtype*  CTL_engine::unfairEG_symbolic       = 0;
engtype*  CTL_engine::fairEG_explicit         = 0;
engtype*  CTL_engine::fairEG_symbolic         = 0;
engtype*  CTL_engine::unfairAEF_explicit      = 0;
engtype*  CTL_engine::unfairAEF_symbolic      = 0;
engtype*  CTL_engine::ProcGen                 = 0;

CTL_engine::CTL_engine(const type* t, const char* name, bool rt, int np)
 : msr_noengine(CTL, t, name, np)
{
  reverse_time = rt;
}

// *****************************************************************
// *                                                               *
// *                            EX_base                            *
// *                                                               *
// *****************************************************************

class EX_base : public CTL_engine {
public:
  EX_base(const char* name, bool rt);
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

EX_base::EX_base(const char* name, bool rt)
 : CTL_engine(em->STATESET, name, rt, 2)
{
  SetFormal(0, em->MODEL, "m");
  HideFormal(0);
  SetFormal(1, em->STATESET, "p");
}

void EX_base::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);
  const lldsm* llm = getLLM(x, pass[0]);
  stateset* p = grabParam(llm, pass[1], x);
  if (0==p) {
    x.answer->setNull();
    return;
  }
  engtype* ex = p->isExplicit() ? EX_explicit : EX_symbolic;
  result engpass[2];
  engpass[0].setBool(reverse_time);
  engpass[1].setPtr(p);
  launchEngine(ex, engpass, np, x);
}

// *****************************************************************
// *                                                               *
// *                               EX                              *
// *                                                               *
// *****************************************************************

class EX_si : public EX_base {
public:
  EX_si();
};

EX_si::EX_si() : EX_base("EX", false)
{
  SetDocumentation("CTL EX operator: build the set of starting states, from which some path has the form:\n~~~~? ---> p ---> ? ---> ? ...");
}

// *****************************************************************
// *                                                               *
// *                               EY                              *
// *                                                               *
// *****************************************************************

class EY_si : public EX_base {
public:
  EY_si();
};

EY_si::EY_si() : EX_base("EY", true)
{
  SetDocumentation("CTL EY operator: build the set of final states, to which some path has the form:\n~~~~... ? ---> ? ---> p ---> ?");
}

// *****************************************************************
// *                                                               *
// *                            EF_base                            *
// *                                                               *
// *****************************************************************

class EF_base : public CTL_engine {
public:
  EF_base(const char* name, bool rt);
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

EF_base::EF_base(const char* name, bool rt)
 : CTL_engine(em->STATESET, name, rt, 2)
{
  SetFormal(0, em->MODEL, "m");
  HideFormal(0);
  SetFormal(1, em->STATESET, "p");
}

void EF_base::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);
  const lldsm* llm = getLLM(x, pass[0]);
  stateset* p = grabParam(llm, pass[1], x);
  if (0==p) {
    x.answer->setNull();
    return;
  }
  engtype* eu = p->isExplicit() ? EU_explicit : EU_symbolic;
  result engpass[3];
  engpass[0].setBool(reverse_time);
  engpass[1].setNull();
  engpass[2].setPtr(p);
  launchEngine(eu, engpass, np+1, x);
}

// *****************************************************************
// *                                                               *
// *                               EF                              *
// *                                                               *
// *****************************************************************

class EF_si : public EF_base {
public:
  EF_si();
};

EF_si::EF_si() : EF_base("EF", false)
{
  SetDocumentation("CTL EF operator: build the set of starting states, from which some path has the form:\n~~~~? ---> ? ---> ... ---> p ---> ? ...");
}

// *****************************************************************
// *                                                               *
// *                               EP                              *
// *                                                               *
// *****************************************************************

class EP_si : public EF_base {
public:
  EP_si();
};

EP_si::EP_si() : EF_base("EP", true)
{
  SetDocumentation("CTL EP operator: build the set of final states, to which some path has the form:\n~~~~... ? ---> p ---> ... ---> ? ---> ?");
}

// *****************************************************************
// *                                                               *
// *                            EU_base                            *
// *                                                               *
// *****************************************************************

class EU_base : public CTL_engine {
public:
  EU_base(const char* name, bool rt);
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

EU_base::EU_base(const char* name, bool rt)
 : CTL_engine(em->STATESET, name, rt, 3)
{
  SetFormal(0, em->MODEL, "m");
  HideFormal(0);
  SetFormal(1, em->STATESET, "p");
  SetFormal(2, em->STATESET, "q");
}

void EU_base::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);
  const lldsm* llm = getLLM(x, pass[0]);
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

  engtype* eu = p->isExplicit() ? EU_explicit : EU_symbolic;
  result engpass[3];
  engpass[0].setBool(reverse_time);
  engpass[1].setPtr(p);
  engpass[2].setPtr(q);
  launchEngine(eu, engpass, np, x);
}

// *****************************************************************
// *                                                               *
// *                               EU                              *
// *                                                               *
// *****************************************************************

class EU_si : public EU_base {
public:
  EU_si();
};

EU_si::EU_si() : EU_base("EU", false)
{
  SetDocumentation("CTL EU operator: build the set of starting states, from which some path has the form:\n~~~~p ---> p ---> ... ---> p ---> q ---> ? ...\nwhere q is eventually satisfied (after zero or more states where p is satisfied).");
}

// *****************************************************************
// *                                                               *
// *                               ES                              *
// *                                                               *
// *****************************************************************

class ES_si : public EU_base {
public:
  ES_si();
};

ES_si::ES_si() : EU_base("ES", true)
{
  SetDocumentation("CTL ES operator: build the set of final states, to which some path has the form:\n~~~~... ? ---> q ---> p ---> ... ---> p");
}

// *****************************************************************
// *                                                               *
// *                            EG_base                            *
// *                                                               *
// *****************************************************************

class EG_base : public CTL_engine {
public:
  EG_base(const char* name, bool rt);
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

EG_base::EG_base(const char* name, bool rt)
 : CTL_engine(em->STATESET, name, rt, 2)
{
  SetFormal(0, em->MODEL, "m");
  HideFormal(0);
  SetFormal(1, em->STATESET, "p");
}

void EG_base::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);
  const checkable_lldsm* llm = getLLM(x, pass[0]);
  stateset* p = grabParam(llm, pass[1], x);
  if (0==p) {
    x.answer->setNull();
    return;
  }
  engtype* eg = 0;
  if (llm->isFairModel()) 
    if (p->isExplicit())
      eg = fairEG_explicit;
    else
      eg = fairEG_symbolic;
  else
    if (p->isExplicit())
      eg = unfairEG_explicit;
    else
      eg = unfairEG_symbolic;

  result engpass[2];
  engpass[0].setBool(reverse_time);
  engpass[1].setPtr(p);
  launchEngine(eg, engpass, np, x);
}

// *****************************************************************
// *                                                               *
// *                               EG                              *
// *                                                               *
// *****************************************************************

class EG_si : public EG_base {
public:
  EG_si();
};

EG_si::EG_si() : EG_base("EG", false)
{
  SetDocumentation("CTL EG operator: build the set of starting states, from which some path has the form:\n~~~~p ---> p ---> p ---> p ...\nNote that paths can be finite or infinite in length, and that some models (e.g., Markov chains) do not consider unfair infinite paths.");
}

// *****************************************************************
// *                                                               *
// *                               EH                              *
// *                                                               *
// *****************************************************************

class EH_si : public EG_base {
public:
  EH_si();
};

EH_si::EH_si() : EG_base("EH", true)
{
  SetDocumentation("CTL EH operator: build the set of final states, to which some path has the form:\n~~~~... p ---> p ---> p ---> p\nNote that paths can be finite or infinite in length, and that some models (e.g., Markov chains) do not consider unfair infinite paths.");
}

// *****************************************************************
// *                                                               *
// *                            AX_base                            *
// *                                                               *
// *****************************************************************

class AX_base : public CTL_engine {
public:
  AX_base(const char* name, bool rt);
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

AX_base::AX_base(const char* name, bool rt)
 : CTL_engine(em->STATESET, name, rt, 2)
{
  SetFormal(0, em->MODEL, "m");
  HideFormal(0);
  SetFormal(1, em->STATESET, "p");
}

void AX_base::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);
  const lldsm* llm = getLLM(x, pass[0]);
  stateset* p = grabParam(llm, pass[1], x);
  if (0==p) {
    x.answer->setNull();
    return;
  }
  engtype* ex = p->isExplicit() ? EX_explicit : EX_symbolic;
  result engpass[2];
  engpass[0].setBool(reverse_time);
  engpass[1].setPtr(Complement(em, x.parent, p));
  launchEngine(ex, engpass, np, x);
  stateset* ans = smart_cast <stateset*> (Share(x.answer->getPtr()));
  if (ans) x.answer->setPtr(Complement(em, x.parent, ans));

  // AX p = !EX !p
}

// *****************************************************************
// *                                                               *
// *                               AX                              *
// *                                                               *
// *****************************************************************

class AX_si : public AX_base {
public:
  AX_si();
};

AX_si::AX_si() : AX_base("AX", false)
{
  SetDocumentation("CTL AX operator: build the set of starting states, from which all paths have the form:\n~~~~? ---> p ---> ? ---> ? ...");
}

// *****************************************************************
// *                                                               *
// *                               AY                              *
// *                                                               *
// *****************************************************************

class AY_si : public AX_base {
public:
  AY_si();
};

AY_si::AY_si() : AX_base("AY", true) 
{
  SetDocumentation("CTL AY operator: build the set of final states, to which all paths have the form:\n~~~~... ? ---> ? ---> p ---> ?");
}

// *****************************************************************
// *                                                               *
// *                            AF_base                            *
// *                                                               *
// *****************************************************************

class AF_base : public CTL_engine {
public:
  AF_base(const char* name, bool rt);
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

AF_base::AF_base(const char* name, bool rt)
 : CTL_engine(em->STATESET, name, rt, 2)
{
  SetFormal(0, em->MODEL, "m");
  HideFormal(0);
  SetFormal(1, em->STATESET, "p");
}

void AF_base::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);
  const checkable_lldsm* llm = getLLM(x, pass[0]);
  stateset* p = grabParam(llm, pass[1], x);
  if (0==p) {
    x.answer->setNull();
    return;
  }
  engtype* eg = 0;
  if (llm->isFairModel()) 
    if (p->isExplicit())
      eg = fairEG_explicit;
    else
      eg = fairEG_symbolic;
  else
    if (p->isExplicit())
      eg = unfairEG_explicit;
    else
      eg = unfairEG_symbolic;

  result engpass[2];
  engpass[0].setBool(reverse_time);
  engpass[1].setPtr(Complement(em, x.parent, p));
  launchEngine(eg, engpass, np, x);
  stateset* ans = smart_cast <stateset*> (Share(x.answer->getPtr()));
  if (ans) x.answer->setPtr(Complement(em, x.parent, ans));

  // AF p = !EG !p
}

// *****************************************************************
// *                                                               *
// *                               AF                              *
// *                                                               *
// *****************************************************************

class AF_si : public AF_base {
public:
  AF_si();
};

AF_si::AF_si() : AF_base("AF", false)
{
  SetDocumentation("CTL AF operator: build the set of starting states, from which all paths have the form:\n~~~~? ---> ... ---> ? ---> p ---> ? ...");
}

// *****************************************************************
// *                                                               *
// *                               AP                              *
// *                                                               *
// *****************************************************************

class AP_si : public AF_base {
public:
  AP_si();
};

AP_si::AP_si() : AF_base("AP", true) 
{
  SetDocumentation("CTL AP operator: build the set of final states, to which all paths have the form:\n~~~~... ? ---> p ---> ? ---> ... ---> ?");
}

// *****************************************************************
// *                                                               *
// *                            AG_base                            *
// *                                                               *
// *****************************************************************

class AG_base : public CTL_engine {
public:
  AG_base(const char* name, bool rt);
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

AG_base::AG_base(const char* name, bool rt)
 : CTL_engine(em->STATESET, name, rt, 2)
{
  SetFormal(0, em->MODEL, "m");
  HideFormal(0);
  SetFormal(1, em->STATESET, "p");
}

void AG_base::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);
  const lldsm* llm = getLLM(x, pass[0]);
  stateset* p = grabParam(llm, pass[1], x);
  if (0==p) {
    x.answer->setNull();
    return;
  }

  engtype* eu = p->isExplicit() ? EU_explicit : EU_symbolic;
  result engpass[3];
  engpass[0].setBool(reverse_time);
  engpass[1].setNull();
  engpass[2].setPtr(Complement(em, x.parent, p));
  launchEngine(eu, engpass, np+1, x);
  stateset* ans = smart_cast <stateset*> (Share(x.answer->getPtr()));
  if (ans) x.answer->setPtr(Complement(em, x.parent, ans));

  // AG p = !EF !p
}

// *****************************************************************
// *                                                               *
// *                               AG                              *
// *                                                               *
// *****************************************************************

class AG_si : public AG_base {
public:
  AG_si();
};

AG_si::AG_si() : AG_base("AG", false)
{
  SetDocumentation("CTL AG operator: build the set of starting states, from which all paths have the form:\n~~~~p ---> p ---> p ---> p ...");
}

// *****************************************************************
// *                                                               *
// *                               AH                              *
// *                                                               *
// *****************************************************************

class AH_si : public AG_base {
public:
  AH_si();
};

AH_si::AH_si() : AG_base("AH", true) 
{
  SetDocumentation("CTL AH operator: build the set of final states, to which all paths have the form:\n~~~~... p ---> p ---> p ---> p");
}

// *****************************************************************
// *                                                               *
// *                            AU_base                            *
// *                                                               *
// *****************************************************************

class AU_base : public CTL_engine {
public:
  AU_base(const char* name, bool rt);
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

AU_base::AU_base(const char* name, bool rt)
 : CTL_engine(em->STATESET, name, rt, 3)
{
  SetFormal(0, em->MODEL, "m");
  HideFormal(0);
  SetFormal(1, em->STATESET, "p");
  SetFormal(2, em->STATESET, "q");
}

void AU_base::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);
  const checkable_lldsm* llm = getLLM(x, pass[0]);
  stateset* notp = Complement(em, x.parent, grabParam(llm, pass[1], x));
  if (0==notp) {
    x.answer->setNull();
    return;
  }
  stateset* notq = Complement(em, x.parent, grabParam(llm, pass[2], x));
  if (0==notq) {
    Delete(notp);
    x.answer->setNull();
    return;
  }
  if (mismatched(notp, notq, x)) {
    Delete(notp);
    Delete(notq);
    x.answer->setNull();
    return;
  }

  // set engines
  engtype* eu = 0;
  engtype* eg = 0;

  if (notp->isExplicit()) {
    eu = EU_explicit;
    eg = llm->isFairModel() ? fairEG_explicit : unfairEG_explicit;
  } else {
    eu = EU_symbolic;
    eg = llm->isFairModel() ? fairEG_symbolic : unfairEG_symbolic;
  }

  // A p U q = !E[ !q U (!p & !q) ] & !EG(!q)
  result engpass[3];
  engpass[0].setBool(reverse_time);
  engpass[1].setPtr(notq);
  engpass[2].setPtr(Intersection(em, x.parent, notp, notq));
  Delete(notp);
  launchEngine(eu, engpass, 3, x);

  stateset* eupart = 0;  
  if (x.answer->isNormal()) {
    eupart = smart_cast <stateset*> (Share(x.answer->getPtr()));
    DCASSERT(eupart);
    eupart = Complement(em, x.parent, eupart);
  }
  // eupart is !E[ !q U (!p & !q) ]

  launchEngine(eg, engpass, 2, x);
  stateset* egpart = 0;
  if (x.answer->isNormal()) {
    egpart = smart_cast <stateset*> (Share(x.answer->getPtr()));
    DCASSERT(egpart);
    egpart = Complement(em, x.parent, egpart);
  }
  // egpart is !EG(!q)

  stateset* answer = Intersection(em, x.parent, eupart, egpart);
  Delete(eupart);
  Delete(egpart);

  if (0==answer) x.answer->setNull();
  else           x.answer->setPtr(answer);
}

// *****************************************************************
// *                                                               *
// *                               AU                              *
// *                                                               *
// *****************************************************************

class AU_si : public AU_base {
public:
  AU_si();
};

AU_si::AU_si() : AU_base("AU", false)
{
  SetDocumentation("CTL AU operator: build the set of starting states, from which all paths have the form:\n~~~~p ---> p ---> ... ---> p ---> q ---> ? ...\nwhere q is eventually satisfied (after zero or more states where p is satisfied).");
}

// *****************************************************************
// *                                                               *
// *                               AS                              *
// *                                                               *
// *****************************************************************

class AS_si : public AU_base {
public:
  AS_si();
};

AS_si::AS_si() : AU_base("AS", true)
{
  SetDocumentation("CTL AS operator: build the set of final states, to which all paths have the form:\n~~~~... ? ---> q ---> p ---> ... ---> p");
}


// *****************************************************************
// *                                                               *
// *                              AEF                              *
// *                                                               *
// *****************************************************************

class AEF_si : public CTL_engine {
public:
  AEF_si();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

AEF_si::AEF_si()
 : CTL_engine(em->STATESET, "AEF", false, 3)
{
  SetFormal(0, em->MODEL, "m");
  HideFormal(0);
  SetFormal(1, em->STATESET, "p");
  SetFormal(2, em->STATESET, "q");
  SetDocumentation("AEF operator: build set of source states, from which we can guarantee that we reach a state in q.  For states in p, we can choose the next state; otherwise we cannot.");
}

void AEF_si::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);
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

  engtype* aef = p->isExplicit() ? unfairAEF_explicit : unfairAEF_symbolic;
  result engpass[3];
  engpass[0].setPtr(Share(llm));
  engpass[1].setPtr(p);
  engpass[2].setPtr(q);
  launchEngine(aef, engpass, 3, x);
}

// *****************************************************************
// *                                                               *
// *                           num_paths                           *
// *                                                               *
// *****************************************************************

class num_paths : public CTL_engine {
public:
  num_paths();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

num_paths::num_paths()
: CTL_engine(em->BIGINT, "num_paths", false, 3)
{
  SetFormal(0, em->MODEL, "m");
  HideFormal(0);
  SetFormal(1, em->STATESET, "src");
  SetFormal(2, em->STATESET, "dest");
  SetDocumentation("Count the number of distinct paths from src states to dest states.  Will be infinite if any of these paths contains a cycle.");
}

void num_paths::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);
  checkable_lldsm* llm = getLLM(x, pass[0]);
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

  DCASSERT(p->isExplicit());
  DCASSERT(q->isExplicit());
  llm->countPaths(p->getExplicit(), q->getExplicit(), *x.answer);
  Delete(p);
  Delete(q);
}

// ******************************************************************
// *                                                                *
// *                           front  end                           *
// *                                                                *
// ******************************************************************

void InitCTLMeasureFuncs(symbol_table* st, exprman* em, List <msr_func> *common)
{
  //
  // CTL help topic
  //
  help_group* ctl_help = new help_group(
    "CTL", "Computation Tree Logic",
    "A CTL formula phi may be checked by constructing the set of states satisfying phi.  This is done by splitting the formula into quantifier, operator pairs and using the appropriate function.  The set of initial states are then compared to the set of states satisfying phi to determine if the model satisfies phi.  Note that both \"forward time\" and \"reverse time\" temporal operators are supported."
  );

  if (st) st->AddSymbol(ctl_help);

  //
  // Initialize engines
  //
  CTL_engine::EX_explicit = MakeEngineType(em,
      "ExplicitEX",
      "Algorithm for explicit computation of EX formulas in CTL.",
      engtype::FunctionCall
  );

  CTL_engine::EX_symbolic = MakeEngineType(em,
      "SymbolicEX",
      "Algorithm for symbolic (MDD-based) computation of EX formulas in CTL.",
      engtype::FunctionCall
  );

  CTL_engine::EU_explicit = MakeEngineType(em,
      "ExplicitEU",
      "Algorithm for explicit computation of EU formulas in CTL.",
      engtype::FunctionCall
  );

  CTL_engine::EU_symbolic = MakeEngineType(em,
      "SymbolicEU",
      "Algorithm for symbolic (MDD-based) computation of EU formulas in CTL.",
      engtype::FunctionCall
  );

  CTL_engine::unfairEG_explicit = MakeEngineType(em,
      "ExplicitUnfairEG",
      "Algorithm for explicit computation of EG formulas in CTL, ignoring fairness.",
      engtype::FunctionCall
  );

  CTL_engine::unfairEG_symbolic = MakeEngineType(em,
      "SymbolicUnfairEG",
      "Algorithm for symbolic (MDD-based) computation of EG formulas in CTL, ignoring fairness.",
      engtype::FunctionCall
  );

  CTL_engine::fairEG_explicit = MakeEngineType(em,
      "ExplicitFairEG",
      "Algorithm for explicit computation of EG formulas in CTL, with fairness.",
      engtype::FunctionCall
  );

  CTL_engine::fairEG_symbolic = MakeEngineType(em,
      "SymbolicFairEG",
      "Algorithm for symbolic (MDD-based) computation of EG formulas in CTL, with fairness.",
      engtype::FunctionCall
  );

  CTL_engine::unfairAEF_explicit = MakeEngineType(em,
      "ExplicitUnfairAEF",
      "Algorithm for explicit computation of AEF formulas in (extended) CTL, without fairness.",
      engtype::FunctionCall
  );

  CTL_engine::unfairAEF_symbolic = MakeEngineType(em,
      "SymbolicUnfairAEF",
      "Algorithm for symbolic (MDD-based) computation of AEF formulas in (extended) CTL, without fairness.",
      engtype::FunctionCall
  );

  CTL_engine::ProcGen = em->findEngineType("ProcessGeneration");

  //
  // Declare function variables
  //
  static msr_func* the_EX_si = 0;
  static msr_func* the_EY_si = 0;
  static msr_func* the_EF_si = 0;
  static msr_func* the_EP_si = 0;
  static msr_func* the_EU_si = 0;
  static msr_func* the_ES_si = 0;
  static msr_func* the_EG_si = 0;
  static msr_func* the_EH_si = 0;
  static msr_func* the_AX_si = 0;
  static msr_func* the_AY_si = 0;
  static msr_func* the_AF_si = 0;
  static msr_func* the_AP_si = 0;
  static msr_func* the_AU_si = 0;
  static msr_func* the_AS_si = 0;
  static msr_func* the_AG_si = 0;
  static msr_func* the_AH_si = 0;
  static msr_func* the_AEF_si = 0;
  static msr_func* the_num_paths = 0;

  //
  // Initialize functions
  //
  if (!the_EX_si)     the_EX_si     = new EX_si;
  if (!the_EY_si)     the_EY_si     = new EY_si;
  if (!the_EF_si)     the_EF_si     = new EF_si;
  if (!the_EP_si)     the_EP_si     = new EP_si;
  if (!the_EU_si)     the_EU_si     = new EU_si;
  if (!the_ES_si)     the_ES_si     = new ES_si;
  if (!the_EG_si)     the_EG_si     = new EG_si;
  if (!the_EH_si)     the_EH_si     = new EH_si;
  if (!the_AX_si)     the_AX_si     = new AX_si;
  if (!the_AY_si)     the_AY_si     = new AY_si;
  if (!the_AF_si)     the_AF_si     = new AF_si;
  if (!the_AP_si)     the_AP_si     = new AP_si;
  if (!the_AU_si)     the_AU_si     = new AU_si;
  if (!the_AS_si)     the_AS_si     = new AS_si;
  if (!the_AG_si)     the_AG_si     = new AG_si;
  if (!the_AH_si)     the_AH_si     = new AH_si;
  if (!the_AEF_si)    the_AEF_si    = new AEF_si;
  if (!the_num_paths) the_num_paths = new num_paths;

  //
  // Add functions to help topic
  //
  ctl_help->addFunction(the_EX_si);
  ctl_help->addFunction(the_EY_si);
  ctl_help->addFunction(the_EF_si);
  ctl_help->addFunction(the_EP_si);
  ctl_help->addFunction(the_EU_si);
  ctl_help->addFunction(the_ES_si);
  ctl_help->addFunction(the_EG_si);
  ctl_help->addFunction(the_EH_si);

  ctl_help->addFunction(the_AX_si);
  ctl_help->addFunction(the_AY_si);
  ctl_help->addFunction(the_AF_si);
  ctl_help->addFunction(the_AP_si);
  ctl_help->addFunction(the_AU_si);
  ctl_help->addFunction(the_AS_si);
  ctl_help->addFunction(the_AG_si);
  ctl_help->addFunction(the_AH_si);

  ctl_help->addFunction(the_AEF_si);
  ctl_help->addFunction(the_num_paths);

  //
  // Add functions to measure table
  //
  if (0==common) return;

  common->Append(the_EX_si);
  common->Append(the_EY_si);
  common->Append(the_EF_si);
  common->Append(the_EP_si);
  common->Append(the_EU_si);
  common->Append(the_ES_si);
  common->Append(the_EG_si);
  common->Append(the_EH_si);

  common->Append(the_AX_si);
  common->Append(the_AY_si);
  common->Append(the_AF_si);
  common->Append(the_AP_si);
  common->Append(the_AU_si);
  common->Append(the_AS_si);
  common->Append(the_AG_si);
  common->Append(the_AH_si);

  common->Append(the_AEF_si);
  common->Append(the_num_paths);
}

