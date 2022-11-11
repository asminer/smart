
#include "ctl_msr.h"
#include "../ExprLib/startup.h"
#include "../ExprLib/engine.h"
#include "../ExprLib/measures.h"
#include "../ExprLib/mod_def.h"
#include "../ExprLib/mod_vars.h"
#include "../ExprLib/values.h"
#include "graph_llm.h"

#include "../Modules/biginttype.h"
#include "../Modules/statesets.h"

#include "../SymTabs/symtabs.h"

#include "../ParseSM/parse_sm.h"
extern parse_module* pm;

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

  template <typename T>
  inline static void setAnswer(traverse_data &x, T* s) {
    if (s) {
      x.answer->setPtr(s);
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
  SetDocumentation("CTL EG operator: build the set of starting states, from which some path has the form:\n~~~~p ---> p ---> p ---> p ...\nNote that paths can be finite (if the last state is a deadlocked state) or infinite in length, and that some models (e.g., Markov chains) do not consider unfair infinite paths.");
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
  SetDocumentation("CTL EH operator: build the set of final states, to which some path has the form:\n~~~~... p ---> p ---> p ---> p\nNote that paths can be finite (if the first state is an initial state) or infinite in length, and that some models (e.g., Markov chains) do not consider unfair infinite paths.");
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
  stateset* p = grabParam(llm, pass[1], x);
  setAnswer(x, llm->AX(revTime(), p));
  Delete(p);
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
  stateset* p = grabParam(llm, pass[1], x);
  setAnswer(x,
    llm->isFairModel() ?  llm->fairAU(revTime(), 0, p) :  llm->unfairAU(revTime(), 0, p)
  );
  Delete(p);
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
  stateset* p = grabParam(llm, pass[1], x);
  setAnswer(x, llm->AG(revTime(), p));
  Delete(p);
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
  stateset* p = grabParam(llm, pass[1], x);
  if (0==p) {
    x.answer->setNull();
    return;
  }
  stateset* q = grabParam(llm, pass[2], x);
  setAnswer(x,
    llm->isFairModel() ?  llm->fairAU(revTime(), p, q) :  llm->unfairAU(revTime(), p, q)
  );
  Delete(p);
  Delete(q);
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

// *****************************************************************
// *                                                               *
// *                            states                             *
// *                                                               *
// *****************************************************************

class states : public CTL_engine {
public:
  states();
  virtual int Traverse(traverse_data &x, expr** pass, int np);
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

states::states()
: CTL_engine(em->STATESET, "states", false, 2)
{
  SetFormal(1, em->TEMPORAL, "formula");
  SetDocumentation("Compute the stateset satisfying the given temporal formula.");
}

int states::Traverse(traverse_data &x, expr** pass, int np)
{
  if (traverse_data::Substitute != x.which) {
    return simple_internal::Traverse(x, pass, np);
  }

  traverse_data xx(traverse_data::TemporalStateSet);
  xx.model = x.model;
  result ans;
  xx.answer = &ans;
  pass[1]->Traverse(xx);

  x.answer->setPtr(Share(xx.answer->getPtr()));
  return 1;
}

void states::Compute(traverse_data &x, expr** pass, int np)
{
  // Should not reach here
  throw subengine::No_Engine;
}

// *****************************************************************
// *                                                               *
// *                           CTL_trace                           *
// *                                                               *
// *****************************************************************

class CTL_trace : public CTL_engine {
protected:
  class CTL_trace_ex : public CTL_engine {
  public:
    CTL_trace_ex(const char* name, bool rt, int np)
      : CTL_engine(em->TRACE, name, rt, np)
    {
    }

    trace_data* grabTraceData(const lldsm* m, expr* p, traverse_data &x) const {
      if (nullptr == m || nullptr == p) return nullptr;
      p->Compute(x);
      if (!x.answer->isNormal()) return nullptr;
      return Share(dynamic_cast<trace_data*>(x.answer->getPtr()));
    }

    stateset* grabInitialState(traverse_data& x) const {
      stateset* ss = dynamic_cast<stateset*>(x.answer->getPtr());
      if (nullptr == ss) {
        // TODO: To be implemented
        return nullptr;
      }
      return Share(ss);
    }

    trace* grabTrace(traverse_data& x) const {
      trace* t = dynamic_cast<trace*>(x.answer->getPtr());
      if (nullptr == t) {
        // TODO: To be implemented
        return nullptr;
      }
      return t;
    }

    trace* buildTrace(List<stateset>* ss) const {
      trace* t = new trace();
      for (int i = 0; i < ss->Length(); i++) {
        stateset* s = ss->Item(i);
        shared_state* st = s->getSingleState();
        t->Append(st);
      }
      return t;
    }
  };

public:
  CTL_trace(const char* name, bool rt, int np);

  virtual int Traverse(traverse_data &x, expr** pass, int np);
};

CTL_trace::CTL_trace(const char* name, bool rt, int np)
  : CTL_engine(em->STATESET, name, rt, np)
{
}

int CTL_trace::Traverse(traverse_data &x, expr** pass, int np)
{
  if (traverse_data::Substitute != x.which) {
    return CTL_engine::Traverse(x, pass, np);
  }

  // Trace functions won't be measurified
  expr* comp = em->makeFunctionCall(
          x.parent ? x.parent->Where() : location::NOWHERE(), this, pass, np);
  setAnswer(x, comp);
  return 1;
}

// *****************************************************************
// *                                                               *
// *                           And_trace_si                        *
// *                                                               *
// *****************************************************************

class And_trace_si : public CTL_trace {
protected:
  class And_trace_ex : public CTL_trace_ex {
  public:
    And_trace_ex()
      : CTL_trace_ex("And_trace_ex", false, 5)
    {
      SetFormal(1, em->STATESET, "the states satisfying the left sub-formula");
      SetFormal(2, em->TRACE, "the callback function of witness generation for the left sub-formula.");
      SetFormal(3, em->STATESET, "the states satisfying the right sub-formula");
      SetFormal(4, em->TRACE, "the callback function of witness generation for the right sub-formula.");
    }

    virtual void Compute(traverse_data &x, expr** pass, int np);
  };

  And_trace_ex the_and_trace_ex;

public:
  And_trace_si();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

void And_trace_si::And_trace_ex::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(np == formals.getLength());

  // The initial state
  stateset* p = grabInitialState(x);

  List<stateset> ans;
  ans.Append(Share(p));
  DCASSERT(ans.Length() > 0);

  trace* t = buildTrace(&ans);

  // XXX: Assume the left operand is an atomic proposition
  DCASSERT(nullptr == pass[2]);
  if (nullptr != pass[2]) {
    // Set the initial state for the left sub-formula
    setAnswer(x, Share(p));
    pass[2]->Compute(x);
    trace* st = grabTrace(x);
    t->Concatenate(t->Length() - 1, st);
  }

  if (nullptr != pass[4]) {
    // Set the initial state for the left sub-formula
    setAnswer(x, Share(p));
    pass[4]->Compute(x);
    trace* st = grabTrace(x);
    t->Concatenate(t->Length() - 1, st);
  }

  setAnswer(x, t);

  Delete(p);
  for (int i = 0; i < ans.Length(); i++) {
    Delete(ans.Item(i));
  }
}

And_trace_si::And_trace_si()
 : CTL_trace("And_trace", false, 3)
{
  SetFormal(1, em->STATESET, "states satisfying the left sub-formula");
  SetFormal(2, em->STATESET, "states satisfying the right sub-formula");
}

void And_trace_si::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);

  const graph_lldsm* llm = getLLM(x, pass[0]);

  x.the_callback = nullptr;
  // The states satisfying the left sub-formula
  // XXX: Assume the left operand is an atomic proposition
  stateset* left = grabParam(llm, pass[1], x);
  if (pm->computeMinimumTrace()) {
    // Attach a weight to each state
    stateset* wp = llm->attachWeight(left);
    Delete(left);
    left = wp;
  }
  expr* left_cb = const_cast<expr*>(x.the_callback);

  x.the_callback = nullptr;
  // The states satisfying the right sub-formula
  stateset* right = grabParam(llm, pass[2], x);
  expr* right_cb = const_cast<expr*>(x.the_callback);

  stateset* ans = left->DeepCopy();
  ans->Plus(this, right);
  if (pm->computeMinimumTrace()) {
    ans->Offset(-1);
  }

  // TODO: Potential memory leakage
  const int nps = 5;
  expr** ps = new expr*[nps];
  ps[0] = pass[0];
  ps[1] = new value(Where(), em->STATESET, result(left));
  ps[2] = left_cb;
  ps[3] = new value(Where(), em->STATESET, result(right));
  ps[4] = right_cb;

  expr* fc = em->makeFunctionCall(Where(), &the_and_trace_ex, ps, nps);
  setAnswer(x, ans);
  x.the_callback = fc;
}

// *****************************************************************
// *                                                               *
// *                          EX_trace_si                          *
// *                                                               *
// *****************************************************************

class EX_trace_si : public CTL_trace {
protected:
  class EX_trace_ex : public CTL_trace_ex {
  public:
    EX_trace_ex()
      : CTL_trace_ex("EX_trace_ex", false, 4)
    {
      SetFormal(1, em->STATESET, "the states satisfying the sub-formula");
      SetFormal(2, em->VOID,     "trace data");
      SetFormal(3, em->TRACE,    "the callback function of witness generation for the sub-formula");
    }

    virtual void Compute(traverse_data &x, expr** pass, int np);
  };

  EX_trace_ex the_EX_trace_ex;

public:
  EX_trace_si();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

void EX_trace_si::EX_trace_ex::Compute(traverse_data &x, expr** pass, int np)
{
  // The initial state
  stateset* p = grabInitialState(x);
  const graph_lldsm* llm = getLLM(x, pass[0]);
  // The states satisfying the sub-formula
  stateset* q = grabParam(llm, pass[1], x);
  trace_data* td = grabTraceData(llm, pass[2], x);

  List<stateset> ans;
  llm->traceEX(revTime(), p, td, &ans);
  DCASSERT(ans.Length() == 2);

  trace* t = buildTrace(&ans);

  if (nullptr != pass[3]) {
    // Set the initial state for the sub-formula
    setAnswer(x, Share(ans.Item(ans.Length() - 1)));
    pass[3]->Compute(x);
    trace* st = grabTrace(x);
    t->Concatenate(1, st);
  }

  setAnswer(x, t);

  Delete(p);
  Delete(q);
  for (int i = 0; i < ans.Length(); i++) {
    Delete(ans.Item(i));
  }
}

EX_trace_si::EX_trace_si()
 : CTL_trace("EX_trace", false, 2)
{
  SetFormal(1, em->STATESET, "states satisfying the sub-formula");
}

void EX_trace_si::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);

  const graph_lldsm* llm = getLLM(x, pass[0]);
  x.the_callback = nullptr;
  // The states satisfying the sub-formula
  stateset* p = grabParam(llm, pass[1], x);
  trace_data* td = llm->makeTraceData();
  // The states satisfying the EX formula
  stateset* ans = llm->EX(revTime(), p, td);

  // TODO: Potential memory leakage
  const int nps = 4;
  expr** ps = new expr*[nps];
  ps[0] = pass[0];
  ps[1] = new value(Where(), em->STATESET, result(p));
  ps[2] = new value(Where(), em->VOID, result(td));
  ps[3] = const_cast<expr*>(x.the_callback);

  expr* fc = em->makeFunctionCall(Where(), &the_EX_trace_ex, ps, nps);
  setAnswer(x, ans);
  x.the_callback = fc;
}

