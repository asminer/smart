
// $Id$

#include "stoch_msr.h"
#include "stoch_llm.h"
#include "../ExprLib/engine.h"
#include "../ExprLib/measures.h"
#include "../Modules/statevects.h"

#define EXPERT_BASIC_MSR
#include "basic_msr.h"

// *****************************************************************
// *                           init_dist                           *
// *****************************************************************

class init_dist_si : public proc_noengine {
public:
  init_dist_si();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

init_dist_si::init_dist_si()
 : proc_noengine(Stochastic, em->STATEDIST, "init_dist", 1)
{
  SetFormal(0, em->MODEL, "m");
  HideFormal(0);
  SetDocumentation("Returns the distribution over states at time 0, or null if the model is not stochastic.");
}

void init_dist_si::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);
  model_instance* mi = grabModelInstance(x, pass[0]);
  const lldsm* llm = BuildProc(mi ? mi->GetCompiledModel() : 0, true, x.parent);
  if (0==llm || lldsm::Error == llm->Type()) {
    x.answer->setNull();
    return;
  }
  const stochastic_lldsm* sllm = dynamic_cast <const stochastic_lldsm*>(llm);
  statedist* p0 = sllm ? sllm->getInitialDistribution() : 0;
  if (p0) x.answer->setPtr(p0);
  else    x.answer->setNull();
}

// *******************************************************************
// *                                                                 *
// *                         stoch_msr  class                        *
// *                                                                 *
// *******************************************************************

class stoch_msr : public msr_func {
protected:
  /// Engines for computing steady state averages and probabilities.
  static engtype* SteadyStateAverage;

  /// Engines for computing transient (point) averages and probabilities.
  static engtype* TransientAverage;

  /// Engines for computing steady state accumulated averages.
  static engtype* SteadyStateAccumulated;

  /// Engines for computing transient accumulated averages.
  static engtype* TransientAccumulated;

  friend void InitStochMeasureFuncs(exprman* em, List <msr_func> *common);
public:
  stoch_msr(const type* rettype, const char* name, int np);
};

engtype* stoch_msr::SteadyStateAverage      = 0;
engtype* stoch_msr::TransientAverage        = 0;
engtype* stoch_msr::SteadyStateAccumulated  = 0;
engtype* stoch_msr::TransientAccumulated    = 0;

stoch_msr::stoch_msr(const type* rettype, const char* name, int np)
 : msr_func(Stochastic, rettype, name, np)
{
  SetFormal(0, em->MODEL, "m");
  HideFormal(0);
}

// *******************************************************************
// *                                                                 *
// *                            basess_si                            *
// *                                                                 *
// *******************************************************************

class basess_si : public stoch_msr {
  bool has_arg;
public:
  basess_si(const type* rtype, const char* name, const type* arg);
  virtual measure* buildMeasure(traverse_data &x, expr** pass, int np);
};

basess_si::basess_si(const type* rtype, const char* name, const type* arg)
 : stoch_msr(rtype, name, arg ? 2 : 1)
{
  has_arg = arg;
  if (has_arg) SetFormal(1, arg, "x");
}

measure* basess_si::buildMeasure(traverse_data &x, expr** pass, int np)
{
  if (0==SteadyStateAverage)  return 0;
  DCASSERT(pass);
  if (has_arg) {
    DCASSERT(2==np);
    return new measure(x.parent, SteadyStateAverage, x.model, pass[1]);
  } else {
    DCASSERT(1==np);
    return new measure(x.parent, SteadyStateAverage, x.model, 0);
  }
}

// *******************************************************************
// *                             distss                              *
// *******************************************************************

class distss_si : public basess_si {
public:
  distss_si();
};

distss_si::distss_si() : basess_si(em->STATEDIST, "dist_ss", 0)
{
  SetDocumentation("Computes and returns the steady state distribution.");
}

// ******************************************************************
// *                             avg_ss                             *
// ******************************************************************

class avgss_si : public basess_si {
public:
  avgss_si();
};

avgss_si::avgss_si() : basess_si(em->REAL, "avg_ss", em->REAL->addProc())
{
  SetDocumentation("Computes the expected value of expression x at steady-state.");
}

// *******************************************************************
// *                             prob_ss                             *
// *******************************************************************

class probss_si : public basess_si {
public:
  probss_si();
};

probss_si::probss_si() : basess_si(em->REAL, "prob_ss", em->BOOL->addProc())
{
  SetDocumentation("Computes the probability of expression x at steady-state.");
}

// *******************************************************************
// *                                                                 *
// *                            baseat_si                            *
// *                                                                 *
// *******************************************************************

class baseat_si : public stoch_msr {
  class mymsr : public time_measure {
    expr* attime;
    const char* name;
  public:
    mymsr(const char* n, const expr* e, model_def* p, expr* rhs, expr* t);
    virtual ~mymsr();
    virtual void classifyNow();
  };
  bool has_arg;
public:
  baseat_si(const type* rtype, const char* name, const type* arg);
  virtual measure* buildMeasure(traverse_data &x, expr** pass, int np);
};

