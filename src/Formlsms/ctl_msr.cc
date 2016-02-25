
// $Id$

#include "ctl_msr.h"
#include "../ExprLib/startup.h"
#include "../ExprLib/engine.h"
#include "../ExprLib/measures.h"
#include "graph_llm.h"

#include "../Modules/biginttype.h"
#include "../Modules/statesets.h"

#include "../SymTabs/symtabs.h"

// *****************************************************************
// *                                                               *
// *                           CTL_engine                          *
// *                                                               *
// *****************************************************************

class CTL_engine : public msr_noengine {
public:
  CTL_engine(const type* t, const char* name, bool rt, int np);

protected:
  inline bool revTime() const {
    return reverse_time;
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

  inline graph_lldsm* getLLM(traverse_data &x, expr* p) const {
    model_instance* mi = grabModelInstance(x, p);
    if (0==mi) return 0;
    lldsm* foo = BuildRG(mi->GetCompiledModel(), x.parent);
    if (0==foo) return 0;
    if (foo->Type() == lldsm::Error) return 0;
    return dynamic_cast <graph_lldsm*>(foo);
  }

  inline static stateset* Complement(stateset* p) {
    if (0==p) return 0;
    stateset* NOTp = 0;
    if (p->numRefs() > 1) {
      NOTp = p->DeepCopy();
      NOTp->Complement();
      Delete(p);
    } else {
      NOTp = p;
      NOTp->Complement();
    }
    return NOTp;
  }

  inline stateset* grabParam(const lldsm* m, expr* p, traverse_data &x) const {
    if (0==m || 0==p) return 0;
    p->Compute(x);
    if (!x.answer->isNormal()) return 0;
    stateset* ss = smart_cast <stateset*> (x.answer->getPtr());
    DCASSERT(ss);
    if (ss->getParent() != m) {
      if (em->startError()) {
        em->causedBy(x.parent);
        em->cerr() << "Stateset in " << Name();
        em->cerr() << " expression is from a different model";
        em->stopIO();
      }   
      return 0;
    }
    return Share(ss);
  }

  inline stateset* grabAndInvertParam(const lldsm* m, expr* p, traverse_data &x) const {
    if (0==m || 0==p) return 0;
    p->Compute(x);
    if (!x.answer->isNormal()) return 0;
    stateset* ss = smart_cast <stateset*> (x.answer->getPtr());
    DCASSERT(ss);
    if (ss->getParent() != m) {
      if (em->startError()) {
        em->causedBy(x.parent);
        em->cerr() << "Stateset in " << Name();
        em->cerr() << " expression is from a different model";
        em->stopIO();
      }   
      return 0;
    }
    if (ss->numRefs() > 1) {
      ss = ss->DeepCopy();
      ss->Complement();
    } else {
      ss->Complement();
      Share(ss);
    }
    return ss;
  }

  inline static void setAnswer(traverse_data &x, stateset* s) {
    if (s) {
      x.answer->setPtr(s);
    } else {
      x.answer->setNull();
    }
  }

  inline static void setAndInvertAnswer(traverse_data &x, stateset* s) {
    if (s) {
      if (s->numRefs()>1) {
        stateset* ns = s->DeepCopy();
        Delete(s);
        ns->Complement();
        x.answer->setPtr(ns);
      } else {
        s->Complement();
        x.answer->setPtr(s);
      }
    } else {
      x.answer->setNull();
    }
  }

private:
  static engtype* ProcGen;
 
  friend class init_ctlmsrs;

  bool reverse_time;

};

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
  SetFormal(1, em->STATESET, "p");
}

void EX_base::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);
  const graph_lldsm* llm = getLLM(x, pass[0]);
  stateset* p = grabParam(llm, pass[1], x);
  setAnswer(x, llm->EX(revTime(), p));
  Delete(p);
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
  SetFormal(1, em->STATESET, "p");
}

void EF_base::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);
  const graph_lldsm* llm = getLLM(x, pass[0]);
  stateset* p = grabParam(llm, pass[1], x);
  setAnswer(x, llm->EU(revTime(), 0, p));
  Delete(p);
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
  SetFormal(1, em->STATESET, "p");
  SetFormal(2, em->STATESET, "q");
}

void EU_base::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);
  const graph_lldsm* llm = getLLM(x, pass[0]);
  stateset* p = grabParam(llm, pass[1], x);
  if (0==p) {
    x.answer->setNull();
    return;
  }
  stateset* q = grabParam(llm, pass[2], x);
  setAnswer(x, llm->EU(revTime(), p, q));
  Delete(p);
  Delete(q);
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
  SetFormal(1, em->STATESET, "p");
}

void EG_base::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);
  const graph_lldsm* llm = getLLM(x, pass[0]);
  stateset* p = grabParam(llm, pass[1], x);
  setAnswer(x,
    llm->isFairModel() ?  llm->fairEG(revTime(), p) : llm->unfairEG(revTime(), p)
  );
  Delete(p);
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
  SetFormal(1, em->STATESET, "p");
}

void AX_base::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);
  const graph_lldsm* llm = getLLM(x, pass[0]);
  stateset* NOTp = grabAndInvertParam(llm, pass[1], x);
  setAndInvertAnswer(x, llm->EX(revTime(), NOTp));
  Delete(NOTp);
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
  SetFormal(1, em->STATESET, "p");
}

void AF_base::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);
  const graph_lldsm* llm = getLLM(x, pass[0]);
  stateset* NOTp = grabAndInvertParam(llm, pass[1], x);
  setAndInvertAnswer(x,
    llm->isFairModel() ?  llm->fairEG(revTime(), NOTp) : llm->unfairEG(revTime(), NOTp)
  );
  Delete(NOTp);

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
  SetFormal(1, em->STATESET, "p");
}