// *****************************************************************
// *                                                               *
// *                          EF_trace_si                          *
// *                                                               *
// *****************************************************************

class EF_trace_si : public CTL_trace {
protected:
  class EF_trace_ex : public CTL_trace_ex {
  public:
    EF_trace_ex()
      : CTL_trace_ex("EF_trace_ex", false, 4)
    {
      SetFormal(1, em->STATESET, "the states satisfying the subformula");
      SetFormal(2, em->VOID,     "trace data");
      SetFormal(3, em->TRACE,    "the callback function of witness generation for the sub-formula");
    }

    virtual void Compute(traverse_data &x, expr** pass, int np);
  };

  EF_trace_ex the_EF_trace_ex;

public:
  EF_trace_si();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

void EF_trace_si::EF_trace_ex::Compute(traverse_data &x, expr** pass, int np)
{
  // The initial state
  stateset* p = grabInitialState(x);
  const graph_lldsm* llm = getLLM(x, pass[0]);
  // The states satisfying the sub-formula
  stateset* q = grabParam(llm, pass[1], x);
  trace_data* td = grabTraceData(llm, pass[2], x);

  List<stateset> ans;
  llm->traceEU(revTime(), p, td, &ans);
  DCASSERT(ans.Length() > 0);

  trace* t = buildTrace(&ans);

  if (nullptr != pass[3]) {
    // Set the initial state for the sub-formula
    setAnswer(x, Share(ans.Item(ans.Length() - 1)));
    pass[3]->Compute(x);
    trace* st = grabTrace(x);
    t->Concatenate(t->Length() - 1, st);
  }

  setAnswer(x, t);

  Delete(p);
  Delete(q);
  for (int i = 0; i < ans.Length(); i++) {
    Delete(ans.Item(i));
  }
}

EF_trace_si::EF_trace_si()
 : CTL_trace("EF_trace", false, 2)
{
  SetFormal(1, em->STATESET, "states satisfying the sub-formula");
}

void EF_trace_si::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);

  const graph_lldsm* llm = getLLM(x, pass[0]);
  x.the_callback = nullptr;
  // The states satisfying the sub-formula
  stateset* p = grabParam(llm, pass[1], x);
  trace_data* td = llm->makeTraceData();
  // The states satisfying the EF formula
  stateset* ans = llm->EU(revTime(), nullptr, p, td);

  // TODO: Potential memory leakage
  const int nps = 4;
  expr** ps = new expr*[nps];
  ps[0] = pass[0];
  ps[1] = new value(Where(), em->STATESET, result(p));
  ps[2] = new value(Where(), em->VOID, result(td));
  ps[3] = const_cast<expr*>(x.the_callback);

  expr* fc = em->makeFunctionCall(Where(), &the_EF_trace_ex, ps, nps);
  setAnswer(x, ans);
  x.the_callback = fc;
}