baseat_si::mymsr
::mymsr(const char* n, const expr* e, model_def* p, expr* rhs, expr* t)
 : time_measure(e, p, rhs)
{
  name = n;
  DCASSERT(name);
  attime = Share(t);

  // build classification dependencies
  if (0==class_deps) class_deps = new List <symbol>;
  if (attime) {
    attime->BuildSymbolList(traverse_data::GetMeasures, 0, class_deps);
  }
  waitDepList(class_deps);
}

baseat_si::mymsr::~mymsr()
{
  Delete(attime);
}

void baseat_si::mymsr::classifyNow()
{
  DCASSERT(owner);
  // Compute the solution time
  traverse_data y(traverse_data::Compute);
  result foo;
  y.answer = &foo;
  SafeCompute(attime, y);
  bool illegal_time;
  if (foo.isInfinity()) {
    illegal_time = (foo.signInfinity()<0);
  } else if (foo.isNormal()) {
    illegal_time = foo.getReal() < 0;
  } else {
    illegal_time = true;
  }

  if (illegal_time) {
    if (owner->StartError(attime)) {
      em->cerr() << "Bad time: ";
      em->REAL->print(em->cerr(), foo);
      em->cerr() << " for " << name;
      owner->DoneError();
    }
    setClassification(0);
    return;
  }

  if (foo.isInfinity()) {
    setClassification(SteadyStateAverage);
  } else {
    setClassification(TransientAverage, foo.getReal());
  }
}

// ******************************************************************

baseat_si::baseat_si(const type* rtype, const char* name, const type* arg)
: stoch_msr(rtype, name, arg ? 3 : 2)
{
  has_arg = arg;
  if (has_arg) {
    SetFormal(1, arg, "x");
    SetFormal(2, em->REAL, "t");
  } else {
    SetFormal(1, em->REAL, "t");
  }
}

measure* baseat_si::buildMeasure(traverse_data &x, expr** pass, int np)
{
  DCASSERT(pass);
  if (has_arg) {
    DCASSERT(3==np);
    return new mymsr(Name(), x.parent, x.model, pass[1], pass[2]);
  } else {
    DCASSERT(2==np);
    return new mymsr(Name(), x.parent, x.model, 0, pass[1]);
  }
}

// *******************************************************************
// *                             distat                              *
// *******************************************************************

class distat_si : public baseat_si {
public:
  distat_si();
};

distat_si::distat_si() : baseat_si(em->STATEDIST, "dist_at", 0)
{
  SetDocumentation("Computes and returns the distribution at time t.");
}

// ******************************************************************
// *                             avg_at                             *
// ******************************************************************

class avgat_si : public baseat_si {
public:
  avgat_si();
};

avgat_si::avgat_si() : baseat_si(em->REAL, "avg_at", em->REAL->addProc())
{
  SetDocumentation("Computes the expected value of expression x at time t.");
}

// *******************************************************************
// *                             prob_at                             *
// *******************************************************************

class probat_si : public baseat_si {
public:
  probat_si();
};

probat_si::probat_si() : baseat_si(em->REAL, "prob_at", em->BOOL->addProc())
{
  SetDocumentation("Computes the probability of expression x at time t.");
}

// *******************************************************************
// *                                                                 *
// *                            baseacc_si                           *
// *                                                                 *
// *******************************************************************

class baseacc_si : public stoch_msr {
  class mymsr : public time_measure {
    expr* acct1;
    expr* acct2;
    const char* name;
  public:
    mymsr(const char* n, const expr* e, model_def* p, expr* rhs, expr* t1, expr* t2);
    virtual ~mymsr();
    virtual void classifyNow();
  };
public:
  baseacc_si(const char* name, const type* arg);
  virtual measure* buildMeasure(traverse_data &x, expr** pass, int np);
};

baseacc_si::mymsr::mymsr(const char* n, const expr* e, model_def* p, 
    expr* rhs, expr* t1, expr* t2) : time_measure(e, p, rhs)
{
  name = n;
  DCASSERT(name);
  acct1 = Share(t1);
  acct2 = Share(t2);

  // build classification dependencies
  if (0==class_deps) class_deps = new List <symbol>;
  if (acct1) acct1->BuildSymbolList(traverse_data::GetMeasures, 0, class_deps);
  if (acct2) acct2->BuildSymbolList(traverse_data::GetMeasures, 0, class_deps);
  waitDepList(class_deps);
}

baseacc_si::mymsr::~mymsr()
{
  Delete(acct1);
  Delete(acct2);
}