void AG_base::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);
  const graph_lldsm* llm = getLLM(x, pass[0]);
  stateset* NOTp = grabAndInvertParam(llm, pass[1], x);
  setAndInvertAnswer(x, llm->EU(revTime(), 0, NOTp));
  Delete(NOTp);

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
  SetFormal(1, em->STATESET, "p");
  SetFormal(2, em->STATESET, "q");
}

void AU_base::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);
  const graph_lldsm* llm = getLLM(x, pass[0]);
  stateset* notp = grabAndInvertParam(llm, pass[1], x);
  if (0==notp) {
    x.answer->setNull();
    return;
  }
  stateset* notq = grabAndInvertParam(llm, pass[2], x);
  if (0==notq) {
    Delete(notp);
    x.answer->setNull();
    return;
  }

  stateset* notpq = 0;
  if (notp->numRefs()>1) {
    notpq = notp->DeepCopy();
    Delete(notp);
  } else {
    notpq = notp;
  }
  notpq->Intersect(pass[1], "AU", notq);

  // we've computed notpq: (!p & !q)

  stateset* eupart = Complement( llm->EU(revTime(), notq, notpq) );
  Delete(notpq);

  // we've computed eupart:  !E[ !q U (!p & !q) ]

  stateset* egpart = Complement(
    llm->isFairModel() ? llm->fairEG(revTime(), notq) : llm->unfairEG(revTime(), notq)
  );
  Delete(notq);

  // we've computed egpart: is !EG(!q)

  // Now finish up, using
  //
  // A p U q = !E[ !q U (!p & !q) ] & !EG(!q)
  //
  // try to be clever about the intersection 
  // and do it "in place" if we can
  //

  if (0==egpart || 0==eupart) {
    Delete(egpart);
    Delete(eupart);
    x.answer->setNull();
    return;
  }

  if (egpart->numRefs() == 1) {
    egpart->Intersect(pass[1], "AU", eupart);
    Delete(eupart);
    x.answer->setPtr(egpart);
    return;
  } 
  if (eupart->numRefs() == 1) {
    eupart->Intersect(pass[1], "AU", egpart);
    Delete(egpart);
    x.answer->setPtr(eupart);
    return;
  } 

  //
  // neither egpart nor eupart can be modified in place
  //
  stateset* answer = egpart->DeepCopy();
  answer->Intersect(pass[1], "AU", eupart);
  Delete(egpart);
  Delete(eupart);
  x.answer->setPtr(answer);
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
  SetFormal(1, em->STATESET, "p");
  SetFormal(2, em->STATESET, "q");
  SetDocumentation("AEF operator: build set of source states, from which we can guarantee that we reach a state in q.  For states in p, we can choose the next state; otherwise we cannot.");
}

void AEF_si::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);
  const graph_lldsm* llm = getLLM(x, pass[0]);
  stateset* p = grabParam(llm, pass[1], x);
  stateset* q = grabParam(llm, pass[2], x);
  setAnswer(x, llm->unfairAEF(false, p, q));
  Delete(p);
  Delete(q);
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
  SetFormal(1, em->STATESET, "src");
  SetFormal(2, em->STATESET, "dest");
  SetDocumentation("Count the number of distinct paths from src states to dest states.  Will be infinite if any of these paths contains a cycle.");
}

void num_paths::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);
  graph_lldsm* llm = getLLM(x, pass[0]);
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

  llm->countPaths(p, q, *x.answer);
  Delete(p);
  Delete(q);
}

// ******************************************************************
// *                                                                *
// *                                                                *
// *                         Initialization                         *
// *                                                                *
// *                                                                *
// ******************************************************************

class init_ctlmsrs : public initializer {
  public:
    init_ctlmsrs();
    virtual bool execute();
};
init_ctlmsrs the_ctlmsr_initializer;

init_ctlmsrs::init_ctlmsrs() : initializer("init_ctlmsrs")
{
  usesResource("em");
  usesResource("st");
  usesResource("statesettype");
  usesResource("biginttype");
  buildsResource("CML");
}

bool init_ctlmsrs::execute()
{
  if (0==em) return false;

  //
  // CTL help topic
  //
  help_group* ctl_help = new help_group(
    "CTL", "Computation Tree Logic",
    "A CTL formula phi may be checked by constructing the set of states satisfying phi.  This is done by splitting the formula into quantifier, operator pairs and using the appropriate function.  The set of initial states are then compared to the set of states satisfying phi to determine if the model satisfies phi.  Note that both \"forward time\" and \"reverse time\" temporal operators are supported."
  );

  if (st) st->AddSymbol(ctl_help);

  CTL_engine::ProcGen = em->findEngineType("ProcessGeneration");
  DCASSERT(CTL_engine::ProcGen);

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
  CML.Append(the_EX_si);
  CML.Append(the_EY_si);
  CML.Append(the_EF_si);
  CML.Append(the_EP_si);
  CML.Append(the_EU_si);
  CML.Append(the_ES_si);
  CML.Append(the_EG_si);
  CML.Append(the_EH_si);

  CML.Append(the_AX_si);
  CML.Append(the_AY_si);
  CML.Append(the_AF_si);
  CML.Append(the_AP_si);
  CML.Append(the_AU_si);
  CML.Append(the_AS_si);
  CML.Append(the_AG_si);
  CML.Append(the_AH_si);

  CML.Append(the_AEF_si);
  CML.Append(the_num_paths);
  return true;
}