// *****************************************************************
// *                                                               *
// *                          EG_trace_si                          *
// *                                                               *
// *****************************************************************

class EG_trace_si : public CTL_trace {
protected:
  class EG_trace_ex : public CTL_trace_ex {
  public:
    EG_trace_ex()
      : CTL_trace_ex("EG_trace_ex", false, 4)
    {
      SetFormal(1, em->STATESET, "the states satisfying the sub-formula");
      SetFormal(2, em->VOID,     "trace data");
      SetFormal(3, em->TRACE,    "the callback function of witness generation for the sub-formula");
    }

    virtual void Compute(traverse_data &x, expr** pass, int np);
  };

  EG_trace_ex the_EG_trace_ex;

public:
  EG_trace_si();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

void EG_trace_si::EG_trace_ex::Compute(traverse_data &x, expr** pass, int np)
{
  // The initial state
  stateset* p = grabInitialState(x);
  const graph_lldsm* llm = getLLM(x, pass[0]);
  // The states satisfying the sub-formula
  stateset* q = grabParam(llm, pass[1], x);
  trace_data* td = grabTraceData(llm, pass[2], x);

  List<stateset> ans;
  llm->traceEG(revTime(), p, td, &ans);
  DCASSERT(ans.Length() >= 2);

  trace* t = buildTrace(&ans);

  if (nullptr != pass[3]) {
    for (int i = 0; i < t->Length() - 1; i++) {
      // Set the initial state for the sub-formula
      setAnswer(x, Share(ans.Item(i)));
      pass[3]->Compute(x);
      trace* st = grabTrace(x);
      t->Concatenate(i, st);
    }
  }

  setAnswer(x, t);

  Delete(p);
  Delete(q);
  for (int i = 0; i < ans.Length(); i++) {
    Delete(ans.Item(i));
  }
}

EG_trace_si::EG_trace_si()
 : CTL_trace("EG_trace", false, 2)
{
  SetFormal(1, em->STATESET, "states satisfying the sub-formula");
}

void EG_trace_si::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);

  const graph_lldsm* llm = getLLM(x, pass[0]);
  x.the_callback = nullptr;
  // The states satisfying the sub-formula
  stateset* p = grabParam(llm, pass[1], x);
  trace_data* td = llm->makeTraceData();
  // The states satisfying the EG formula
  stateset* ans = llm->unfairEG(revTime(), p, td);

  // TODO: Potential memory leakage
  const int nps = 4;
  expr** ps = new expr*[nps];
  ps[0] = pass[0];
  ps[1] = new value(Where(), em->STATESET, result(p));
  ps[2] = new value(Where(), em->VOID, result(td));
  ps[3] = const_cast<expr*>(x.the_callback);

  expr* fc = em->makeFunctionCall(Where(), &the_EG_trace_ex, ps, nps);
  setAnswer(x, ans);
  x.the_callback = fc;
}