void baseacc_si::mymsr::classifyNow()
{
  // Compute the solution times
  traverse_data y(traverse_data::Compute);
  result t1, t2;
  y.answer = &t1;
  SafeCompute(acct1, y);
  y.answer = &t2;
  SafeCompute(acct2, y);
  bool illegal_time;

  // Check t1
  if (t1.isNormal()) {
    illegal_time = t1.getReal() < 0;
  } else {
    illegal_time = true;
  }
  if (illegal_time) {
    if (owner->StartError(acct1)) {
      em->cerr() << "Bad time: ";
      em->REAL->print(em->cerr(), t1);
      em->cerr() << " for parameter t1 in " << name;
      owner->DoneError();
    }
    setClassification(0);
    return;
  }

  // Check t2
  if (t2.isInfinity()) {
    illegal_time = (t2.signInfinity()<0);
  } else if (t2.isNormal()) {
    illegal_time = t2.getReal() < 0;
  } else {
    illegal_time = true;
  }
  if (illegal_time) {
    if (owner->StartError(acct2)) {
      em->cerr() << "Bad time: ";
      em->REAL->print(em->cerr(), t2);
      em->cerr() << " for parameter t2 in " << name;
      owner->DoneError();
    }
    setClassification(0);
    return;
  }

  // Check t1 <= t2
  if (t2.isNormal() && t2.getReal() < t1.getReal()) {
    if (owner->StartError(acct2)) {
      em->cerr() << "Times not in order: t1=";
      em->REAL->print(em->cerr(), t1);
      em->cerr() << ", t2=";
      em->REAL->print(em->cerr(), t2);
      em->cerr() << " in " << name;
      owner->DoneError();
    }
    setClassification(0);
    return;
  }


  if (t2.isInfinity()) {
    setClassification(SteadyStateAccumulated, t1.getReal());
  } else {
    setClassification(TransientAccumulated, t1.getReal(), t2.getReal());
  }
}

baseacc_si::baseacc_si(const char* name, const type* arg)
 : stoch_msr(em->REAL, name, 4)
{
  DCASSERT(arg);
  SetFormal(1, arg, "x");
  SetFormal(2, em->REAL, "t1");
  SetFormal(3, em->REAL, "t2");
}

measure* baseacc_si::buildMeasure(traverse_data &x, expr** pass, int np)
{
  DCASSERT(pass);
  DCASSERT(4==np);
  return new mymsr(Name(), x.parent, x.model, pass[1], pass[2], pass[3]);
}

// ******************************************************************
// *                             avg_acc                            *
// ******************************************************************

class avgacc_si : public baseacc_si {
public:
  avgacc_si();
};

avgacc_si::avgacc_si() : baseacc_si("avg_acc", em->REAL->addProc())
{
  SetDocumentation("Computes the expected accumulated value of expression x between times t1 and t2.");
}

// *******************************************************************
// *                             prob_acc                            *
// *******************************************************************

class probacc_si : public baseacc_si {
public:
  probacc_si();
};

probacc_si::probacc_si() : baseacc_si("prob_acc", em->BOOL->addProc())
{
  SetDocumentation("Computes the expected amount of time that expression x is true, between times t1 and t2.");
}


// *************************************************************************
// *                                Helpers                                *
// *************************************************************************

engtype* MakeUnordered(exprman* em, const char* n, const char* d)
{
  engtype* et = new unordered_engtype(n, d);
  CHECK_RETURN( em->registerEngineType(et), true );
  return et;
}

engtype* MakeTimeOrdered(exprman* em, const char* n, const char* d)
{
  engtype* et = new time_engtype(n, d);
  CHECK_RETURN( em->registerEngineType(et), true );
  return et;
}

// ******************************************************************
// *                                                                *
// *                           front  end                           *
// *                                                                *
// ******************************************************************

void InitStochMeasureFuncs(exprman* em, List <msr_func> *common)
{
  // Initialize engines
  stoch_msr::SteadyStateAverage = MakeUnordered(em,
      "SteadyStateAverage",
      "Method to use for computing steady-state averages or steady-state probabilities within models"
  );

  stoch_msr::TransientAverage = MakeTimeOrdered(em,
      "TransientAverage",
      "Method to use for computing transient averages or transient probabilities within models"
  );

  stoch_msr::SteadyStateAccumulated = MakeTimeOrdered(em,
      "SteadyStateAccumulated",
      "Method to use for computing infinitely accumulated averages within models"
  );

  stoch_msr::TransientAccumulated = MakeTimeOrdered(em,
      "TransientAccumulated",
      "Method to use for computing finitely accumulated averages within models"
  );

  // Add functions
  if (0==common) return;

  common->Append(new init_dist_si);
  common->Append(new distss_si);
  common->Append(new avgss_si);
  common->Append(new probss_si);
  common->Append(new distat_si);
  common->Append(new avgat_si);
  common->Append(new probat_si);
  common->Append(new probacc_si);
  common->Append(new avgacc_si);
}