// *****************************************************************
// *                                                               *
// *                          EU_trace_si                          *
// *                                                               *
// *****************************************************************

class EU_trace_si : public CTL_trace {
protected:
  class EU_trace_ex : public CTL_trace_ex {
  public:
    EU_trace_ex()
      : CTL_trace_ex("EU_trace_ex", false, 5)
    {
      // E p U q
      SetFormal(1, em->STATESET, "the states satisfying the subformula");
      SetFormal(2, em->VOID,     "trace data");
      SetFormal(3, em->TRACE,    "the callback function of witness generation for p-sub-formula");
      SetFormal(4, em->TRACE,    "the callback function of witness generation for q-sub-formula");
    }

    virtual void Compute(traverse_data &x, expr** pass, int np);
  };

  EU_trace_ex the_EG_trace_ex;

public:
  EU_trace_si();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

void EU_trace_si::EU_trace_ex::Compute(traverse_data &x, expr** pass, int np)
{
  // The initial state
  stateset* p = grabInitialState(x);
  const graph_lldsm* llm = getLLM(x, pass[0]);
  // The states satisfying the sub-formula
  stateset* q = grabParam(llm, pass[1], x);
  trace_data* td = grabTraceData(llm, pass[2], x);

  List<stateset> ans;
  llm->traceEU(revTime(), p, td, &ans);
  DCASSERT(ans.Length() >= 1);

  trace* t = buildTrace(&ans);

  if (nullptr != pass[3]) {
    for (int i = 0; i < t->Length() - 1; i++) {
      // Set the initial state for the p-sub-formula
      setAnswer(x, Share(ans.Item(i)));
      pass[3]->Compute(x);
      trace* st = grabTrace(x);
      t->Concatenate(i, st);
    }
  }
  if (nullptr != pass[4]) {
    // Set the initial state for the q-sub-formula
    setAnswer(x, Share(ans.Item(t->Length() - 1)));
    pass[4]->Compute(x);
    trace* st = grabTrace(x);
    t->Concatenate(t->Length() - 1, st);
  }

  setAnswer(x, t);

  Delete(p);
  Delete(q);
  for (int i = 0; i < ans.Length(); i++) {
    Delete(ans.Item(i));
  }
}

EU_trace_si::EU_trace_si()
 : CTL_trace("EU_trace", false, 3)
{
  // E p U q
  SetFormal(1, em->STATESET, "states satisfying the p-sub-formula");
  SetFormal(2, em->STATESET, "states satisfying the q-sub-formula");
}

void EU_trace_si::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);

  const graph_lldsm* llm = getLLM(x, pass[0]);

  x.the_callback = nullptr;
  // The states satisfying the p-sub-formula
  stateset* p = grabParam(llm, pass[1], x);
  expr* pcb = const_cast<expr*>(x.the_callback);

  x.the_callback = nullptr;
  // The states satisfying the q-sub-formula
  stateset* q = grabParam(llm, pass[2], x);
  expr* qcb = const_cast<expr*>(x.the_callback);

  trace_data* td = llm->makeTraceData();
  // The states satisfying the EG formula
  stateset* ans = llm->EU(revTime(), p, q, td);

  // TODO: Potential memory leakage
  const int nps = 5;
  expr** ps = new expr*[nps];
  ps[0] = pass[0];
  ps[1] = new value(Where(), em->STATESET, result(p));
  ps[2] = new value(Where(), em->VOID, result(td));
  ps[3] = pcb;
  ps[4] = qcb;

  expr* fc = em->makeFunctionCall(Where(), &the_EG_trace_ex, ps, nps);
  setAnswer(x, ans);
  x.the_callback = fc;
}

// *****************************************************************
// *                                                               *
// *                            traces                             *
// *                                                               *
// *****************************************************************

class traces : public CTL_engine {
protected:
  class traces_ex : public CTL_engine {
  public:
    traces_ex()
      : CTL_engine(em->TRACE, "traces_ex", false, 3)
    {
      SetFormal(1, em->STATESET, "initial_states");
      SetFormal(2, em->STATESET, "states satisfying the temporal formula");
    }

    virtual void Compute(traverse_data &x, expr** pass, int np);
  };

  traces_ex the_trace_ex;

public:
  traces();
  virtual int Traverse(traverse_data &x, expr** pass, int np);
};

traces::traces()
: CTL_engine(em->TRACE, "traces", false, 3)
{
  SetFormal(1, em->STATESET, "initial_states");
  SetFormal(2, em->TEMPORAL, "formula");
  SetDocumentation("Compute a trace verifying the given temporal formula.");
}

int traces::Traverse(traverse_data &x, expr** pass, int np)
{
  if (traverse_data::Substitute != x.which) {
    return CTL_engine::Traverse(x, pass, np);
  }

  traverse_data xx(traverse_data::TemporalTrace);
  xx.model = x.model;
  result ans;
  xx.answer = &ans;
  pass[2]->Traverse(xx);

  const int newnp = 3;
  expr** newpass = new expr*[newnp];
  newpass[0] = pass[0];
  newpass[1] = pass[1];
  newpass[2] = dynamic_cast<expr*>(Share(xx.answer->getPtr()));

  return the_trace_ex.Traverse(x, newpass, newnp);
}

void traces::traces_ex::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);

  const graph_lldsm* llm = getLLM(x, pass[0]);
  // Initial states
  stateset* p = grabParam(llm, pass[1], x);
  if (pm->computeMinimumTrace()) {
    // Attach a weight to each state
    stateset* wp = llm->attachWeight(p);
    Delete(p);
    p = wp;
  }
  // States satisfying the formula
  stateset* q = grabParam(llm, pass[2], x);

  stateset* r = p->DeepCopy();
  r->Intersect(this, q);

  if (r->isEmpty()) {
    // No initial states satisfying the formula
    x.answer->setNull();
  }
  else {
    r->Select();

    if (nullptr == x.the_callback) {
      x.answer->setPtr(new trace());
    }
    else {
      // The selected initial state is passed in via traverse_data
      setAnswer(x, r);
      expr* callback = const_cast<expr*>(x.the_callback);
      callback->Compute(x);
      x.the_callback = nullptr;
      Delete(callback);
    }
  }

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
  usesResource("procgen");
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
  static msr_func* the_states = 0;

  static msr_func* the_And_trace_si = 0;
  static msr_func* the_EX_trace_si = 0;
  static msr_func* the_EF_trace_si = 0;
  static msr_func* the_EG_trace_si = 0;
  static msr_func* the_EU_trace_si = 0;
  static msr_func* the_traces = 0;

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
  if (!the_states)    the_states    = new states;

  if (!the_And_trace_si)        the_And_trace_si = new And_trace_si;
  if (!the_EX_trace_si)         the_EX_trace_si  = new EX_trace_si;
  if (!the_EF_trace_si)         the_EF_trace_si  = new EF_trace_si;
  if (!the_EG_trace_si)         the_EG_trace_si  = new EG_trace_si;
  if (!the_EU_trace_si)         the_EU_trace_si  = new EU_trace_si;
  if (!the_traces)              the_traces       = new traces;

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

  ctl_help->addFunction(the_states);

  ctl_help->addFunction(the_traces);

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

  CML.Append(the_states);

  CML.Append(the_And_trace_si);
  CML.Append(the_EX_trace_si);
  CML.Append(the_EF_trace_si);
  CML.Append(the_EG_trace_si);
  CML.Append(the_EU_trace_si);
  CML.Append(the_traces);

  return true;
}
